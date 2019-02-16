#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <ThingSpeak.h>
#include "dht_sensor.h"

unsigned long myChannelNumber = 390349;
const char *myWriteAPIKey = "8GLZVK5VYFC9UI9N";

char ssid[] = "zort"; //  your network SSID (name)
char pass[] = "jacek12345";

unsigned long lastwrite = 0;

String esp_chipid;
String server_name;

ESP8266WebServer server;

bool ota_flag = false;
uint16_t time_elapsed = 0;
WiFiClient client;

void setup()
{
  WiFi.hostname("ESP_TEMPS");
  
//  Serial.begin(115200); // Output to Serial at 9600 baud
  //esp_chipid = String(ESP.getChipId());
  //WiFi.persistent(false);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
//    Serial.print(".");
  }

  ArduinoOTA.setPassword((const char *)"Jacek1");
  ArduinoOTA.setHostname("Termometr-sypialnia");
  ArduinoOTA.begin();
//  Serial.println("");
//  Serial.println("WiFi connected");
//  Serial.println("IP address: ");
//  Serial.println(WiFi.localIP());
  ThingSpeak.begin(client);
  setup_dht();
  // sometimes parallel sending data and web page will stop nodemcu, watchdogtimer set to 30 seconds
//  wdt_disable();
//  wdt_enable(30000); // 30 sec
  
  server.on("/update", handleOTAUpdate);
  server.begin();
}

void sendTSData()
{
  if (DHT_readings.temp > 0 && DHT_readings.hum > 0)
  {
    ThingSpeak.setField(7, DHT_readings.temp);
    ThingSpeak.setField(8, DHT_readings.hum);
    ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  }
//  Serial.print("Sent temp: ");
//  Serial.print(DHT_readings.temp);
//  Serial.print(" and hum: ");
//  Serial.println(DHT_readings.hum);
}

void handleOTAUpdate()
{
  ota_flag = true;
  server.send(200, "text/plain", "You can upload the sketch for 30 seconds!");
  time_elapsed = 0;
}

int counter = 0;

void loop()
{
  if (ota_flag)
  {
    uint16_t time_start = millis();
    while(time_elapsed < 30000)
    {
      ArduinoOTA.handle();
      time_elapsed = millis()-time_start;  
      delay(10);
    }
    ota_flag = false;
  }

  if (counter%20 == 0)
  {
    read_DHT();
  }
  
  if (counter++ == 300*5) // 5 minut
  {
    sendTSData();
    counter = 0;
  }
  server.handleClient();
  delay(200);
  
  //Serial.print("TICK:\t");
  //Serial.println(millis() / 1000);
}
