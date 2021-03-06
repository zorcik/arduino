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

/*!
 * \brief DS3231 high precision RTC serial terminal example for Arduino
 * \details
 *      Sources:
 *          https://github.com/Erriez/ErriezDS3231
 *          https://github.com/Erriez/ErriezSerialTerminal
 *      Documentation:
 *          https://erriez.github.io/ErriezDS3231
 *          https://erriez.github.io/ErriezSerialTerminal
 */

#include <Wire.h>

#include <ErriezDS3231_debug.h>
#include <ErriezSerialTerminal.h>

// Newline character '\r' or '\n'
char newlineChar = '\n';
// Separator character between commands and arguments
char delimiterChar = ' ';

// Create serial terminal object
SerialTerminal term(newlineChar, delimiterChar);

// Create DS3231 RTC object
static DS3231Debug rtc;

// Define days of the week in flash
const char day_1[] PROGMEM = "Monday";
const char day_2[] PROGMEM = "Tuesday";
const char day_3[] PROGMEM = "Wednesday";
const char day_4[] PROGMEM = "Thursday";
const char day_5[] PROGMEM = "Friday";
const char day_6[] PROGMEM = "Saturday";
const char day_7[] PROGMEM = "Sunday";

const char* const dayWeekTable[] PROGMEM = {
        day_1, day_2, day_3, day_4, day_5, day_6, day_7
};

// Define months in flash
const char month_1[] PROGMEM = "January";
const char month_2[] PROGMEM = "February";
const char month_3[] PROGMEM = "March";
const char month_4[] PROGMEM = "April";
const char month_5[] PROGMEM = "May";
const char month_6[] PROGMEM = "June";
const char month_7[] PROGMEM = "July";
const char month_8[] PROGMEM = "August";
const char month_9[] PROGMEM = "September";
const char month_10[] PROGMEM = "October";
const char month_11[] PROGMEM = "November";
const char month_12[] PROGMEM = "December";

const char* const monthTable[] PROGMEM = {
        month_1, month_2, month_3, month_4, month_5, month_6,
        month_7, month_8, month_9, month_10, month_11, month_12
};

static bool periodicPrintEnable = false;
static bool outputClockEnable = false;

// Function prototypes
static void printError();
static bool getArgumentInt(int *argumentValue);
static void cmdUnknown(const char *command);
static void cmdHelp();
static void cmdTogglePrint();
static void cmdPrintDateTime();
static void cmdPrintDate();
static void cmdPrintTime();
static void cmdPrintEpoch();
static void cmdSetDateTime();
static void cmdToggle32kHzOutputClockPin();
static void cmdSquareWaveOut();
static void cmdOscillatorStop();
static void cmdOscillatorStart();
static void cmdStartTemperatureConversion();
static void cmdPrintTemperature();
static void cmdPrintDiagnostics();
static void cmdDumpRegisters();
static void cmdWriteRegister();
static void cmdReadRegister();


void setup()
{
    // Initialize serial port
    Serial.begin(115200);
    while (!Serial) {
        ;
    }

    Serial.println(F("DS3231 RTC terminal example\n"));
    Serial.println(F("Type 'help' to display usage."));

    // Set default handler for unknown commands
    term.setDefaultHandler(cmdUnknown);

    // Add command callback handlers
    term.addCommand("?", cmdHelp);
    term.addCommand("help", cmdHelp);

    term.addCommand("print", cmdTogglePrint);
    term.addCommand("dt", cmdPrintDateTime);
    term.addCommand("date", cmdPrintDate);
    term.addCommand("time", cmdPrintTime);
    term.addCommand("epoch", cmdPrintEpoch);
    term.addCommand("set", cmdSetDateTime);

    term.addCommand("tconv", cmdStartTemperatureConversion);
    term.addCommand("temp", cmdPrintTemperature);

    term.addCommand("clkout", cmdToggle32kHzOutputClockPin);
    term.addCommand("sqwout", cmdSquareWaveOut);
    term.addCommand("stop", cmdOscillatorStop);
    term.addCommand("start", cmdOscillatorStart);

    term.addCommand("diag", cmdPrintDiagnostics);
    term.addCommand("dump", cmdDumpRegisters);
    term.addCommand("w", cmdWriteRegister);
    term.addCommand("r", cmdReadRegister);

    // Initialize TWI
    Wire.begin();
    Wire.setClock(400000);

    // Initialize RTC
    while (rtc.begin()) {
        Serial.println(F("Warning: Could not detect DS3231 RTC"));
        delay(3000);
    }

    // Check oscillator status
    if (rtc.isOscillatorStopped()) {
        Serial.println(F("Warning: DS3231 RTC oscillator stopped. Program new date/time."));
    }

    // Set output clock
    rtc.outputClockPinEnable(outputClockEnable);

    // Set square wave out
    rtc.setSquareWave(SquareWaveDisable);
}

