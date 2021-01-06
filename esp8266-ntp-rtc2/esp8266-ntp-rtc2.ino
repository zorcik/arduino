// działa dla DS3231


#include <Wire.h>
#include <ErriezDS3231.h>
#include <Time.h>
#include <TimeLib.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

const char *ssid     = "zort";
const char *password = "jacek12345";

WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600);
static DS3231 rtc;

int rememberedDay = 0;



void setup(){
  pinMode(D5, OUTPUT);
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }

  Wire.begin();
  Wire.setClock(400000);
    
  // Initialize RTC
  while (rtc.begin()) {
      // Error: Could not detect DS3231 RTC, retry after some time
      Serial.println("Nie znaleziono RTC");
      delay(3000);
  }

  timeClient.begin();

  setRTCTime();

  digitalWrite(D5, LOW);
  
  delay(1000);
}

void setRTCTime()
{
  timeClient.update();
  time_t t = timeClient.getEpochTime();

  static DS3231_DateTime dt;
    
  dt.second = second(t);
  dt.minute = minute(t);
  dt.hour = hour(t);
  dt.dayWeek = timeClient.getDay(); // 1 = Monday
  dt.dayMonth = day(t);
  dt.month = month(t);
  dt.year = year(t);
  
  rtc.setDateTime(&dt);
}

void loop() {

  DS3231_DateTime dt;
  char buf[32];

  if (rtc.getDateTime(&dt)) {
    Serial.println(F("Error: DS1302 read failed"));
    setRTCTime();
  } else {

    if (dt.dayWeek != 0 && dt.dayWeek != 6) // bez niedzieli i soboty
    {
      if (dt.hour > 8 && dt.hour < 18)
      {
          digitalWrite(D5, HIGH);
      }
    }
      snprintf(buf, sizeof(buf), "%d %02d-%02d-%d %d:%02d:%02d",
               dt.dayWeek, dt.dayMonth, dt.month, dt.year, dt.hour, dt.minute, dt.second);
      Serial.println(buf);
  }

  digitalWrite(D5, LOW);
  delay(1000);
  digitalWrite(D5, HIGH);
}
