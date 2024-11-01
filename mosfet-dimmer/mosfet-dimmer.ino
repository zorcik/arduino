#include <ModbusSerial.h>
#include <Encoder.h>
#include <Bounce2.h>

#define ONOFF A1
#define ENC1 2
#define ENC2 3
#define OUT 10

int address = 90;
ModbusSerial mb(Serial, address, -1);
uint8_t POSITIONS[] = {0, 1, 2, 3, 4, 5, 7, 9, 12, 15, 18, 22, 27, 32, 38, 44, 51, 58, 67, 76, 86, 96, 108, 120, 134, 148, 163, 180, 197, 216, 235, 255}; //32
#define MAX 128

int position = 0;
int arrayPosition = 0;
bool status = 0;
bool newStatus = 0;
bool tempStatus = 1;
bool changed = false;
bool changed2 = false;
Bounce onButton = Bounce();

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
    mb.addHreg(0, position);
    mb.addHreg(1, arrayPosition);
    onButton.attach(ONOFF, INPUT_PULLUP);
    onButton.interval(25);
}

void loop()
{
    mb.task();

    long newLeft;
    newLeft = knob.read();

    if (newLeft != position)
    {
        position = newLeft;
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
        mb.setHreg(0, position);
    }

    int newPosition = mb.hreg(0);
    if (newPosition != position)
    {
        position = newPosition;
        if (position > MAX)
        {
            position = MAX;
        }

        if (position < 0)
        {
            position = 0;
        }
        knob.write(position);
    }

    onButton.update();

    if (onButton.changed())
    {
        tempStatus = onButton.read();
    }    

    if (tempStatus == LOW)
    {
       status = !status;
       mb.setCoil(0, status);
       tempStatus = HIGH;
    }
    
    int newStatus = mb.coil(0);
    if (newStatus != status)
    {
      status = newStatus;
    } 

    if (!status)
    {
        analogWrite(OUT, 0);
    }
    else
    {
        arrayPosition = (int)position/4;
        if (arrayPosition > 31)
        {
          arrayPosition = 31;
        }
        mb.setHreg(1, arrayPosition);
        analogWrite(OUT, POSITIONS[arrayPosition]);
    }

    
    
    changed = false;
}
