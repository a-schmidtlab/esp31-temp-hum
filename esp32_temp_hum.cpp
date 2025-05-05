// -*- mode: c++ -*-
#include <WiFi.h>
#include <ETH.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <SPIFFS.h>
#include <FS.h>

// Pin definitions
#define DHTPIN 4
#define DHTTYPE DHT11

// Constants
const unsigned long MEASUREMENT_INTERVAL = 30000; // 30 seconds
const int MAX_DATA_POINTS = 2880; // 24 hours of data (30s intervals)

// Global variables
DHT dht(DHTPIN, DHTTYPE);
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);
unsigned long lastMeasurement = 0;
float temperature = 0;
float humidity = 0;
struct DataPoint {
  unsigned long timestamp;
  float temperature;
  float humidity;
};
DataPoint dataPoints[MAX_DATA_POINTS];
int dataIndex = 0;

// HTML content
const char* htmlContent = R"(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 Temperature & Humidity Monitor</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        .container { max-width: 800px; margin: 0 auto; }
        .current-values { display: flex; justify-content: space-around; margin: 20px 0; }
        .value-box { text-align: center; padding: 20px; background: #f0f0f0; border-radius: 10px; }
        .controls { margin: 20px 0; }
        button { padding: 10px 20px; margin: 0 5px; cursor: pointer; }
        canvas { width: 100% !important; height: 400px !important; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Temperature & Humidity Monitor</h1>
        <div class="current-values">
            <div class="value-box">
                <h2>Temperature</h2>
                <p id="current-temp">--°C</p>
            </div>
            <div class="value-box">
                <h2>Humidity</h2>
                <p id="current-hum">--%</p>
            </div>
        </div>
        <div class="controls">
            <button onclick="setTimeRange('hour')">Last Hour</button>
            <button onclick="setTimeRange('12hours')">Last 12 Hours</button>
            <button onclick="setTimeRange('all')">All Data</button>
        </div>
        <canvas id="chart"></canvas>
    </div>
    <script>
        let chart;
        let timeRange = 'hour';
        
        function initChart() {
            const ctx = document.getElementById('chart').getContext('2d');
            chart = new Chart(ctx, {
                type: 'line',
                data: {
                    labels: [],
                    datasets: [{
                        label: 'Temperature (°C)',
                        data: [],
                        borderColor: 'rgb(255, 99, 132)',
                        tension: 0.1
                    }, {
                        label: 'Humidity (%)',
                        data: [],
                        borderColor: 'rgb(54, 162, 235)',
                        tension: 0.1
                    }]
                },
                options: {
                    responsive: true,
                    scales: {
                        x: {
                            type: 'time',
                            time: {
                                unit: 'minute'
                            }
                        }
                    }
                }
            });
        }

        function updateChart(data) {
            const now = Date.now();
            const ranges = {
                'hour': 3600000,
                '12hours': 43200000,
                'all': Infinity
            };
            
            const filteredData = data.filter(point => 
                now - point.timestamp <= ranges[timeRange]
            );

            chart.data.labels = filteredData.map(point => 
                new Date(point.timestamp).toLocaleTimeString()
            );
            chart.data.datasets[0].data = filteredData.map(point => point.temperature);
            chart.data.datasets[1].data = filteredData.map(point => point.humidity);
            chart.update();
        }

        function setTimeRange(range) {
            timeRange = range;
            fetch('/data')
                .then(response => response.json())
                .then(data => updateChart(data));
        }

        function updateCurrentValues(data) {
            document.getElementById('current-temp').textContent = 
                data.temperature.toFixed(1) + '°C';
            document.getElementById('current-hum').textContent = 
                data.humidity.toFixed(1) + '%';
        }

        // WebSocket connection
        const ws = new WebSocket('ws://' + window.location.hostname + ':81/');
        
        ws.onmessage = function(event) {
            const data = JSON.parse(event.data);
            updateCurrentValues(data);
        };

        // Initial data load
        fetch('/data')
            .then(response => response.json())
            .then(data => {
                updateChart(data);
                if (data.length > 0) {
                    updateCurrentValues(data[data.length - 1]);
                }
            });

        initChart();
    </script>
</body>
</html>
)";

void setup() {
  Serial.begin(115200);
  
  // Initialize SPIFFS
  if(!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  // Initialize DHT sensor
  dht.begin();

  // Initialize Ethernet
  ETH.begin();
  
  // Wait for Ethernet connection
  while (ETH.linkStatus() != LinkON) {
    delay(100);
  }
  
  Serial.print("IP address: ");
  Serial.println(ETH.localIP());

  // Setup web server routes
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", htmlContent);
  });

  server.on("/data", HTTP_GET, []() {
    StaticJsonDocument<1024> doc;
    JsonArray array = doc.to<JsonArray>();
    
    for (int i = 0; i < dataIndex; i++) {
      JsonObject point = array.createNestedObject();
      point["timestamp"] = dataPoints[i].timestamp;
      point["temperature"] = dataPoints[i].temperature;
      point["humidity"] = dataPoints[i].humidity;
    }
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  });

  // Start web server and WebSocket
  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  server.handleClient();
  webSocket.loop();

  unsigned long currentMillis = millis();
  
  if (currentMillis - lastMeasurement >= MEASUREMENT_INTERVAL) {
    lastMeasurement = currentMillis;
    
    // Read sensor data
    float newTemp = dht.readTemperature();
    float newHum = dht.readHumidity();
    
    if (!isnan(newTemp) && !isnan(newHum)) {
      temperature = newTemp;
      humidity = newHum;
      
      // Store data point
      dataPoints[dataIndex].timestamp = currentMillis;
      dataPoints[dataIndex].temperature = temperature;
      dataPoints[dataIndex].humidity = humidity;
      
      dataIndex = (dataIndex + 1) % MAX_DATA_POINTS;
      
      // Broadcast to all connected clients
      StaticJsonDocument<200> doc;
      doc["temperature"] = temperature;
      doc["humidity"] = humidity;
      String jsonString;
      serializeJson(doc, jsonString);
      webSocket.broadcastTXT(jsonString);
    }
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED:
      Serial.printf("[%u] Connected\n", num);
      break;
  }
} 