#include <ModbusSerial.h>
#include <Encoder.h>

#define ONOFF A1
#define ENC1 2
#define ENC2 3
#define OUT 10

int address = 90;
ModbusSerial mb(Serial, address, -1);
uint8_t POSITIONS[] = {0, 1, 2, 3, 4, 5, 7, 9, 12, 15, 18, 22, 27, 32, 38, 44, 51, 58, 67, 76, 86, 96, 108, 120, 134, 148, 163, 180, 197, 216, 235, 255}; //32
#define MAX 32

int position = 0;
bool status = 0;
bool changed = false;
bool changed2 = false;

Encoder knob(ENC1, ENC2);

void setup()
{
    Serial.begin(9600, MB_PARITY_NONE);
    mb.config(9600);
    mb.setAdditionalServerData("LAMP"); // for Report Server ID function (0x11)
    pinMode(OUT, OUTPUT);
    digitalWrite(OUT, LOW);

    pinMode(ONOFF, INPUT_PULLUP);
    mb.addCoil(0, status);
    mb.addIreg(0, position);
}

void loop()
{
    mb.task();

    long newLeft;
    newLeft = knob.read();

    if (newLeft != position)
    {
        position = newLeft;
        changed = true;
    }

    int newPosition = mb.ireg(0);
    if (newPosition != position && !changed)
    {
        changed = true;
        position = newPosition;
    }

    if (position > MAX)
    {
        position = MAX;
        knob.write(MAX);
    }

    if (position < 0)
    {
        position = 0;
        knob.write(0);
    }

    if (ONOFF == LOW && !changed2)
    {
        delay(20);
        if (ONOFF == LOW)
        {
            changed2 = true;
            status != status;
        }
    }

    if (ONOFF == HIGH && changed2)
    {
        delay(20);
        if (ONOFF == HIGH)
        {
            changed2 = false;
        }
    }

    if (!status)
    {
        analogWrite(OUT, 0);
    }
    else
    {
        analogWrite(OUT, POSITIONS[position]);
    }

    mb.setIreg(0, position);
}
