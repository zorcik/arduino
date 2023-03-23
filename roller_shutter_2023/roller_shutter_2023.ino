#include <ModbusRtu.h>
//https://github.com/thomasfredericks/Bounce2
#include <Bounce2.h>


#define UP_RELAY A0
#define DOWN_RELAY A1
#define RS485PIN 2

#define UP_TIME 60000
#define DOWN_TIME 60000
#define TILT_TIME 100

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

    pinMode(UP_RELAY, OUTPUT);
    pinMode(DOWN_RELAY, OUTPUT);

    digitalWrite(UP_RELAY, LOW);
    digitalWrite(DOWN_RELAY, LOW);

    bitWrite(address, 0, digitalRead(7));
    bitWrite(address, 1, digitalRead(8));
    bitWrite(address, 2, digitalRead(9));
    bitWrite(address, 3, digitalRead(10));

    address += 50;

    upButton.attach(A2, INPUT);
    upButton.interval(25);
    downButton.attach(A3, INPUT);
    downButton.interval(25);

    slave = Modbus(address, Serial, RS485PIN);
    slave.begin(9600);
}

int8_t state = 0;
int lastUpState = HIGH;
int lastDownState = HIGH;

int down = HIGH;
int up = HIGH;

bool autoMove = false;
bool lastDirection = 0; // 0 - góra, 1 - dół

uint16_t lastModbusRegister1 = 0;
uint16_t lastModbusRegister2 = 0;
bool modbusChanged = false;

unsigned long lastCheckTime = 0;

int state = 0;

void stop()
{
    digitalWrite(UP_RELAY, 0);
    digitalWrite(DOWN_RELAY, 0);
    state = 0;
}

void goUP()
{
    digitalWrite(DOWN_RELAY, 0);
    delay(10);
    digitalWrite(UP_RELAY, 1);
    state = 1;
}

void goDOWN()
{
    digitalWrite(UP_RELAY, 0);
    delay(10);
    digitalWrite(DOWN_RELAY, 1);
    state = 3;
}

//state_opening: 1
//state_open: 2
//state_closing: 3
//state_closed: 4



void loop()
{
    if (autoMove && millis() > (lastCheckTime + UP_TIME))
    {
        autoMove = false;
        stop();
        if (lastDirection == 0)
        {
            state = 2;
        }
        else
        {
            state = 4;
        }
    }

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

    if (modbusChanged && modbusData[0] == 2)
    {
        goUP();
        autoMove = true;
        lastCheckTime = millis();
    }

    if (modbusChanged && modbusData[0] == 0)
    {
        stop();
        autoMove = false;
    }

    if (modbusChanged && modbusData[0] == 4)
    {
        goDOWN();
        autoMove = true;
        lastCheckTime = millis();
    }

    if (down == LOW && up == LOW)
    {
        stop();
    }

    // dwa przyciski
    if (down == HIGH && up == HIGH)
    {
        if (lastDirection == 0)
        {
            lastDirection = 1;
            goDOWN();
        }
        else
        {
            lastDirection = 0;
            goUP();
        }
        lastCheckTime = millis();
        autoMove = true;
    }

    if (down == LOW && up == HIGH)
    {
        if (autoMove)
        {
            stop();
            autoMove = false;
        }
        else
        {
            goDOWN();
        }
    }

    if (down == HIGH && up == LOW)
    {
        if (autoMove)
        {
            stop();
            autoMove = false;
        }
        else
        {
            goUP();
        }
    }

    modbusData[2] = slave.getInCnt();

    lastModbusRegister1 = modbusData[0];
    modbusData[1] = state;

    state = slave.poll( modbusData, 3 );

    if (modbusData[0] != lastModbusRegister1)
    {
        modbusChanged = true;
    }
    else
    {
        modbusChanged = false;
    }
}
