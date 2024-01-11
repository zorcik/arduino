#include <ErriezDS1302.h>
#include <Time.h>
#include <TimeLib.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define DS1302_CLK_PIN      D4
#define DS1302_IO_PIN       D3
#define DS1302_CE_PIN       D2

#define RELAY               D5
#define SWITCH              D6
#define BLUE_LED            D0
#define GREEN_LED           D7
#define RED_LED             D8

const char *ssid     = "mserwis-sq";
const char *password = "HASŁO";

WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600);
DS1302 rtc = DS1302(DS1302_CLK_PIN, DS1302_IO_PIN, DS1302_CE_PIN);

void setup() {
  pinMode(RELAY, OUTPUT);
  pinMode(SWITCH, INPUT_PULLUP);
  pinMode(GREEN_LED, OUTPUT); // zielona ON
  pinMode(RED_LED, OUTPUT); // czerwona OFF
  pinMode(BLUE_LED, OUTPUT); // niebieska AUTO

  Serial.begin(115200);
  rtc.begin();

  timeClient.begin();

  setRTCTime();

  digitalWrite(RELAY, LOW);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
  digitalWrite(BLUE_LED, HIGH);

  delay(1000);
}

void setRTCTime()
{
  char buf[32];
  Serial.println("Connecting");
  WiFi.begin(ssid, password);

  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }

  Serial.println ( "." );

  timeClient.update();
  time_t t = timeClient.getEpochTime();

  if (t > 1579780799) // czy na pewno pobraliśmy?
  {
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
    snprintf(buf, sizeof(buf), "%d %02d-%02d-%d %d:%02d:%02d",
                 dt.dayWeek, dt.dayMonth, dt.month, dt.year, dt.hour, dt.minute, dt.second);
    Serial.println(buf);
  }
  else
  {
    Serial.println("Error getting current date");
  }

  WiFi.disconnect();
  Serial.println("Disconnected");
}

int stat = 0; // AUTO
// 1 - ON
// 2 - OFF

int daylightSavingTimeOffset = 0;

void loop() {

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
    digitalWrite(RELAY, HIGH);
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
    Serial.println("ON");
  }
  else if (stat == 2)
  {
    digitalWrite(RELAY, LOW);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, HIGH);
    digitalWrite(BLUE_LED, LOW);
    Serial.println("OFF");
  }
  else
  {
    DS1302_DateTime dt;

    if (!rtc.getDateTime(&dt)) {
      Serial.println(F("Error: DS1302 read failed"));
      setRTCTime();
    } else {
      Serial.println("AUTO");
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(RED_LED, LOW);
      digitalWrite(BLUE_LED, HIGH);

      //daylightSavingTimeOffset
      if (dt.month == 11 || dt.month == 12 || dt.month == 1 || dt.month == 2 || (dt.month == 10 && dt.dayMonth > 25) || (dt.month == 3 && dt.dayMonth < 25))
      {
        daylightSavingTimeOffset = 1;
      }
      else
      {
        daylightSavingTimeOffset = 0;
      }


      if (dt.dayWeek != 0 && dt.dayWeek != 6) // bez niedzieli i soboty
      {
        if ((dt.hour + daylightSavingTimeOffset) > 8 && (dt.hour + daylightSavingTimeOffset) < 17)
        {
          digitalWrite(RELAY, HIGH);
        }
        else
        {
          digitalWrite(RELAY, LOW);
        }
      }
      else
      {
        digitalWrite(RELAY, LOW);
      }

      if (dt.hour == 4 && dt.minute == 15 && dt.second == 0)
      {
        setRTCTime();
        delay(2000);
      }

    }

  }
}
