# WiFi RF Door Lock
This is an example code written to showcase the usage of RF Module
and WiFi to control a relay to lock and unlock a door.
The ESP32 Dev Board is connected to a 433MHz RF Reciever and a
Relay. You can add custom RF keys to the ``authorizedRFKeys``
array and allow access control to specific key combinations.
Also the ESP32 connects to an MQTT Server to provide control
using WiFi.

## Features
- WiFi and RF Control
- Asynchronous connect and reconnect of WiFi
- Asynchronous reconnect of MQTT Server.

## Prerequisites
This code uses MQTT to control the door lock, so you will need
to have a working MQTT Server beforehand. You can set up an
MQTT Server on a Raspberry Pi following [this tutorial](https://www.youtube.com/watch?v=_DO2wHI6JWQ ).

## Usage
- Clone the repository
```bash
git clone https://github.com/mathaimon/esp-wifi-rf-door-lock.git
```
- Rename the ``src/config.example.h`` file to ``src/config.h``
- Edit the ``config.h`` file to add your own WiFi, MQTT and OTA Credentials
- Set the ``#define ELEGANTOTA_USE_ASYNC_WEBSERVER`` to ``1`` to use AsynWebserver
- Flash the file to ESP.
