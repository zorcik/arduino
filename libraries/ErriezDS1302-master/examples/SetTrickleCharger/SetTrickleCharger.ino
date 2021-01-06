/*
 * MIT License
 *
 * Copyright (c) 2018 Erriez
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* DS1302 RTC example for Arduino
 *
 * Required library:
 *   https://github.com/Erriez/ErriezDS1302
 */

#include <ErriezDS1302.h>

// Connect DS1302 data pin to Arduino DIGITAL pin
#if defined(ARDUINO_ARCH_AVR)
#define DS1302_CLK_PIN      2
#define DS1302_IO_PIN       3
#define DS1302_CE_PIN       4
#elif defined(ARDUINO_ARCH_ESP8266)
// Swap D2 and D4 pins for the ESP8266, because pin D2 is high during a
// power-on / MCU reset / and flashing. This corrupts RTC registers.
#define DS1302_CLK_PIN      D4 // Pin is high during power-on / reset / flashing
#define DS1302_IO_PIN       D3
#define DS1302_CE_PIN       D2
#elif defined(ARDUINO_ARCH_ESP32)
#define DS1302_CLK_PIN      0
#define DS1302_IO_PIN       4
#define DS1302_CE_PIN       5
#else
#error #error "May work, but not tested on this target"
#endif

// Create DS1302 RTC object
DS1302 rtc = DS1302(DS1302_CLK_PIN, DS1302_IO_PIN, DS1302_CE_PIN);



void setup()
{
    DS1302_DateTime dt;

    // Initialize serial port
    Serial.begin(115200);
    while (!Serial) {
        ;
    }
    Serial.println(F("DS1302 RTC trickle charger example\n"));

    // Initialize RTC
    rtc.begin();

    // Make clock and RAM registers writable
    rtc.writeProtect(false);

    // Check RTC write protect state
    if (rtc.isWriteProtected()) {
        Serial.println(F("Error: DS1302 not found"));
        while (1) {
            ;
        }
    }

    // Read TCS register
    Serial.print(F("TCS reg: 0x"));
    Serial.println(rtc.readClockRegister(DS1302_REG_TC), HEX);

    // Write TCS register
    // Please refer to the datasheet to set charge current
    rtc.writeClockRegister(DS1302_REG_TC, DS1302_TCS_DISABLE);

    // Read TCS register
    Serial.print(F("TCS reg: 0x"));
    Serial.println(rtc.readClockRegister(DS1302_REG_TC), HEX);
}

void loop()
{

}
