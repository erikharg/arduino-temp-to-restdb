/*
  Temperature sensor to RESTDB.io collection
  By: Erik Harg <erik@harg.no>

*/
#define LOGGING

// libraries

#include <MKRGSM.h>
#include <ArduinoHttpClient.h>
#include <Arduino_JSON.h>
#include "TimeLib.h"
#include <Adafruit_SleepyDog.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include "secrets.h" 
// Please enter your sensitive data in secrets.h
// An example is provided in the repo (secrets_example.h)

// Initialize Dallas Temperature sensors
// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// Initialize GSM system
GSM gsmAccess = GSM(false);        // GSM access: include a 'true' parameter for debug enabled
GPRS gprsAccess;  // GPRS access
GSMScanner gsmscanner;
GSMSSLClient gsmclient;  // Client service for TCP connection

// Initialize HTTP client with URL, port and path
String serverAddress = String(SERVER_ADDRESS);
int port = 443;
String path = "/rest/temperatures";
HttpClient client = HttpClient(gsmclient, serverAddress, port);

// Variable for keeping the next values to send
JSONVar tempValues;

// Variable for saving obtained response
String response = "";

// Messages for serial monitor response
String oktext = "OK";
String errortext = "ERROR";

// Timekeeping 
unsigned long sendData = 0; // next time we'll send data
unsigned long sampleData = 0; // next time we'll sample data
unsigned long SEND_WAIT = 1800; // how long to wait between submissions -- 3600 = 1h
unsigned long SAMPLE_WAIT = 1800; // how long to wait between samples  -- 600 = 10min
unsigned long LOOP_WAIT_MS = 5000;  // how long to wait between loops -- 1000 ms = 1 sec
unsigned long lastLoopMillis = 0; // time of last loop execution

void setup() {
  int countdownMS = Watchdog.enable(16000); // 16s is max timeout
  
  // initialize serial communications and wait for port to open:
  Serial.begin(9600);
  Serial.println("BEGIN");
  delay(100);
  gsmAccess.lowPowerMode();
  delay(LOOP_WAIT_MS);
  Serial.println("\n");
  Serial.println("\n");
  Serial.println("\n");
  Serial.println("\nStarting service!");
  Serial.print("Enabled the watchdog with max countdown of ");
  Serial.print(countdownMS, DEC);
  Serial.println(" milliseconds!");
  sensors.begin();
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
}

