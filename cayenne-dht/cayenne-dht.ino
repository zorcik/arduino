//#define CAYENNE_DEBUG
//#define CAYENNE_PRINT Serial
#include <CayenneMQTTESP8266.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

// WiFi network info.
char ssid[] = "zort";
char wifiPassword[] = "jacek12345";

// Cayenne authentication info. This should be obtained from the Cayenne Dashboard.
char username[] = "145220b0-0af6-11e9-898f-c12a468aadce";
char password[] = "4631f085e8be62dcd3d08d39cd3d94d273b5a307";
char clientID[] = "2c3348c0-0b92-11e9-a08c-c5a286f8c00d";

#define DHTPIN            D4         // Pin which is connected to the DHT sensor.
#define DHTTYPE           DHT22     // DHT 22 (AM2302)

DHT_Unified dht(DHTPIN, DHTTYPE);

uint32_t delayMS;

unsigned long lastMillis = 0;

void setup() {
	Cayenne.begin(username, password, clientID, ssid, wifiPassword);
  dht.begin();
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  delayMS = sensor.min_delay / 1000;
}

void loop() {
  delay(delayMS);
	Cayenne.loop();
  sensors_event_t event;  
  dht.temperature().getEvent(&event);
  float temp = event.temperature;
  dht.humidity().getEvent(&event);
  float hum = event.relative_humidity;
  Cayenne.virtualWrite(0, temp, TYPE_TEMPERATURE, UNIT_CELSIUS);
  Cayenne.virtualWrite(1, hum, TYPE_RELATIVE_HUMIDITY, UNIT_PERCENT);
}


