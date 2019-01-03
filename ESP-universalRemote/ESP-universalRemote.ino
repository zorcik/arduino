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

ESP8266WebServer server;
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

