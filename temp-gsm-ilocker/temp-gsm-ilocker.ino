 #include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#define DHTTYPE DHT11

#define DHTPIN 14

DHT_Unified dht(DHTPIN, DHTTYPE);

#define TINY_GSM_MODEM_SIM800 // Modem is SIM800L

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial
// Set serial for AT commands
#define SerialAT Serial1
// Set serial for RS485
#define SerialRS Serial2

#define RedLED 27
#define BlueLED 25
#define GreenLED 26

// Define the serial console for debug prints, if needed
// #define TINY_GSM_DEBUG SerialMon
// #define DUMP_AT_COMMANDS

// set GSM PIN, if any
#define GSM_PIN ""

// Your GPRS credentials, if any
const char apn[] = "internet";
const char gprsUser[] = "";
const char gprsPass[] = "";

// SIM card PIN (leave empty, if not defined)
const char simPIN[] = "";

#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

TinyGsmClient client(modem);

#define MODEM_TX 17
#define MODEM_RX 16

#define SERIAL_TX 18
#define SERIAL_RX 19

uint32_t lastReconnectAttempt = 0;

long lastMsg = 0;

float temperature = 0;
float humidity = 0;
const char server[] = "thingspeak.com"; // domain name: example.com, maker.ifttt.com, etc
const char resource[] = "http://api.thingspeak.com/update?";
const int port = 80; // server port number
String apiKeyValue = "U1A8KSB0C07LHYPQ";

HttpClient http(client, server, port);

void litLED(int led)
{
    if (led != RedLED)
        digitalWrite(RedLED, LOW);
    if (led != BlueLED)
        digitalWrite(BlueLED, LOW);
    if (led != GreenLED)
        digitalWrite(GreenLED, LOW);

    digitalWrite(led, HIGH);
}

void setup()
{
    pinMode(RedLED, OUTPUT);
    pinMode(BlueLED, OUTPUT);
    pinMode(GreenLED, OUTPUT);

    digitalWrite(RedLED, LOW);
    digitalWrite(BlueLED, HIGH);
    digitalWrite(GreenLED, LOW);
    // Set console baud rate
    SerialMon.begin(115200);
    
    dht.begin();
    sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  SerialMon.println(F("------------------------------------"));
  SerialMon.println(F("Temperature Sensor"));
  SerialMon.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  SerialMon.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  SerialMon.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  SerialMon.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  SerialMon.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  SerialMon.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  SerialMon.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  SerialMon.println(F("Humidity Sensor"));
  SerialMon.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  SerialMon.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  SerialMon.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  SerialMon.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  SerialMon.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  SerialMon.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  SerialMon.println(F("------------------------------------"));    


    delay(100);

    SerialMon.println("Wait...");

    // Set GSM module baud rate and UART pins
    SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX, false);
    delay(6000);

    // Restart takes quite some time
    // To skip it, call init() instead of restart()
    SerialMon.println("Initializing modem...");
    modem.restart();
    // modem.init();

    String modemInfo = modem.getModemInfo();
    SerialMon.print("Modem Info: ");
    SerialMon.println(modemInfo);

    // Unlock your SIM card with a PIN if needed
    if (GSM_PIN && modem.getSimStatus() != 3)
    {
        modem.simUnlock(GSM_PIN);
    }
}

void loop()
{
   if (!modem.isGprsConnected())
   {
    SerialMon.print("Connecting to APN: ");
    SerialMon.print(apn);
    if (!modem.gprsConnect(apn, gprsUser, gprsPass))
    {
        SerialMon.println(" fail");
    }
    return;    
   }
    else
    {
        SerialMon.println(" OK");
        litLED(GreenLED);

  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    SerialMon.println(F("Error reading temperature!"));
    litLED(RedLED);
    delay(5000);
    return;
  }
  else {
    SerialMon.print(F("Temperature: "));
    SerialMon.print(event.temperature);
    SerialMon.println(F("°C"));
    temperature = event.temperature;
  }
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    SerialMon.println(F("Error reading humidity!"));
    litLED(RedLED);    
    delay(5000);
    return;
  }
  else {
    SerialMon.print(F("Humidity: "));
    SerialMon.print(event.relative_humidity);
    SerialMon.println(F("%"));
    humidity = event.relative_humidity;
  }        

        SerialMon.println("Performing HTTP GET request...");
        // Prepare your HTTP POST request data (Temperature in Celsius degrees)
        String httpRequestData = "api_key=" + apiKeyValue + "&field1=" + String(temperature) + "&field2=" + String(humidity) + "";

        SerialMon.print(F("Performing HTTPS GET request... "));
        http.connectionKeepAlive(); // Currently, this is needed for HTTPS
        String fullURL = resource + httpRequestData;
        SerialMon.println(fullURL);
        int err = http.get(fullURL);
        if (err != 0)
        {
            SerialMon.println(F("failed to connect"));
            delay(10000);
            return;
        }

int status = http.responseStatusCode();
  SerialMon.print(F("Response status code: "));
  SerialMon.println(status);
  if (!status) {
    return;
  }

  SerialMon.println(F("Response Headers:"));
  while (http.headerAvailable()) {
    String headerName  = http.readHeaderName();
    String headerValue = http.readHeaderValue();
    SerialMon.println("    " + headerName + " : " + headerValue);
  }

  int length = http.contentLength();
  if (length >= 0) {
    SerialMon.print(F("Content length is: "));
    SerialMon.println(length);
  }
  if (http.isResponseChunked()) {
    SerialMon.println(F("The response is chunked"));
  }

  String body = http.responseBody();
  SerialMon.println(F("Response:"));
  SerialMon.println(body);

  SerialMon.print(F("Body length is: "));
  SerialMon.println(body.length());        

        // Shutdown

        http.stop();
        SerialMon.println(F("Server disconnected"));

        litLED(BlueLED);
        //}
    }

    delay(60000);
}
