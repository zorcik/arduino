#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <CayenneMQTTESP8266.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <WebSocketsServer.h>

ESP8266WebServer server;
WebSocketsServer webSocket = WebSocketsServer(81);

uint8_t pin_led = 2;
char* ssid = ""; //not used
char* password = "";
char* mySsid = "ESP_REMOTE1";

IPAddress local_ip(192,168,11,4);
IPAddress gateway(192,168,11,1);
IPAddress netmask(255,255,255,0);

bool ota_flag = false;
uint16_t time_elapsed = 0;

char username[] = "145220b0-0af6-11e9-898f-c12a468aadce";
char cpassword[] = "4631f085e8be62dcd3d08d39cd3d94d273b5a307";
char clientID[] = "d41dfc80-0ee7-11e9-898f-c12a468aadce";

char webpage[] PROGMEM = R"=====(
<html>
<head>
</head>
<body>
  <form>
    <fieldset>
      <div>
        <label for="ssid">SSID</label>      
        <input value="" id="ssid" placeholder="SSID">
      </div>
      <div>
        <label for="password">PASSWORD</label>
        <input type="password" value="" id="password" placeholder="PASSWORD">
      </div>
      <div>
        <button class="primary" id="savebtn" type="button" onclick="myFunction()">SAVE</button>
      </div>
    </fieldset>
  </form>
</body>
<script>
function myFunction()
{
  console.log("button was clicked!");
  var ssid = document.getElementById("ssid").value;
  var password = document.getElementById("password").value;
  var data = {ssid:ssid, password:password};
  var xhr = new XMLHttpRequest();
  var url = "/settings";
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      // Typical action to be performed when the document is ready:
      if(xhr.responseText != null){
        console.log(xhr.responseText);
      }
    }
  };
  xhr.open("POST", url, true);
  xhr.send(JSON.stringify(data));
};
</script>
</html>
)=====";

// kody pilota cyfrowego polsatu
uint16_t power[77] = {4640, 4376,  652, 376,  624, 376,  624, 376,  624, 374,  624, 1348,  652, 1348,  652, 376,  624, 374,  624, 1348,  652, 376,  624, 1348,  652, 1348,  652, 376,  624, 374,  624, 376,  624, 376,  624, 4384,  652, 376,  624, 376,  624, 376,  624, 374,  624, 376,  624, 1348,  652, 1348,  652, 1348,  652, 1348,  652, 376,  624, 376,  624, 376,  624, 1348,  652, 376,  624, 376,  624, 376,  624, 376,  624, 1348,  652, 1348,  652, 1348,  652};  // UNKNOWN 446117B3
uint16_t pplus[77] = {4638, 4378,  652, 376,  622, 376,  624, 374,  624, 376,  624, 1348,  652, 1348,  652, 376,  624, 376,  624, 1348,  652, 376,  624, 1348,  652, 1348,  652, 376,  624, 376,  624, 374,  624, 376,  624, 4382,  652, 378,  622, 376,  624, 376,  624, 376,  624, 376,  624, 376,  624, 376,  624, 376,  624, 376,  624, 1348,  652, 1348,  652, 376,  624, 1348,  652, 1348,  652, 1348,  652, 1348,  652, 1348,  652, 376,  624, 376,  624, 1348,  652};  // UNKNOWN 88617E93
uint16_t pminus[77] = {4638, 4376,  652, 376,  624, 374,  624, 376,  624, 374,  624, 1348,  652, 1348,  652, 376,  624, 376,  624, 1348,  652, 374,  624, 1348,  652, 1348,  652, 376,  624, 374,  624, 376,  624, 376,  624, 4384,  650, 376,  624, 376,  624, 376,  624, 374,  624, 1348,  652, 376,  624, 376,  624, 376,  624, 376,  624, 1350,  650, 1350,  650, 376,  624, 376,  624, 1348,  652, 1348,  652, 1348,  652, 1348,  652, 376,  624, 376,  624, 1348,  652};  // UNKNOWN 7516EE95
uint16_t one[77] = {4638, 4376,  652, 376,  624, 374,  626, 374,  626, 374,  626, 1348,  652, 1348,  652, 374,  626, 374,  626, 1348,  652, 374,  626, 1346,  654, 1348,  652, 374,  624, 374,  626, 374,  626, 374,  626, 4382,  652, 376,  624, 374,  626, 374,  626, 374,  626, 374,  626, 374,  626, 374,  626, 374,  626, 374,  626, 374,  626, 374,  626, 374,  626, 1348,  652, 1348,  652, 1348,  652, 1348,  652, 1348,  652, 1348,  652, 1348,  652, 1348,  652};  // UNKNOWN 4863A6FB
uint16_t two[77] = {4640, 4378,  650, 376,  624, 374,  626, 374,  626, 374,  626, 1348,  652, 1348,  652, 374,  626, 374,  626, 1348,  652, 374,  626, 1346,  654, 1348,  652, 374,  624, 374,  626, 374,  626, 374,  626, 4382,  652, 376,  624, 374,  626, 374,  626, 374,  626, 1346,  654, 374,  624, 374,  626, 374,  626, 374,  626, 374,  626, 374,  626, 374,  626, 374,  626, 1348,  652, 1348,  652, 1348,  652, 1348,  652, 1348,  652, 1348,  652, 1348,  652};  // UNKNOWN BC282835










