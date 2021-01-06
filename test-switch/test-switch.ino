#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

const char* ssid = "zort";
const char* password = "jacek12345";
const char* mqtt_server = "192.168.1.251";
const char* mqtt_user = "zort";
const char* mqtt_pass = "Jacek1Mserwis";

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  // put your setup code here, to run once:
  WiFi.hostname("ESP_SWITCH4");
  pinMode(D0, OUTPUT);
  pinMode(D5, OUTPUT);
  pinMode(D1, INPUT_PULLUP);
  pinMode(D2, INPUT_PULLUP);
  digitalWrite(D0, LOW);
  digitalWrite(D5, LOW);

  Serial.begin(9600);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
     delay(500);
     Serial.println("Connecting to WiFi..");
  }
  
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  ArduinoOTA.setPassword((const char *)"Jacek1");
  ArduinoOTA.begin();
}

void callback(char* topic, byte* payload, unsigned int length) {
 Serial.print("Message arrived [");
 Serial.print(topic);
 Serial.print("] ");
 for (int i=0;i<length;i++) {
  char receivedChar = (char)payload[i];
  Serial.print(receivedChar);
  if (receivedChar == '0')
     digitalWrite(D0, LOW);
  if (receivedChar == '1')
     digitalWrite(D0, HIGH);
  if (receivedChar == '2')
     digitalWrite(D5, LOW);
  if (receivedChar == '3')
     digitalWrite(D5, HIGH);
  if (receivedChar == '4') // sprawdzanie
  {
     // 5 - D0 = 0, 
     // 6 = D0 = 1
     // 7 = D5 = 0
     // 8 = D5 = 1
     String ret = "";
     if (digitalRead(D0) == HIGH)
     {
      ret = "6";
     }
     else
     {
      ret = "5";
     }
     
     if (digitalRead(D5) == HIGH)
     {
      ret = ret+"8";
     }
     else
     {
      ret = ret +"7";
     }
     char ch[3];
     ret.toCharArray(ch, 3);
     client.publish("homesw4in", ch);
  }
  if (receivedChar == '9')
     handleOTAUpdate();
  }
  Serial.println();
}

void reconnect() {
 // Loop until we're reconnected
 while (!client.connected()) {
 Serial.print("Attempting MQTT connection...");
 // Attempt to connect
 if (client.connect("ESP_Switch4", mqtt_user, mqtt_pass)) {
  Serial.println("connected");
  // ... and subscribe to topic
  client.subscribe("homesw4in");
 } else {
  Serial.print("failed, rc=");
  Serial.print(client.state());
  Serial.println(" try again in 5 seconds");
  // Wait 5 seconds before retrying
  delay(5000);
  }
 }
}

bool lastD1 = false;
bool lastD2 = false;

bool ota_flag = false;
uint16_t time_elapsed = 0;

void handleOTAUpdate()
{
  ota_flag = true;
  time_elapsed = 0;
}

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
  
  if (digitalRead(D1) == LOW && lastD1 == true)
  {
    digitalWrite(D0, HIGH);
    lastD1 = false;
    delay(100);
  }

  if (digitalRead(D1) == HIGH && lastD1 == false)
  {
    digitalWrite(D0, LOW);
    lastD1 = true;
    delay(100);
  }

  if (digitalRead(D2) == LOW && lastD2 == true)
  {
    digitalWrite(D5, HIGH);
    lastD2 = false;
    delay(100);
  }
  
  if (digitalRead(D2) == HIGH && lastD2 == false)
  {
    digitalWrite(D5, LOW);
    lastD2 = true;
    delay(100);
  }
  
   if (!client.connected()) {
      reconnect();
   }
   client.loop();
}