void loop() {
  unsigned long currentMillis = millis();
  //Serial.println("millis: " + String(currentMillis));
  if(currentMillis - lastLoopMillis >= LOOP_WAIT_MS)
  {
    Watchdog.reset();
    digitalWrite(LED_BUILTIN, HIGH);
    
    time_t ticktime = now();
    if(ticktime < 10000)
    {

      updateTimeKeeper();
      ticktime = now();
    }
    Watchdog.reset();

    if(ticktime > sampleData) {
      int sampleNo = tempValues.length();
      if(sampleNo < 0) {
        sampleNo = 0;
      }
  
      Serial.print("Sampling (#" + String(sampleNo) + ") at ");
      Serial.print(formatDateTime(ticktime) + "...");
      JSONVar sample;
      Watchdog.reset();
      float tempVal = getTemp();
      Watchdog.reset();
      if(tempVal != DEVICE_DISCONNECTED_C && tempVal > -127.0f)
      {
        sample["temperature"] = tempAsString(tempVal);
        sample["time"] = formatDateTime(ticktime);
  
        int sensorValue = analogRead(ADC_BATTERY);
        // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 4.3V):
        float voltage = sensorValue * (4.3 / 1023.0);
        sample["voltage"] = formatVoltageAsString(voltage);
  
        tempValues[sampleNo] = sample;
        Serial.print(sample["temperature"]);
        Serial.println(oktext);
        sampleData = now() + SAMPLE_WAIT; // wait this long until we sample data again
        Serial.println("Waiting until " + formatDateTime(sampleData) + " to sample data again");
      } else {
        Serial.println("bad value " + tempAsString(tempVal));
      }
    }

    if(ticktime > sendData && tempValues.length() > 0)
    {
      // start GSM shield
      // if your SIM has PIN, pass it as a parameter of begin() in quotes
      gsmAccess.noLowPowerMode();
      Serial.print("Connecting to GSM network at ");
      Serial.print(formatDateTime(ticktime) + "...");
      Watchdog.reset();
      if (gsmAccess.begin(PINNUMBER) != GSM_READY) {
        Serial.println(errortext);
        Watchdog.reset();
      } else {
        Serial.println(oktext);
        Serial.println("GSM connected at signal strength " + gsmscanner.getSignalStrength() + "/32");
        Watchdog.reset();
  
        if(tempValues.length() > 0) // only connect if we have values to send
        {
          // attach GPRS
          Serial.print("Attaching to GPRS with your APN...");
          if (gprsAccess.attachGPRS("telenor", "","") != GPRS_READY) {
            Watchdog.reset();
            Serial.println(errortext);
          } else {
            Serial.println(oktext);
            Watchdog.reset();
        
            Serial.print("Making HTTP POST request with ");
            Serial.print(tempValues.length());
            Serial.print(" samples:");
            String contentType = "application/json";
            String postData = JSON.stringify(tempValues);
            //Serial.println(postData);
  
            Watchdog.reset();
            client.beginRequest();
            Watchdog.reset();
            Serial.print(".");
            client.post("/rest/temperatures");
            Watchdog.reset();
            Serial.print(".");
            client.sendHeader(HTTP_HEADER_CONTENT_TYPE, contentType);
            Watchdog.reset();
            Serial.print(".");
            client.sendHeader(HTTP_HEADER_CONTENT_LENGTH, postData.length());
            Watchdog.reset();
            Serial.print(".");
            client.sendHeader("x-apikey", X_API_KEY);
            Watchdog.reset();
            Serial.print(".");
            client.beginBody();
            Watchdog.reset();
            Serial.print(".");
            client.print(postData);
            Watchdog.reset();
            Serial.print(".");
            client.endRequest();
            Watchdog.reset();
            Serial.print("OK\n");
        
            // read the status code and body of the response
            Serial.print("getting response: ");
            int statusCode = client.responseStatusCode();
            Watchdog.reset();
            Serial.print(".");
            String response = client.responseBody();
            Watchdog.reset();
            Serial.print("OK\n");
          
            Serial.print("Status code: ");
            Serial.println(statusCode);
            Serial.print("Response: ");
            Serial.println(response);
            if(statusCode >= 200 && statusCode < 300) {
              tempValues = JSONVar(); // empty the value array
            }
          }
        } else {
          Serial.println("No data to send!");
          sendData = 0;
        }
        Watchdog.reset();
      }
      sendData = now() + SEND_WAIT; // wait this long until we send data again
      Serial.println("Waiting until " + formatDateTime(sendData) + " to send data again");
    }
    //Serial.println("Loop done");
    gsmAccess.shutdown();
    Watchdog.reset();
    gsmAccess.lowPowerMode();
    digitalWrite(LED_BUILTIN, LOW);
    lastLoopMillis = millis(); // set the timing for the next loop
    Watchdog.reset();
  }
}

String formatDateTime(time_t t) {
 int y = year(t);
 int mn = month(t);
 int d = day(t);
 int h = hour(t);
 int mi = minute(t);
 int s = second(t);

 String y_s = String(y);
 String mn_s = mn > 9 ? String(mn) : ("0" + String(mn));
 String d_s = d > 9 ? String(d) : ("0" + String(d));
 String h_s = h > 9 ? String(h) : ("0" + String(h));
 String mi_s = mi > 9 ? String(mi) : ("0" + String(mi));
 String s_s = s > 9 ? String(s) : ("0" + String(s));
 
 String retval = y_s + "-" + mn_s + "-" + d_s + " " + h_s + ":" + mi_s + ":" + s_s + " UTC";

 return retval;
 
}

float getTemp() {
  sensors.requestTemperatures(); // Send the command to get temperatures
  return sensors.getTempCByIndex(0);  
}

String tempAsString(float tempC) {
  String tempString = String(tempC);
  char tempChars[6];
  tempString.toCharArray(tempChars, 5);
  return String(tempChars);
}

String formatVoltageAsString(float voltage) {
  String tempString = String(voltage);
  char vChars[4];
  tempString.toCharArray(vChars, 4);
  return String(vChars);
}

void updateTimeKeeper()
{
    Watchdog.reset();
    gsmAccess.noLowPowerMode();
    Serial.println("Update timekeeper");
    if (gsmAccess.begin(PINNUMBER) != GSM_READY) {
      Serial.println("Timekeeper:");
      Serial.println(errortext);
      Serial.println("Timekeeper done.");
      Watchdog.reset();
    } else {
      Serial.println("GSM connected at signal strength " + gsmscanner.getSignalStrength() + "/32");
      Serial.println("Timekeeper:");
      Serial.println(oktext);
      Serial.println("Timekeeper done.");
      Watchdog.reset();
      setTime(gsmAccess.getTime());
    }
} 