void loop()
{
    static unsigned long printTimestamp;

    // Read from serial port and handle command callbacks
    term.readSerial();

    // Prints every second
    if (periodicPrintEnable) {
        if ((millis() - printTimestamp) > 1000) {
            printTimestamp = millis();

            // Print epoch/date/time/temperature
            cmdPrintEpoch();
            cmdPrintDateTime();
            cmdPrintTemperature();
            Serial.println();
        }
    }
}

static bool getArgumentInt(int *argumentValue)
{
    int val;
    char *arg;

    // Get next decimal or hexadecimal argument string
    arg = term.getNext();
    if (strncmp(arg, "0x", 2) == 0) {
        if (sscanf(arg, "0x%x", &val) != 1) {
            return false;
        }
    } else {
        if (sscanf(arg, "%d", &val) != 1) {
            return false;
        }
    }

    *argumentValue = val;

    return true;
}

static void cmdUnknown(const char *command)
{
    // Print unknown command
    Serial.print(F("Unknown command: "));
    Serial.println(command);
}

static void cmdHelp()
{
    // Print usage
    Serial.println(F("DS3231 RTC terminal usage:"));
    Serial.println(F("  help or ?          Print this usage"));
    Serial.println();
    Serial.println(F(" Date/time functions:"));
    Serial.println(F("  print              Toggle periodic prints epoch/date/time/temp"));
    Serial.println(F("  dt                 Print date short and time"));
    Serial.println(F("  date               Print date long"));
    Serial.println(F("  time               Print time"));
    Serial.println(F("  set date <w d-m-Y> Set day week and date"));
    Serial.println(F("  set time <H:M:S>   Set time"));
    Serial.println();
    Serial.println(F(" Temperature functions:"));
    Serial.println(F("  tconv              Start temperature conversion"));
    Serial.println(F("  temp               Print temperature"));
    Serial.println();
    Serial.println(F(" Clock functions:"));
    Serial.println(F("  clkout             Toggle 32kHz output clock enable"));
    Serial.println(F("  sqwout <0|1|1024|4096|8192>"));
    Serial.println(F("                     SQW output pin in Hz"));
    Serial.println(F("  stop               Stop RTC oscillator on V-BAT"));
    Serial.println(F("  start              Start RTC oscillator"));
    Serial.println();
    Serial.println(F(" Diagnostics:"));
    Serial.println(F("  diag               Print diagnostics"));
    Serial.println(F("  dump               Dump all registers"));
    Serial.println(F("  w [reg] [val]      Write register"));
    Serial.println(F("  r [reg]            Read register"));
}

static void cmdTogglePrint()
{
    // Toggle print on/off
    periodicPrintEnable ^= true;

    Serial.print(F("Periodic prints: "));
    Serial.println(periodicPrintEnable ? F("Enabled") : F("Disabled"));
    Serial.println();
}

static void cmdPrintDate()
{
    DS3231_DateTime dt;
    char buf[10];

    Serial.print(F("Date: "));

    // Read date/time from RTC
    if (rtc.getDateTime(&dt)) {
        Serial.println(F("Error: Read date/time failed"));
        return;
    }

    // Print day of the week, day month, month and year
    strncpy_P(buf, (char *)pgm_read_dword(&(dayWeekTable[dt.dayWeek - 1])), sizeof(buf));
    Serial.print(buf);
    Serial.print(F(" "));
    Serial.print(dt.dayMonth);
    Serial.print(F(" "));
    strncpy_P(buf, (char *)pgm_read_dword(&(monthTable[dt.month - 1])), sizeof(buf));
    Serial.print(buf);
    Serial.print(F(" "));
    Serial.println(dt.year);
}

static void cmdPrintTime()
{
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    char buf[9];

    Serial.print(F("Time: "));

    // Read time from RTC
    if (rtc.getTime(&hour, &minute, &second)) {
        Serial.println(F("Error: Read time failed"));
        return;
    }

    // Print time
    snprintf(buf, sizeof(buf), "%d:%02d:%02d", hour, minute, second);
    Serial.println(buf);
}

static void cmdPrintDateTime()
{
    DS3231_DateTime dt;
    char buf[11];

    // Read date/time from RTC
    if (rtc.getDateTime(&dt)) {
        Serial.println(F("Error: Read date/time failed"));
        return;
    }

    Serial.print(F("Date: "));
    snprintf(buf, sizeof(buf), "%d-%d-%d", dt.dayMonth, dt.month, dt.year);
    Serial.println(buf);
    Serial.print(F("Time: "));
    snprintf(buf, sizeof(buf), "%d:%02d:%02d", dt.hour, dt.minute, dt.second);
    Serial.println(buf);
}

static void cmdPrintEpoch()
{
    DS3231_DateTime dt;
    char buf[11];
#if defined(ESP32)
    long unsigned int epoch;
#else
    uint32_t epoch;
#endif

    Serial.print(F("Epoch: "));

    // Read date/time
    if (rtc.getDateTime(&dt)) {
        Serial.println(F("Error: Read date/time failed"));
        return;
    }

    // Print 32-bit epoch time
    epoch = rtc.getEpochTime(&dt);
    snprintf(buf, sizeof(buf), "%lu", epoch);
    Serial.println(buf);
}

