//#define CAYENNE_DEBUG
//#define CAYENNE_PRINT Serial
#include <CayenneMQTTESP8266.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>

ESP8266WebServer server;
bool ota_flag = false;
uint16_t time_elapsed = 0;

// WiFi network info.
char ssid[] = "zort";
char wifiPassword[] = "jacek12345";

// Cayenne authentication info. This should be obtained from the Cayenne Dashboard.
char username[] = "145220b0-0af6-11e9-898f-c12a468aadce";
char password[] = "4631f085e8be62dcd3d08d39cd3d94d273b5a307";
char clientID[] = "2c3348c0-0b92-11e9-a08c-c5a286f8c00d";

#define DHTPIN            D4         // Pin which is connected to the DHT sensor.
#define DHTTYPE           DHT22     // DHT 22 (AM2302)

DHT dht(DHTPIN, DHTTYPE);

uint32_t delayMS;

unsigned long lastMillis = 0;

void setup() {
  WiFi.hostname("ESP_TEMP1");
	Cayenne.begin(username, password, clientID, ssid, wifiPassword);
  dht.begin();
  server.on("/update", handleOTAUpdate);
  server.begin();
  ArduinoOTA.setPassword((const char *)"Jacek1");
  ArduinoOTA.begin();
}

void handleOTAUpdate()
{
  ota_flag = true;
  server.send(200, "text/plain", "You can upload the sketch for 60 seconds!");
  time_elapsed = 0;
}

int counter = 0;

void loop() {
  if (ota_flag)
  {
    uint16_t time_start = millis();
    while(time_elapsed < 60000)
    {
      ArduinoOTA.handle();
      time_elapsed = millis()-time_start;  
      delay(10);
    }
    ota_flag = false;
  }

  server.handleClient();
	Cayenne.loop();
  if (counter++ == 50)
  {
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    Cayenne.virtualWrite(0, temp, TYPE_TEMPERATURE, UNIT_CELSIUS);
    Cayenne.virtualWrite(1, hum, TYPE_RELATIVE_HUMIDITY, UNIT_PERCENT);
    counter = 0;
  }
  
  delay(100);
}


