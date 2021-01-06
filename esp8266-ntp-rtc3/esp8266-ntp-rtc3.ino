#include <ErriezDS1302.h>
#include <Time.h>
#include <TimeLib.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define DS1302_CLK_PIN      D4
#define DS1302_IO_PIN       D3
#define DS1302_CE_PIN       D2

#define SWITCH              D6

const char *ssid     = "zort";
const char *password = "jacek12345";

WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600);
DS1302 rtc = DS1302(DS1302_CLK_PIN, DS1302_IO_PIN, DS1302_CE_PIN);

int rememberedDay = 0;

void setup() {
  pinMode(D5, OUTPUT);
  pinMode(D6, INPUT_PULLUP);
  pinMode(D7, OUTPUT); // zielona ON
  pinMode(D8, OUTPUT); // czerwona OFF
  pinMode(D0, OUTPUT); // niebieska AUTO

  Serial.begin(115200);
  rtc.begin();

  timeClient.begin();

  setRTCTime();

  digitalWrite(D5, LOW);
  digitalWrite(D7, LOW);
  digitalWrite(D8, LOW);
  digitalWrite(D0, HIGH);

  delay(1000);
}

void setRTCTime()
{
  Serial.println("Connecting");
  WiFi.begin(ssid, password);

  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }

  timeClient.update();
  time_t t = timeClient.getEpochTime();

  DS1302_DateTime dt;

  dt.second = second(t);
  dt.minute = minute(t);
  dt.hour = hour(t);
  dt.dayWeek = timeClient.getDay(); // 1 = Monday
  dt.dayMonth = day(t);
  dt.month = month(t);
  dt.year = year(t);

  rtc.setDateTime(&dt);
  rtc.halt(false);

  WiFi.disconnect();
  Serial.println("Disconnected");
}

int stat = 0; // AUTO
// 1 - ON
// 2 - OFF

void loop() {

  DS1302_DateTime dt;
  char buf[32];

  if (digitalRead(SWITCH) == LOW)
  {
    stat += 1;
    if (stat == 3)
    {
      stat = 0;
    }
        
    while (digitalRead(SWITCH) == LOW)
    {
      delay(50);
    }
  }

  if (stat == 1)
  {
    digitalWrite(D5, HIGH);
    digitalWrite(D7, HIGH);
    digitalWrite(D8, LOW);
    digitalWrite(D0, LOW);
    Serial.println("ON");
  }
  else if (stat == 2)
  {
    digitalWrite(D5, LOW);
    digitalWrite(D7, LOW);
    digitalWrite(D8, HIGH);
    digitalWrite(D0, LOW);
    Serial.println("OFF");
  }
  else
  {
    if (!rtc.getDateTime(&dt)) {
      Serial.println(F("Error: DS1302 read failed"));
      setRTCTime();
    } else {
      Serial.println("AUTO");
      digitalWrite(D7, LOW);
      digitalWrite(D8, LOW);
      digitalWrite(D0, HIGH);
      if (dt.dayWeek != 0 && dt.dayWeek != 6) // bez niedzieli i soboty
      {
        if (dt.hour > 8 && dt.hour < 18)
        {
          digitalWrite(D5, HIGH);
        }
        else
        {
          digitalWrite(D5, LOW);
        }
      }
      else
      {
        digitalWrite(D5, LOW);
      }

      if (dt.second == 0)
      {
        setRTCTime();
        delay(2000);
      }

      snprintf(buf, sizeof(buf), "%d %02d-%02d-%d %d:%02d:%02d",
               dt.dayWeek, dt.dayMonth, dt.month, dt.year, dt.hour, dt.minute, dt.second);
      //Serial.println(buf);
      //delay(1000);
    }
    
  }
}
