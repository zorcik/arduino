#include <ModbusRtu.h>

#define In1 A0
#define In2 A1
#define In3 A2
#define In4 A3
#define In5 A4
#define In6 A5
#define In7 2
#define In8 3

#define OUT1 4
#define OUT2 5
#define OUT3 6
#define OUT4 7
#define OUT5 8
#define OUT6 9
#define OUT7 10
#define OUT8 A6

Modbus slave;

uint16_t modbusData[9];
int address = 50;
uint8_t outPins = 0;

uint8_t state = 0;
uint8_t lastState = 0;

void setup()
{
    slave = Modbus(address, 0, 0);
    Serial.begin(9600);
    slave.start();

    pinMode(OUT1, OUTPUT);
    pinMode(OUT2, OUTPUT);
    pinMode(OUT3, OUTPUT);
    pinMode(OUT4, OUTPUT);
    pinMode(OUT5, OUTPUT);
    pinMode(OUT6, OUTPUT);
    pinMode(OUT7, OUTPUT);
    pinMode(OUT8, OUTPUT);

    pinMode(In1, INPUT);
    pinMode(In2, INPUT);
    pinMode(In3, INPUT);
    pinMode(In4, INPUT);
    pinMode(In5, INPUT);
    pinMode(In6, INPUT);
    pinMode(In7, INPUT);
    pinMode(In8, INPUT);
}

uint8_t read()
{
    uint8_t temp = 0;
    bitWrite(temp, 0, digitalRead(In1));
    bitWrite(temp, 1, digitalRead(In2));
    bitWrite(temp, 2, digitalRead(In3));
    bitWrite(temp, 3, digitalRead(In4));
    bitWrite(temp, 4, digitalRead(In5));
    bitWrite(temp, 5, digitalRead(In6));
    bitWrite(temp, 6, digitalRead(In7));
    bitWrite(temp, 7, digitalRead(In8));
    return temp;
}

void write(uint8_t data)
{
    digitalWrite(0, bitRead(data, 0));
    digitalWrite(1, bitRead(data, 1));
    digitalWrite(2, bitRead(data, 2));
    digitalWrite(3, bitRead(data, 3));
    digitalWrite(4, bitRead(data, 4));
    digitalWrite(5, bitRead(data, 5));
    digitalWrite(6, bitRead(data, 6));
    digitalWrite(7, bitRead(data, 7));
}

void loop()
{
    state = read();

    if (state != lastState)
    {
        delay(20); //debounce
    }

    state = read();

    if (state != lastState)
    {
        lastState = state;
        for (int i=0; i<8; i++)
        {
            if (bitRead(state, i) == LOW)
            {
                bitWrite(outPins, i, !bitRead(outPins, i));
            }
        }

        write(outPins);
    }

    for (int i=0; i<8; i++)
    {
        modbusData[i] = !bitRead(outPins, i);
    }
    modbusData[8] = slave.getInCnt();
    slave.poll(modbusData, 9);
}
