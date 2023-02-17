#include <ModbusRtu.h>
//https://github.com/poelstra/arduino-multi-button
#include <PinButton.h>


#define UP_RELAY A0
#define DOWN_RELAY A1
#define RS485PIN 2

Modbus slave;

uint16_t modbusData[3];
int address = 0;

PinButton upButton(A2);
PinButton downButton(A3);

void setup() {
    pinMode(7, INPUT_PULLUP);
    pinMode(8, INPUT_PULLUP);
    pinMode(9, INPUT_PULLUP);
    pinMode(10, INPUT_PULLUP);

    pinMode(UP_RELAY, OUTPUT);
    pinMode(DOWN_RELAY, OUTPUT);

    digitalWrite(UP_RELAY, LOW);
    digitalWrite(DOWN_RELAY, LOW);

    bitWrite(address, 0, digitalRead(7));
    bitWrite(address, 1, digitalRead(8));
    bitWrite(address, 2, digitalRead(9));
    bitWrite(address, 3, digitalRead(10));

    address += 10;

    slave = Modbus(address, Serial, RS485PIN);
    slave.begin(9600);
}

int8_t state = 0;
int lastUpState = HIGH;
int lastDownState = HIGH;

int down = HIGH;
int up = HIGH;

bool doubleClickActive = false;
bool lastDoubleClickTime = 0;

uint16_t lastModbusRegister1 = 0;
uint16_t lastModbusRegister2 = 0;
bool modbusChanged = false;

void loop()
{
    upButton.update();
    downButton.update();

    if (doubleClickActive && millis() > (lastDoubleClickTime+(30*1000)))
    {
        up = LOW;
        down = LOW;
        doubleClickActive = false;
    }

    if (upButton.isDoubleClick())
    {
        down = LOW;
        up = HIGH;
        doubleClickActive = true;
        lastDoubleClickTime = millis();
    }

    if (downButton.isDoubleClick())
    {
        up = LOW;
        down = HIGH;
        doubleClickActive = true;
        lastDoubleClickTime = millis();
    }

    if (upButton.isLongClick())
    {
        down = LOW;
        up = HIGH;
        doubleClickActive = false;
    }

    if (upButton.isReleased() && !doubleClickActive)
    {
        up = LOW;
        down = LOW;
    }

    if (downButton.isLongClick())
    {
        up = LOW;
        down = HIGH;
        doubleClickActive = false;
    }

    if (downButton.isReleased() && !doubleClickActive)
    {
        down = LOW;
    }

    if (modbusChanged && modbusData[0] == 1 && modbusData[1] == 0)
    {
        down = LOW;
        up = HIGH;
        doubleClickActive = true;
        lastDoubleClickTime = millis();
    }

    if (modbusChanged && modbusData[0] == 0 && modbusData[1] == 1)
    {
        down = LOW;
        up = HIGH;
        doubleClickActive = true;
        lastDoubleClickTime = millis();
    }

    if ((down == HIGH && up == HIGH) || (down == LOW && up == LOW))
    {
        digitalWrite(UP_RELAY, 0);
        digitalWrite(DOWN_RELAY, 0);
    }

    if (down == LOW && up == HIGH)
    {
        digitalWrite(UP_RELAY, 0);
        delay(10);
        digitalWrite(DOWN_RELAY, 1);
        delay(10);
    }

    if (down == HIGH && up == LOW)
    {
        digitalWrite(DOWN_RELAY, 0);
        delay(10);
        digitalWrite(UP_RELAY, 1);
        delay(10);
    }

    modbusData[2] = slave.getInCnt();

    lastModbusRegister1 = modbusData[0];
    lastModbusRegister2 = modbusData[1];

    state = slave.poll( modbusData, 3 );

    if (modbusData[0] != lastModbusRegister1 || modbusData[1] != lastModbusRegister2)
    {
        modbusChanged = true;
    }
    else
    {
        modbusChanged = false;
    }
}
