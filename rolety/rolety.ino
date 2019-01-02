#include "ESP8266WiFi.h"
#include <WebSocketsServer.h>

const char* ssid = "zort";
const char* password = "jacek12345";
 
//WiFiServer wifiServer(80);
WebSocketsServer webSocket(81);

void processReceivedValue(char command){
 
  if(command == '1')
  { 
    digitalWrite(D0, HIGH); 
    digitalWrite(D1, LOW); 
  }
  else if(command == '0')
  { 
    digitalWrite(D0, LOW);
    digitalWrite(D1, LOW); 
  }
  else if(command == '2')
  { 
    digitalWrite(D1, HIGH);
    digitalWrite(D0, LOW); 
  }
 
  return;
}

void startWebSocket() { // Start a WebSocket server
  webSocket.begin();                          // start the websocket server
  webSocket.onEvent(webSocketEvent);          // if there's an incomming websocket message, go to function 'webSocketEvent'
  Serial.println("WebSocket server started.");
}

void setup() {
  // put your setup code here, to run once:
  pinMode(D0, OUTPUT);
  pinMode(D1, OUTPUT);
  digitalWrite(D0, LOW);
  digitalWrite(D1, LOW);
  Serial.begin(115200);
 
  delay(1000);

  //WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  //IPAddress ip(192,168,1,242);   
  //IPAddress gateway(192,168,1,1);   
  //IPAddress subnet(255,255,255,0);   
  //WiFi.config(ip, gateway, subnet);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting..");
  }
 
  Serial.print("Connected to WiFi. IP:");
  Serial.println(WiFi.localIP());
 
  //wifiServer.begin();
  startWebSocket();
}

void loop() {
  /*
  WiFiClient client = wifiServer.available();
 
  if (client) {
 
    while (client.connected()) {
 
      while (client.available()>0) {
        char c = client.read();
        processReceivedValue(c);
        Serial.write(c);
      }
 
      delay(10);
    }
 
    client.stop();
    Serial.println("Client disconnected");
 
  }
  */
  webSocket.loop();
  
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) { // When a WebSocket message is received
  switch (type) {
    case WStype_DISCONNECTED:             // if the websocket is disconnected
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {              // if a new websocket connection is established
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      break;
    case WStype_TEXT:                     // if new text data is received
      Serial.printf("[%u] get Text: %s\n", num, payload);
      processReceivedValue(payload[0]);
      break;
  }
}
