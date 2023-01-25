#include "DHT.h"
#define DHTTYPE DHT22

#define DHTPIN 14

DHT dht(DHTPIN, DHTTYPE);

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

#include <ArduinoJson.h>
#include <Wire.h>
#include <TinyGsmClient.h>

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
const char resource[] = "http://api.thingspeak.com/update?api_key=U1A8KSB0C07LHYPQ";
const int port = 80; // server port number
String apiKeyValue = "U1A8KSB0C07LHYPQ";

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

    SerialMon.print("Connecting to APN: ");
    SerialMon.print(apn);
    if (!modem.gprsConnect(apn, gprsUser, gprsPass))
    {
        SerialMon.println(" fail");
        ESP.restart();
    }
    else
    {
        SerialMon.println(" OK");
    }

    if (modem.isGprsConnected())
    {
        SerialMon.println("GPRS connected");
    }
}

void loop()
{
    SerialMon.print("Connecting to APN: ");
    SerialMon.print(apn);
    if (!modem.gprsConnect(apn, gprsUser, gprsPass))
    {
        SerialMon.println(" fail");
    }
    else
    {
        SerialMon.println(" OK");

        SerialMon.print("Connecting to ");
        SerialMon.print(server);
        if (!client.connect(server, port))
        {
            SerialMon.println(" fail");
        }
        else
        {
            SerialMon.println(" OK");

            // Reading temperature or humidity takes about 250 milliseconds!
            // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
            humidity = dht.readHumidity();
            // Read temperature as Celsius (the default)
            temperature = dht.readTemperature();

            // Check if any reads failed and exit early (to try again).
            if (isnan(humidity) || isnan(temperature))
            {
                Serial.println(F("Failed to read from DHT sensor!"));
                return;
            }

            char tempString[8];
            dtostrf(temperature, 1, 2, tempString);
            Serial.print("Temperature: ");
            Serial.println(tempString);
            // Convert the value to a char array
            char humString[8];
            dtostrf(humidity, 1, 2, humString);
            Serial.print("Humidity: ");
            Serial.println(humString);
            // Making an HTTP POST request
            SerialMon.println("Performing HTTP POST request...");
            // Prepare your HTTP POST request data (Temperature in Celsius degrees)
            String httpRequestData = "api_key=" + apiKeyValue + "&field1=" + String(dht.readTemperature()) + "&field2=" + String(dht.readHumidity()) + "";

            client.print(String("POST ") + resource + " HTTP/1.1\r\n");
            client.print(String("Host: ") + server + "\r\n");
            client.println("Connection: close");
            client.println("Content-Type: application/x-www-form-urlencoded");
            client.print("Content-Length: ");
            client.println(httpRequestData.length());
            client.println();
            client.println(httpRequestData);

            unsigned long timeout = millis();
            while (client.connected() && millis() - timeout < 10000L)
            {
                // Print available data (HTTP response from server)
                while (client.available())
                {
                    char c = client.read();
                    SerialMon.print(c);
                    timeout = millis();
                }
            }
            SerialMon.println();

            // Close client and disconnect
            client.stop();
            SerialMon.println(F("Server disconnected"));
            modem.gprsDisconnect();
            SerialMon.println(F("GPRS disconnected"));
        }
    }

    delay(30000);
}