IRsend irsend(4); // An IR LED is controlled by GPIO pin 4 (D2)

void wifiConnect()
{
  //reset networking
  WiFi.softAPdisconnect(true);
  WiFi.disconnect();          
  delay(1000);
  //check for stored credentials
  if(SPIFFS.exists("/config.json")){
    const char * _ssid = "", *_pass = "";
    File configFile = SPIFFS.open("/config.json", "r");
    if(configFile){
      size_t size = configFile.size();
      std::unique_ptr<char[]> buf(new char[size]);
      configFile.readBytes(buf.get(), size);
      configFile.close();

      DynamicJsonBuffer jsonBuffer;
      JsonObject& jObject = jsonBuffer.parseObject(buf.get());
      if(jObject.success())
      {
        _ssid = jObject["ssid"];
        _pass = jObject["password"];
        //WiFi.mode(WIFI_STA);
        //WiFi.begin(_ssid, _pass);
        Cayenne.begin(username, cpassword, clientID, _ssid, _pass);
        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED) 
        {
          delay(500);
          Serial.print(".");
          digitalWrite(pin_led,!digitalRead(pin_led));
          if ((unsigned long)(millis() - startTime) >= 5000) break;
        }
      }
    }
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    digitalWrite(pin_led,HIGH);
  } 
  else 
  {
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(local_ip, gateway, netmask);
    WiFi.softAP(mySsid); 
    digitalWrite(pin_led,LOW);      
  }
  Serial.println("");
  WiFi.printDiag(Serial);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");
  SPIFFS.begin();
  
  WiFi.hostname(mySsid);

  wifiConnect();

  server.on("/",[](){server.send_P(200,"text/html", webpage);});
  server.on("/toggle",toggleLED);
  server.on("/settings", HTTP_POST, handleSettingsUpdate);
  server.on("/update", handleOTAUpdate);
  server.on("/sendDenon", sendDenonCode);
  server.on("/sendNec", sendNECCode);
  server.on("/sendPolsat", sendPolsatCode);

  ArduinoOTA.setPassword((const char *)"Jacek1");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  pinMode(2,OUTPUT);
  server.begin();
  irsend.begin();
  webSocket.begin();                          // start the websocket server
  webSocket.onEvent(webSocketEvent);
}

void sendDenonCode() {
 for (uint8_t i = 0; i < server.args(); i++) {
    if (server.argName(i) == "code") {
      uint32_t code = strtoul(server.arg(i).c_str(), NULL, 10);
      switch(code)
      {
        case 1 : irsend.sendDenon(0x2A4C02840086, 48);
          break;
        case 2 : irsend.sendDenon(0x2A4C028C008E, 48);
          break;
        case 3 : irsend.sendDenon(0x2A4C028248C8, 48);
          break;
        case 4 : irsend.sendDenon(0x2A4C028A48C0, 48);
          break;
        case 5 : irsend.sendDenon(0x2A4C028F34B9, 48);
          break;
        case 6 : irsend.sendDenon(0x2A4C0280E86A, 48);
          break;
        case 7 : irsend.sendDenon(0x2A4C0288E862, 48);
          break;
        case 8 : irsend.sendDenon(0x2A4C02833CBD, 48);
          break;
        case 9 : irsend.sendDenon(0x2A4C028B3CB5, 48);
          break;
        default:
          irsend.sendDenon(code, 32);
      }
    }
  }
  server.send(200, "application/json", "{\"status\" : \"ok\"}");
}

void sendNECCode() {
 for (uint8_t i = 0; i < server.args(); i++) {
    if (server.argName(i) == "code") {
      uint32_t code = strtoul(server.arg(i).c_str(), NULL, 16);
      irsend.sendNEC(code, 32);
    }
  }
  server.send(200, "application/json", "{\"status\" : \"ok\"}");
}

