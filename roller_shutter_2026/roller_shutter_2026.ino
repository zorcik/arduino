#include <avr/wdt.h>

//https://github.com/epsilonrt/modbus-serial
#include <ModbusSerial.h>
//https://github.com/thomasfredericks/Bounce2
#include <Bounce2.h>

#define UP_RELAY A0
#define DOWN_RELAY A1
#define UP_RELAY2 8
#define DOWN_RELAY2 9

#define UP_TIME 31000
#define DOWN_TIME 31000
#define TILT_TIME 2000

#define ON LOW
#define OFF HIGH
#define UP 0
#define DOWN 1



/**
 * 0 - rozkaz:
 *      0 - stop
 *      2 - otwórz
 *      4 - zamknij
 * 1 - potwierdzenie
 *      0 - stop
 *      1 - otwiera się
 *      2 - otwarte
 *      3 - zamyka się
 *      4 - zamknięte
 * 2 - tilt [0,100]
 * 3 - otwarcie % [0,100]
 * 4 - stan otwarcia % [0,100]
 * 5 - stan tilt
*/
uint16_t modbusData[7];
int address = 51;

ModbusSerial slave(Serial, address, -1);

Bounce upButton = Bounce();
Bounce downButton = Bounce();

/**
 * aktualny stan rolety
*/
int8_t state = 0;

int down = HIGH;
int up = HIGH;

bool autoMove = false;
bool lastDirection = UP; // 0 - góra, 1 - dół

uint16_t lastModbusCommandRegister = 0;
uint16_t lastModbusTiltRegister = 0;
uint16_t lastModbusPositionRegister = 0;
bool modbusChanged = false;

unsigned long lastCheckTime = 0;
unsigned long positionMoveTime = 0;
int currentPosition = 0;
/**
 * jak długo już działa roleta, do wyliczenia pozycji
*/
unsigned long currentWorkTime = 0;

void setup() {

    wdt_enable(WDTO_1S);

    pinMode(7, INPUT);
    pinMode(8, INPUT);
    pinMode(9, INPUT);
    pinMode(10, INPUT);

    digitalWrite(7, LOW);
    digitalWrite(8, LOW);
    digitalWrite(9, LOW);
    digitalWrite(10, LOW);


    pinMode(UP_RELAY, OUTPUT);
    pinMode(DOWN_RELAY, OUTPUT);
    pinMode(UP_RELAY2, OUTPUT);
    pinMode(DOWN_RELAY2, OUTPUT);

    digitalWrite(UP_RELAY, LOW);
    digitalWrite(DOWN_RELAY, LOW);
    digitalWrite(UP_RELAY2, LOW);
    digitalWrite(DOWN_RELAY2, LOW);

    upButton.attach(A3, INPUT);
    upButton.interval(25);
    downButton.attach(A2, INPUT);
    downButton.interval(25);

    Serial.begin(9600, MB_PARITY_NONE);
    slave.config(9600);
    slave.setAdditionalServerData("BLIND"); // for Report Server ID function (0x11)

    slave.addHreg(0);
    slave.addHreg(1);
    slave.addHreg(2);
    slave.addHreg(3);
    slave.addHreg(4);
    slave.addHreg(5);
}



void stop()
{
    digitalWrite(UP_RELAY, 0);
    digitalWrite(DOWN_RELAY, 0);
    digitalWrite(UP_RELAY2, 0);
    digitalWrite(DOWN_RELAY2, 0);
    calculatePosition();
    state = 0;
}

void simpleStop()
{
    digitalWrite(UP_RELAY, 0);
    digitalWrite(DOWN_RELAY, 0);
    digitalWrite(UP_RELAY2, 0);
    digitalWrite(DOWN_RELAY2, 0);
}

void goUP()
{
    digitalWrite(DOWN_RELAY, 0);
    digitalWrite(DOWN_RELAY2, 0);
    delay(10);
    digitalWrite(UP_RELAY, 1);
    digitalWrite(UP_RELAY2, 1);
    state = 1;
    currentWorkTime = millis();
    lastDirection = UP;
}

void goDOWN()
{
    digitalWrite(UP_RELAY, 0);
    digitalWrite(UP_RELAY2, 0);
    delay(10);
    digitalWrite(DOWN_RELAY, 1);
    digitalWrite(DOWN_RELAY2, 1);
    state = 3;
    currentWorkTime = millis();
    lastDirection = DOWN;
}

void calculatePosition()
{
    unsigned long timePassed = millis()-currentWorkTime;
    int percent = (timePassed / UP_TIME) * 100;
    if (state == 1)
    {
        currentPosition -= percent;
        if (currentPosition < 0 || currentPosition > 100)
        {
            currentPosition = 0;
        }
    }
    else if (state == 3)
    {
        currentPosition += percent;
        if (currentPosition < 0 || currentPosition > 100)
        {
            currentPosition = 100;
        }
    }
}

//state_opening: 1
//state_open: 2
//state_closing: 3
//state_closed: 4

bool waitFlag = false;

unsigned long mils = 0;

void loop()
{
    wdt_reset();

    mils = millis();

    slave.task();

    if (autoMove && mils > (lastCheckTime + UP_TIME))
    {
        autoMove = false;
        stop();
        if (lastDirection == UP)
        {
            state = 2;
            currentPosition = 0;
        }
        else
        {
            state = 4;
            currentPosition = 100;
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
        modbusChanged = false;
    }

    if (modbusChanged && modbusData[0] == 0)
    {
        stop();
        autoMove = false;
        modbusChanged = false;
    }

    if (modbusChanged && modbusData[0] == 4)
    {
        goDOWN();
        autoMove = true;
        lastCheckTime = millis();
        modbusChanged = false;
    }

    if (down == OFF && up == OFF && !autoMove)
    {
        simpleStop();
    }

    if (down == OFF && up == OFF)
    {
        waitFlag = false;
    }

    // dwa przyciski
    if (down == ON && up == ON && !waitFlag)
    {
        if (lastDirection == DOWN)
        {
            goDOWN();
        }
        else
        {
            goUP();
        }
        lastCheckTime = millis();
        autoMove = true;
        waitFlag = true;
        delay(100);
    }

    if (down == OFF && up == ON && !waitFlag)
    {
        if (autoMove)
        {
            stop();
            autoMove = false;
            waitFlag = true;
        }
        else
        {
            goDOWN();
            lastDirection = DOWN;
        }
        delay(50);
    }

    if (down == ON && up == OFF && !waitFlag)
    {
        if (autoMove)
        {
            stop();
            autoMove = false;
            waitFlag = true;
        }
        else
        {
            goUP();
            lastDirection = UP;
        }
        delay(50);
    }



    modbusData[4] = currentPosition;
    modbusData[5] = 99;

    lastModbusCommandRegister = modbusData[0];
    lastModbusTiltRegister = modbusData[2];
    lastModbusPositionRegister = modbusData[3];
    modbusData[1] = state;

    slave.setHreg(1, modbusData[1]);
    slave.setHreg(4, modbusData[4]);
    slave.setHreg(5, modbusData[5]);

    modbusData[0] = slave.hreg(0);
    modbusData[2] = slave.hreg(2);
    modbusData[3] = slave.hreg(3);

    if (modbusData[0] != lastModbusCommandRegister)
    {
        modbusChanged = true;
    }
    else
    {
        modbusChanged = false;
    }
}