static void cmdSetDateTime()
{
    DS3231_DateTime dt;
    int second;
    int minute;
    int hour;
    int dayWeek;
    int dayMonth;
    int month;
    int year;
    char *arg;

    arg = term.getNext();
    if (strncmp(arg, "date", 4) == 0) {
        arg = term.getRemaining();
        if (arg != NULL) {
            if (sscanf(arg, "%d %d-%d-%d", &dayWeek, &dayMonth, &month, &year) == 4) {
                rtc.getDateTime(&dt);
                dt.dayWeek = dayWeek;
                dt.dayMonth = dayMonth;
                dt.month = month;
                dt.year = year;
                rtc.setDateTime(&dt);
                Serial.print(F("Set date: "));
                Serial.println(arg);
            } else {
                Serial.println(F("Incorrect time format"));
            }
        }
    } else if (strncmp(arg, "time", 4) == 0) {
        arg = term.getRemaining();
        if (arg != NULL) {
            if (sscanf(arg, "%d:%d:%d", &hour, &minute, &second) == 3) {
                rtc.setTime(hour, minute, second);
                Serial.print(F("Set time: "));
                Serial.println(arg);
            } else {
                Serial.println(F("Incorrect time format"));
            }
        }
    } else {
        Serial.println(F("Argument date/time missing"));
    }
}

static void cmdToggle32kHzOutputClockPin()
{
    // Toggle 32kHz output clock pin
    outputClockEnable ^= true;

    Serial.print(F("32kHz output pin: "));
    Serial.println(outputClockEnable ? F("Enable") : F("Disable"));

    // Enable or disable 32kHz output clock pin
    rtc.outputClockPinEnable(outputClockEnable);
}

static void cmdSquareWaveOut()
{
    int argValue;

    if (getArgumentInt(&argValue) != true) {
        Serial.println(F("Incorrect value"));
        return;
    }

    Serial.print(F("Set SQW clock out: "));
    switch (argValue) {
        case 0:
            Serial.println(F("Disable"));
            rtc.setSquareWave(SquareWaveDisable);
            break;
        case 1:
            Serial.println(F("1Hz"));
            rtc.setSquareWave(SquareWave1Hz);
            break;
        case 1024:
            Serial.println(F("1024Hz"));
            rtc.setSquareWave(SquareWave1024Hz);
            break;
        case 4096:
            Serial.println(F("4096Hz"));
            rtc.setSquareWave(SquareWave4096Hz);
            break;
        case 8192:
            Serial.println(F("8192Hz"));
            rtc.setSquareWave(SquareWave8192Hz);
            break;
        default:
            Serial.println(F("Incorrect value"));
    }
}

static void cmdOscillatorStop()
{
    Serial.println(F("Stop oscillator when running on V-BAT"));

    rtc.oscillatorEnable(false);
}

static void cmdOscillatorStart()
{
    Serial.println(F("Start oscillator"));

    rtc.oscillatorEnable(true);
}

static void cmdStartTemperatureConversion()
{
    Serial.println(F("Forced temperature conversion"));

    // Without this call, it takes 64 seconds before the temperature is updated.
    rtc.startTemperatureConversion();
}

static void cmdPrintTemperature()
{
    int8_t temperature;
    uint8_t fraction;

    Serial.print(F("Temperature: "));

    // Read temperature
    rtc.getTemperature(&temperature, &fraction);
    Serial.print(temperature);
    Serial.print(F("."));
    Serial.print(fraction);
    Serial.println(F("C"));
}

static void cmdPrintDiagnostics()
{
    rtc.printDiagnostics(&Serial);
}

static void cmdDumpRegisters()
{
    rtc.dumpRegisters(&Serial, true);
}

static void cmdWriteRegister()
{
    int reg;
    int val;
    char buf[13];

    if (getArgumentInt(&reg) != true) {
        Serial.println(F("Error: Incorrect register format"));
        return;
    }

    if (getArgumentInt(&val) != true) {
        Serial.println(F("Error: Incorrect value format"));
        return;
    }

    rtc.writeRegister(reg, val);
    Serial.print(F("Write reg "));
    snprintf(buf, sizeof(buf), "0x%02x: 0x%02x", reg, val);
    Serial.println(buf);
}

static void cmdReadRegister()
{
    int reg;
    char buf[13];

    if (getArgumentInt(&reg) != true) {
        Serial.println(F("Error: Incorrect register format"));
        return;
    }

    Serial.print(F("Read reg "));
    snprintf(buf, sizeof(buf), "0x%02x: 0x%02x", reg, rtc.readRegister(reg));
    Serial.println(buf);
}

static void printDateTime()
{
    DS3231_DateTime dt;
    char buf[32];

    // Read date/time from RTC
    if (rtc.getDateTime(&dt)) {
        Serial.println(F("Error: Read date/time failed"));
        return;
    }

    snprintf(buf, sizeof(buf), "%d %02d-%02d-%d %d:%02d:%02d",
             dt.dayWeek, dt.dayMonth, dt.month, dt.year, dt.hour, dt.minute, dt.second);
    Serial.println(buf);
}
