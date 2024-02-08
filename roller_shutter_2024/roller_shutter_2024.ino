//https://github.com/epsilonrt/modbus-serial
#include <ModbusSerial.h>
//https://github.com/thomasfredericks/Bounce2
#include <Bounce2.h>


#define UP_RELAY A0
#define DOWN_RELAY A1

#define UP_TIME 60000
#define DOWN_TIME 60000
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
int address = 1;

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
bool positionMove = false;
bool lastDirection = UP; // 0 - góra, 1 - dół

uint16_t lastModbusCommandRegister = 0;
uint16_t lastModbusTiltRegister = 0;
uint16_t lastModbusPositionRegister = 0;
bool modbusChanged = false;

unsigned long lastCheckTime = 0;
unsigned long positionMoveTime = 0;
int currentPosition = 0;
int currentTilt = 0;
/**
 * jak długo już działa roleta, do wyliczenia pozycji
*/
unsigned long currentWorkTime = 0;

void setup() {

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

    digitalWrite(UP_RELAY, LOW);
    digitalWrite(DOWN_RELAY, LOW);

    bitWrite(address, 0, digitalRead(10));
    bitWrite(address, 1, digitalRead(9));
    bitWrite(address, 2, digitalRead(8));
    bitWrite(address, 3, digitalRead(7));

    address += 50;

    slave.setSlaveId(address);

    upButton.attach(A2, INPUT);
    upButton.interval(25);
    downButton.attach(A3, INPUT);
    downButton.interval(25);

    Serial.begin(9600, MB_PARITY_NONE);
    while (! Serial)
    ;

    Serial.print("Address: ");
    Serial.println(address);

    slave.config(9600);
    mb.setAdditionalServerData("BLIND"); // for Report Server ID function (0x11)

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
    state = 0;
    calculatePosition();
    positionMove = false;
}

void goUP()
{
    digitalWrite(DOWN_RELAY, 0);
    delay(10);
    digitalWrite(UP_RELAY, 1);
    state = 1;
    currentWorkTime = millis();
}

void goDOWN()
{
    digitalWrite(UP_RELAY, 0);
    delay(10);
    digitalWrite(DOWN_RELAY, 1);
    state = 3;
    currentWorkTime = millis();
}

void calculatePosition()
{
    unsigned long timePassed = millis()-currentWorkTime;
    int percent = (timePassed / UP_TIME) * 100;
    if (currentPosition == UP)
    {
        currentPosition -= percent;
        if (currentPosition < 0 || currentPosition > 100)
        {
            currentPosition = 0;
        }
    }
    else if (currentPosition == DOWN)
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

void tilt(int percent)
{
    unsigned long timeNeeded = 0;
    if (lastDirection == UP && currentTilt < 90)
    {
        if (percent > currentTilt) // podnosimy
        {
            timeNeeded = (TILT_TIME * (percent-currentTilt) / 100);
            goUP();
        }
        else
        {
            timeNeeded = (TILT_TIME * (currentTilt-percent) / 100);
            goDOWN();
        }
    }
    else
    {
        goDOWN();
        delay(TILT_TIME);
        timeNeeded = (TILT_TIME * (percent-currentTilt) / 100);
        goUP();
    }
    delay(timeNeeded);
    currentTilt = percent;
    stop();
}

void goToPosition(int percent)
{
    unsigned long timeNeeded = 0;
    if (percent > currentPosition)
    {
        timeNeeded = (DOWN_TIME * (percent-currentPosition) / 100);
        goDOWN();
    }
    else
    {
        timeNeeded = (UP_TIME * (currentPosition-percent) / 100);
        goUP();
    }
    positionMoveTime = millis()+timeNeeded;
    positionMove = true;
}

void loop()
{
    if (positionMove && millis() > positionMoveTime)
    {
        positionMove = false;
        stop();
    }

    if (autoMove && millis() > (lastCheckTime + UP_TIME))
    {
        autoMove = false;
        stop();
        if (lastDirection == 0)
        {
            state = 2;
            currentPosition = 0;
            currentTilt = 0;
        }
        else
        {
            state = 4;
            currentPosition = 100;
            currentTilt = 100;
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
        currentTilt = 100;
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
        currentTilt = 0;
    }

    if (down == OFF && up == OFF && !autoMove && !positionMove)
    {
        stop();
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
    modbusData[5] = currentTilt;

    lastModbusCommandRegister = modbusData[0];
    lastModbusTiltRegister = modbusData[2];
    lastModbusPositionRegister = modbusData[3];
    modbusData[1] = state;

    slave.setHreg(1, modbusData[1]);
    slave.setHreg(4, modbusData[4]);
    slave.setHreg(5, modbusData[5]);

    slave.task();

    modbusData[0] = slave.Hreg(0);
    modbusData[2] = slave.Hreg(2);
    modbusData[3] = slave.Hreg(3);

    if (modbusData[0] != lastModbusCommandRegister)
    {
        modbusChanged = true;
    }
    else
    {
        modbusChanged = false;
    }

    if (modbusData[2] != lastModbusTiltRegister)
    {
        int tiltPercent = modbusData[2];
        if (tiltPercent > 0 && tiltPercent < 100)
        {
            tilt(tiltPercent);
        }
    }

    if (modbusData[3] != lastModbusPositionRegister)
    {
        int openPercent = modbusData[3];
        if (openPercent >= 0 && openPercent < 101)
        {
            goToPosition(openPercent);
        }
    }
}
