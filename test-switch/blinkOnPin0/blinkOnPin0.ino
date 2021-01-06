/*
 Blink led on PIN0
 by Mischianti Renzo <http://www.mischianti.org>

 https://www.mischianti.org/2019/01/02/pcf8575-i2c-digital-i-o-expander-fast-easy-usage/
*/

#include "Arduino.h"
#include "PCF8575.h"

// Set i2c address
PCF8575 pcf8575(0x27);

void setup()
{
	Serial.begin(115200);

	// Set pinMode to OUTPUT
pcf8575.pinMode(P0, INPUT);
pcf8575.pinMode(P1, INPUT);
pcf8575.pinMode(P2, INPUT);
pcf8575.pinMode(P3, INPUT);
pcf8575.pinMode(P4, INPUT);
pcf8575.pinMode(P5, INPUT);
pcf8575.pinMode(P6, INPUT);
pcf8575.pinMode(P7, INPUT);
pcf8575.pinMode(P8, INPUT);
pcf8575.pinMode(P9, INPUT);
pcf8575.pinMode(P10, INPUT);
pcf8575.pinMode(P11, INPUT);
pcf8575.pinMode(P12, INPUT);
pcf8575.pinMode(P13, INPUT);
pcf8575.pinMode(P14, INPUT);
pcf8575.pinMode(P15, INPUT);

	pcf8575.begin();
}

void loop()
{
	pcf8575.digitalWrite(P0, HIGH);
	delay(1000);
	pcf8575.digitalWrite(P0, LOW);
	delay(1000);
}
