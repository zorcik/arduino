#include <ModbusRtu.h>
#include "PCF8574.h"
#include <Wire.h>

Modbus slave;

uint16_t modbusData[2];
int address = 50;
uint8_t outPins = 0b11111111;

PCF8574 out;
PCF8574 in;

uint8_t state = 0;
uint8_t lastState = 0;

void setup()
{
//    slave = Modbus(ID, Serial, 3);
//    slave.begin(9600);

    out.begin(0x20, false);
    in.begin(0x27, false);
    out.write(255);

    in.pinMode(0, INPUT);
    in.pinMode(1, INPUT);
    in.pinMode(2, INPUT);
    in.pinMode(3, INPUT);
    in.pinMode(4, INPUT);
    in.pinMode(5, INPUT);
    in.pinMode(6, INPUT);
    in.pinMode(7, INPUT);

    in.write(255);

    //in.enableInterrupt(2, onInterrupt);
}

void onInterrupt()
{
    state = in.read();
}

void loop()
{
    //state = in.read();
    if (state != lastState)
    {
        lastState = state;
        for (int i = 0; i < 8; i++)
        {
            if (bitRead(state, i) == LOW)
            {
                bitWrite(outPins, i, abs(bitRead(outPins, i) - 1));
            }
        }

      //  modbusData[0] = outPins;
        out.write(outPins);
    }

    state = in.read();
/*
    if (modbusData[0] != outPins)
    {
        outPins = modbusData[0];
        out.write(outPins);
    }

    modbusData[1] = slave.getInCnt();
    state = slave.poll(modbusData, 2); */
}