void sendPolsatCode() {
  for (uint8_t i = 0; i < server.args(); i++) {
    if (server.argName(i) == "code") {
      uint32_t code = strtoul(server.arg(i).c_str(), NULL, 10);
      switch(code)
      {
        case 1 : irsend.sendRaw(power, 77, 38);
          break;
        case 2 : irsend.sendRaw(pplus, 77, 38);
          break;
        case 3 : irsend.sendRaw(pminus, 77, 38);
          break;
        case 4 : irsend.sendRaw(one, 77, 38);
          break;
        case 5 : irsend.sendRaw(two, 77, 38);
          break;
      }
    }
  }
  server.send(200, "application/json", "{\"status\" : \"ok\"}");
}



void handleSettingsUpdate()
{
  String data = server.arg("plain");
  DynamicJsonBuffer jBuffer;
  JsonObject& jObject = jBuffer.parseObject(data);

  File configFile = SPIFFS.open("/config.json", "w");
  jObject.printTo(configFile);  
  configFile.close();
  
  server.send(200, "application/json", "{\"status\" : \"ok\"}");
  delay(500);
  
  wifiConnect();
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
  Cayenne.loop();
  webSocket.loop();
}

void toggleLED()
{
  digitalWrite(pin_led,!digitalRead(pin_led));
  server.send(204,"");
}

CAYENNE_IN(0) {
    toggleLED();
}

CAYENNE_IN(1) { // ON
    if (getValue.asInt() == 1) {
       irsend.sendDenon(0x2A4C02840086, 48);
    }
    else
    {
       irsend.sendDenon(0x2A4C028C008E, 48);
    }
}
CAYENNE_IN(2) { // SURR+
    irsend.sendDenon(0x2A4C0280A82A, 48);
}
CAYENNE_IN(3) { // QS1
    irsend.sendDenon(0x2A4C028248C8, 48);
}
CAYENNE_IN(4) { // QS2
    irsend.sendDenon(0x2A4C028A48C0, 48);
}
CAYENNE_IN(5) { // TUNER
    irsend.sendDenon(0x2A4C028F34B9, 48);
}
CAYENNE_IN(6) { // VOL+
    irsend.sendDenon(0x2A4C0280E86A, 48);
}
CAYENNE_IN(7) { // VOL-
    irsend.sendDenon(0x2A4C0288E862, 48);
}
CAYENNE_IN(8) { // CH+
    irsend.sendDenon(0x2A4C02833CBD, 48);
}
CAYENNE_IN(9) { // CH-
    irsend.sendDenon(0x2A4C028B3CB5, 48);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) { // When a WebSocket message is received
  if (type != WStype_TEXT)
  {
    return;
  }
  int command = (int)payload[0] - '0';

  switch (command)
  {
    case 0: irsend.sendDenon(0x2A4C028C008E, 48); //OFF
      break;
    case 1: irsend.sendDenon(0x2A4C02840086, 48); //ON
      break;
    case 2: irsend.sendDenon(0x2A4C0280A82A, 48); //SURR+
      break;
    case 3: irsend.sendDenon(0x2A4C028248C8, 48); //QS1
      break;
    case 4: irsend.sendDenon(0x2A4C028A48C0, 48); //QS2
      break;
    case 5: irsend.sendDenon(0x2A4C028F34B9, 48); //TUNER
      break;
    case 6: irsend.sendDenon(0x2A4C0280E86A, 48); //VOL+
      break;
    case 7: irsend.sendDenon(0x2A4C0288E862, 48); //VOL-
      break;
    case 8: irsend.sendDenon(0x2A4C0280E86A, 48); //CH+
      break;
    case 9: irsend.sendDenon(0x2A4C0288E862, 48); //CH-
      break;
    // pilot do rzutnika  
    case 17: irsend.sendNEC(0x4CB340BF, 32); //ON // A
      break;
    case 18: irsend.sendNEC(0x4CB3748B, 32); //OFF // B
      break;

    // pilot polsatu
    case 19 : irsend.sendRaw(power, 77, 38);
          break;
    case 20 : irsend.sendRaw(pplus, 77, 38);
          break;
    case 21 : irsend.sendRaw(pminus, 77, 38);
          break;
    case 22 : irsend.sendRaw(one, 77, 38);
          break;
    case 23 : irsend.sendRaw(two, 77, 38);
          break;
  }
  
}

