#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>

const char* ssid = "zort";
const char* password = "jacek12345";
 
WebSocketsServer webSocket(81);
ESP8266WebServer server;
bool ota_flag = false;
uint16_t time_elapsed = 0;

void processReceivedValue(char command){
 
  if(command == '1')
  { 
    digitalWrite(D2, HIGH); 
    digitalWrite(D1, LOW); 
  }
  else if(command == '0')
  { 
    digitalWrite(D2, LOW);
    digitalWrite(D1, LOW); 
  }
  else if(command == '2')
  { 
    digitalWrite(D1, HIGH);
    digitalWrite(D2, LOW); 
  }
 
  return;
}

void startWebSocket() { // Start a WebSocket server
  webSocket.begin();                          // start the websocket server
  webSocket.onEvent(webSocketEvent);          // if there's an incomming websocket message, go to function 'webSocketEvent'
//  Serial.println("WebSocket server started.");
}

void setup() {
  // put your setup code here, to run once:
  pinMode(D2, OUTPUT);
  pinMode(D1, OUTPUT);
  digitalWrite(D2, LOW);
  digitalWrite(D1, LOW);
//  Serial.begin(115200);
 
  delay(1000);

  WiFi.hostname("ESP_rolety");
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
//    Serial.println("Connecting..");
  }
 
//  Serial.print("Connected to WiFi. IP:");
//  Serial.println(WiFi.localIP());

  ArduinoOTA.setPassword((const char *)"Jacek1");
  ArduinoOTA.begin();
  startWebSocket();
  server.on("/update", handleOTAUpdate);
  server.on("/restart", rebootESP);
  server.begin();
}

void rebootESP()
{
  ESP.restart();
}

void handleOTAUpdate()
{
  ota_flag = true;
  server.send(200, "text/plain", "You can upload the sketch for 60 seconds!");
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

  server.handleClient();  
  webSocket.loop();
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) { // When a WebSocket message is received
  switch (type) {
    case WStype_DISCONNECTED:             // if the websocket is disconnected
      //Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {              // if a new websocket connection is established
        IPAddress ip = webSocket.remoteIP(num);
        //Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      break;
    case WStype_TEXT:                     // if new text data is received
      //Serial.printf("[%u] get Text: %s\n", num, payload);
      processReceivedValue(payload[0]);
      break;
  }
}
