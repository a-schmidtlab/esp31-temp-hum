# ESP32 Temperature and Humidity Monitor

This project implements a web-based temperature and humidity monitoring system using an ESP32-ETH01 v1.4 microcontroller with a DHT11 sensor. The system creates a local web server that displays real-time measurements and historical data through an interactive web interface.

## Features

- Real-time temperature and humidity monitoring
- Interactive web interface accessible via local network
- Historical data visualization with time-based filtering
- Automatic measurements every 30 seconds
- Responsive design for various screen sizes

## Hardware Requirements

- ESP32-ETH01 v1.4
- DHT11 Temperature and Humidity Sensor
- Ethernet cable
- Power supply (5V)
- Jumper wires

## Wiring

Connect the DHT11 sensor to the ESP32 as follows:

- DHT11 VCC → ESP32 3.3V
- DHT11 GND → ESP32 GND
- DHT11 DATA → ESP32 GPIO4 (configurable)

## Software Requirements

- Arduino IDE
- Required Libraries:
  - ESP32 board support package
  - DHT sensor library
  - ArduinoJson
  - WebSockets
  - SPIFFS

## Setup Instructions

1. Install the required libraries in Arduino IDE
2. Connect the hardware according to the wiring diagram
3. Upload the code to your ESP32
4. Connect the ESP32 to your local network via Ethernet
5. Access the web interface using the ESP32's IP address

## Web Interface

The web interface provides:
- Current temperature and humidity readings
- Interactive graph showing historical data
- Time range selection (Last Hour, Last 12 Hours, All Data)
- Real-time updates

## Data Storage

The system stores measurements in the ESP32's flash memory using SPIFFS, allowing for historical data visualization even after power cycles.

## Network Configuration

The ESP32 will automatically obtain an IP address via DHCP when connected to your network. The IP address will be displayed in the Serial Monitor during startup.

## License

(C) 2025 by Axel Schmidt
This project is open source and available under the MIT License. 