#include <ModbusRtu.h>
#include "PCF8574.h"
#include <Wire.h>

Modbus slave;

uint16_t modbusData[9];
int address = 50;
uint8_t outPins = 0b11111111;

PCF8574 out;
PCF8574 in;

uint8_t state = 0;
uint8_t lastState = 0;

void setup()
{
    slave = Modbus(address, Serial, 3);
    slave.begin(9600);

    out.begin(0x20);
    in.begin(0x27);

    out.pinMode(0, OUTPUT);
    out.pinMode(1, OUTPUT);
    out.pinMode(2, OUTPUT);
    out.pinMode(3, OUTPUT);
    out.pinMode(4, OUTPUT);
    out.pinMode(5, OUTPUT);
    out.pinMode(6, OUTPUT);
    out.pinMode(7, OUTPUT);
    out.clear();

    in.pinMode(0, INPUT);
    in.pinMode(1, INPUT);
    in.pinMode(2, INPUT);
    in.pinMode(3, INPUT);
    in.pinMode(4, INPUT);
    in.pinMode(5, INPUT);
    in.pinMode(6, INPUT);
    in.pinMode(7, INPUT);
}

void loop()
{
    state = in.read();

    if (state != lastState)
    {
        lastState = state;
        for (int i = 0; i < 8; i++)
        {
            bit t = bitRead(state, i);
            if (t == LOW)
            {
                bitWrite(outPins, i, abs(bitRead(outPins, i) - 1));
            }
            modbusData[i] = bitRead(outPins, i);
        }
    }

    for (int i=0; i<8; i++)
    {
        bitWrite(outPins, i, modbusData[i]);
    }

    out.write(outPins);

    modbusData[8] = slave.getInCnt();
    slave.poll(modbusData, 9);
}
