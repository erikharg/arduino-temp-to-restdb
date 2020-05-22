# Arduino Temperature Sensor to RESTDB.io
Arduino project to monitor temperature with a DS18B20 probe and send the results to a RESTDB.io collection.

## Goal
The goal was to use the waterproof temperature sensor to measure sea water temperature and report this to a permanent record in a robust fashion, to be used for a simple web service and dashboard.

## Hardware

* Arduino MKR GSM 1400
** with SIM and antenna
* DS18B20 waterproof temperature probe
* Breadboard or other circuit
* One 4.7kOhm resistor
* Jump wires
* Power source

### Set up installation

1. Insert SIM and connect antenna to MKR GSM 1400
1. Connect GND and VCC to Gnd (black) and Vcc (red) of DS18B20
1. Connect PIN2 to the signal wire (yellow) of DS18B20
1. Connect the resistor between signal and Vcc of DS18B20
1. Set up code (below)
1. Load the software onto board
1. Set up permanent power source and make the installation rugged enough for outdoor use...

## Setup code

1. Set up your project on RESTDB.io, make a note of your API key and project name/server name
  1. Code expects RESTDB collection to be called "temperatures"
1. Install libraries (see below)
1. Define PIN code, RESTDB.io API key and RESTDB.io server name (project name) in secrets.h
1. Adjust timeouts as needed: SEND_WAIT and SAMPLE_WAIT
1. Adjust collection name in `client.post("/rest/temperatures");` if needed

### Required libraries
* MKRGSM
* ArduinoHttpClient
* Arduino_JSON
* Time
* Adafruit_SleepyDog_Library
* OneWire
* DallasTemperature

### Library modifications
* You may need to move `libraries/Time/Time.h` to another file name to not conflict with `<time.h>`. I renamed it to `_Time.h` and included `"TimeLib.h"` in my project instead of `<Time.h>`, to get it to compile on my Mac.
* You may want to change the value of `kHttpWaitForDataDelay` set in `libraries/HttpClient/HttpClient.h` from `1000` to `30` (should be more than 20). This seems to improve robustness on bad GSM networks.
