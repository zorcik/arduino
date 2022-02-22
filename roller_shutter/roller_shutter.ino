#include <ModbusRtu.h>
//https://github.com/thomasfredericks/Bounce2
#include <Bounce2.h>

#define UP_RELAY A0
#define DOWN_RELAY A1

Modbus slave;

uint16_t modbusData[3];
int address = 0;

Bounce upButton = Bounce();
Bounce downButton = Bounce();

void setup() {
    pinMode(7, INPUT_PULLUP);
    pinMode(8, INPUT_PULLUP);
    pinMode(9, INPUT_PULLUP);
    pinMode(10, INPUT_PULLUP);

    upButton.attach(A2, INPUT);
    upButton.interval(25);
    downButton.attach(A3, INPUT);
    downButton.interval(25);

    bitWrite(address, 0, digitalRead(7));
    bitWrite(address, 1, digitalRead(8));
    bitWrite(address, 2, digitalRead(9));
    bitWrite(address, 3, digitalRead(10));

    address += 10;

    slave = Modbus(ID, Serial, 2);
    slave.begin(9600);
}

int8_t state = 0;
int lastUpState = HIGH;
int lastDownState = HIGH;

int down = HIGH;
int up = HIGH;

void loop()
{
    upButton.update();
    downButton.update();
    if (upButton.changed())
    {
        up = upButton.read();
    }

    if (downButton.changed())
    {
        down = downButton.read();
    }

    if (modbusData[0] == 1 && modbusData[1] == 0)
    {
        digitalWrite(DOWN_RELAY, 0);
        delay(10);
        digitalWrite(UP_RELAY, 1);
        delay(10);
    }

    if (modbusData[0] == 0 && modbusData[1] == 1)
    {
        digitalWrite(UP_RELAY, 0);
        delay(10);
        digitalWrite(DOWN_RELAY, 1);
        delay(10);
    }

    if (modbusData[0] == 0 && modbusData[1] == 0 && down == HIGH && up == HIGH)
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

    state = slave.poll( modbusData, 3 );
}
