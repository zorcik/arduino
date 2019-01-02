unsigned long myChannelNumber = 390349;
const char *myWriteAPIKey = "8GLZVK5VYFC9UI9N";

char ssid[] = "zort"; //  your network SSID (name)
char pass[] = "jacek12345";

#define TS_DELAY 60 * 1000
#include <SoftwareSerial.h>
#include <ThingSpeak.h>

#include <ESP8266WiFi.h>
#include "dht_sensor.h"
#include <SoftwareSerial.h>

bool is_SDS_running = true;

#define SDS_PIN_RX D1
#define SDS_PIN_TX D2

SoftwareSerial serialSDS(SDS_PIN_RX, SDS_PIN_TX, false, 1024);

unsigned long lastwrite = 0;


String esp_chipid;

String server_name;

/*****************************************************************
  /* Debug output                                                  *
  /*****************************************************************/
void debug_out(const String &text, int linebreak = 1)
{
  if (linebreak)
  {
    Serial.println(text);
  }
  else
  {
    Serial.print(text);
  }
}

WiFiClient client;

void setup()
{
  //  analogReference(EXTERNAL);
  Serial.begin(115200); // Output to Serial at 9600 baud
  esp_chipid = String(ESP.getChipId());
  WiFi.persistent(false);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  int status = WL_IDLE_STATUS;
  ThingSpeak.begin(client);
  debug_out("\nChipId: ", 0);
  debug_out(esp_chipid, 1);

  setup_dht();
  delay(2000);
  // sometimes parallel sending data and web page will stop nodemcu, watchdogtimer set to 30 seconds
  wdt_disable();
  wdt_enable(30000); // 30 sec

}

void sendTSData()
{
  ThingSpeak.setField(5, DHT_readings.temp);
  ThingSpeak.setField(6, DHT_readings.hum);
  ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  Serial.print("Sent temp: ");
  Serial.print(DHT_readings.temp);
  Serial.print(" and hum: ");
  Serial.println(DHT_readings.hum);
}

int counter = 0;

void loop()
{
  read_DHT();
  if (counter++ == 30*5) // 5 minut
  {
    sendTSData();
    counter = 0;
  }
  delay(2000);
  Serial.print("TICK:\t");
  Serial.println(millis() / 1000);
}
