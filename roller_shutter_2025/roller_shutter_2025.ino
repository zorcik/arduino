#include <avr/wdt.h>

//https://github.com/epsilonrt/modbus-serial
#include <ModbusSerial.h>
//https://github.com/thomasfredericks/Bounce2
#include <Bounce2.h>

#define UP_RELAY A0
#define DOWN_RELAY A1

#define UP_TIME 31000
#define TILT_TIME 2000

#define ON HIGH
#define OFF LOW
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
bool lastDirection = UP; // 0 - up, 1 - down

uint16_t lastModbusCommandRegister = 0;
uint16_t lastModbusTiltRegister = 0;
uint16_t lastModbusPositionRegister = 0;
bool modbusChanged = false;

unsigned long lastCheckTime = 0;
int currentPosition = 0;

/**
 * movement timing and position tracking
 */
unsigned long moveStartMillis = 0; // millis() when current movement started
int positionAtStart = 0; // position at movement start
int targetPosition = -1; // -1 = no target position via Modbus
unsigned long targetStopMillis = 0; // time to stop when moving to target

// true if current movement is driven by a held button (momentary behavior)
bool movementByButton = false;

const unsigned long RELAY_SWITCH_DELAY = 50; // ms safety delay between switching relays

/**
 * jak długo już działa roleta, do wyliczenia pozycji
*/
unsigned long currentWorkTime = 0; // legacy variable kept for compatibility

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

    digitalWrite(UP_RELAY, OFF);
    digitalWrite(DOWN_RELAY, OFF);

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
}



void stop()
{
    // turn both relays off using ON/OFF macros
    digitalWrite(UP_RELAY, OFF);
    digitalWrite(DOWN_RELAY, OFF);

    // update position based on elapsed move time
    calculatePosition();

    // clear movement state
    moveStartMillis = 0;
    targetStopMillis = 0;
    targetPosition = -1;
    autoMove = false;
    state = 0;
}

void goUP()
{
    // ensure DOWN relay is off before switching UP relay
    digitalWrite(DOWN_RELAY, OFF);
    delay(RELAY_SWITCH_DELAY);
    digitalWrite(UP_RELAY, ON);

    // cancel any target-based movement
    targetPosition = -1;
    targetStopMillis = 0;

    state = 1; // opening
    positionAtStart = currentPosition;
    moveStartMillis = millis();
    autoMove = true;
    lastCheckTime = millis();
}

void goDOWN()
{
    // ensure UP relay is off before switching DOWN relay
    digitalWrite(UP_RELAY, OFF);
    delay(RELAY_SWITCH_DELAY);
    digitalWrite(DOWN_RELAY, ON);

    // cancel any target-based movement
    targetPosition = -1;
    targetStopMillis = 0;

    state = 3; // closing
    positionAtStart = currentPosition;
    moveStartMillis = millis();
    autoMove = true;
    lastCheckTime = millis();
}

void calculatePosition()
{
    // If not moving, nothing to calculate
    if (moveStartMillis == 0) return;

    unsigned long timePassed = millis() - moveStartMillis;
    unsigned long movedPct = (timePassed * 100UL) / UP_TIME; // percent of full travel
    if (movedPct > 100) movedPct = 100;

    int newPos = positionAtStart;
    if (state == 1) // opening -> decrease position
    {
        int delta = (int)movedPct;
        newPos = positionAtStart - delta;
        if (newPos < 0) newPos = 0;
    }
    else if (state == 3) // closing -> increase position
    {
        int delta = (int)movedPct;
        newPos = positionAtStart + delta;
        if (newPos > 100) newPos = 100;
    }

    currentPosition = newPos;

    // If we already passed full travel time, finalize
    if (timePassed >= UP_TIME)
    {
        if (state == 1)
        {
            state = 2; // fully open
            currentPosition = 0;
        }
        else if (state == 3)
        {
            state = 4; // fully closed
            currentPosition = 100;
        }
        // stop movement
        digitalWrite(UP_RELAY, OFF);
        digitalWrite(DOWN_RELAY, OFF);
        moveStartMillis = 0;
        autoMove = false;
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

    // update button states
    upButton.update();
    downButton.update();

    if (upButton.changed()) { up = upButton.read(); }
    if (downButton.changed()) { down = downButton.read(); }

    // Pressed state is LOW because buttons use external pull-ups
    bool upPressed = (up == LOW);
    bool downPressed = (down == LOW);
    static bool bothPressedFlagLocal = false; // track simultaneous press-and-release

    // detect simultaneous press
    if (upPressed && downPressed) {
        bothPressedFlagLocal = true;
    }

    // if both were pressed and now released -> start a timed move (monostable) in lastDirection
    if (bothPressedFlagLocal && !upPressed && !downPressed) {
        bothPressedFlagLocal = false;
        if (autoMove) { stop(); }
        else {
            if (lastDirection == DOWN) { goDOWN(); }
            else { goUP(); }
            // this is a timed movement (UP_TIME), not a held-button movement
            movementByButton = false;
            lastCheckTime = millis();
        }
        delay(50); // small safety delay to avoid immediate re-trigger
    }

    // single button behavior: movement while button is held
    if (upButton.changed()) {
        if (upPressed) {
            // start moving up (button control); overrides ongoing auto moves
            goUP();
            movementByButton = true;
            lastDirection = UP;
        } else {
            // button released -> stop only if movement was triggered by button
            if (movementByButton) { stop(); movementByButton = false; }
        }
        delay(50);
    }

    if (downButton.changed()) {
        if (downPressed) {
            // start moving down (button control); overrides ongoing auto moves
            goDOWN();
            movementByButton = true;
            lastDirection = DOWN;
        } else {
            // button released -> stop only if movement was triggered by button
            if (movementByButton) { stop(); movementByButton = false; }
        }
        delay(50);
    }

    // Handle Modbus command register (0): 2=open, 4=close, 0=stop
    // Read registers from Modbus slave
    modbusData[0] = slave.hreg(0);
    modbusData[2] = slave.hreg(2);
    modbusData[3] = slave.hreg(3);

    // Command register changed handling
    if (modbusData[0] != lastModbusCommandRegister) {
        if (modbusData[0] == 2) { goUP(); }
        else if (modbusData[0] == 4) { goDOWN(); }
        else if (modbusData[0] == 0) { stop(); }
        lastModbusCommandRegister = modbusData[0];
    }

    // Target position (register 3) support: move to requested percent
    if (modbusData[3] != lastModbusPositionRegister) {
        int req = modbusData[3];
        if (req < 0) req = 0; if (req > 100) req = 100;
        if (req != currentPosition) {
            // compute how long to move proportionally
            unsigned long duration = (unsigned long)abs(req - currentPosition) * (unsigned long)UP_TIME / 100UL;
            if (req > currentPosition) { // need to close
                goDOWN();
            } else { // need to open
                goUP();
            }
            targetPosition = req;
            targetStopMillis = millis() + duration;
        }
        lastModbusPositionRegister = modbusData[3];
    }

    // If moving to a target and time elapsed -> finalize
    if (targetPosition != -1 && targetStopMillis != 0 && millis() >= targetStopMillis) {
        // finalize position
        currentPosition = targetPosition;
        targetPosition = -1;
        targetStopMillis = 0;
        // stop movement and set state accordingly
        if (currentPosition == 0) state = 2; else if (currentPosition == 100) state = 4; else state = 0;
        stop();
    }

    // Generic autoMove timeout for full run
    if (autoMove && moveStartMillis != 0 && millis() >= (moveStartMillis + UP_TIME)) {
        calculatePosition();
        // movement completed to an endpoint
        if (state == 1) { state = 2; currentPosition = 0; }
        else if (state == 3) { state = 4; currentPosition = 100; }
        stop();
    }

    // Save state to modbus registers
    modbusData[4] = currentPosition;
    modbusData[1] = state;

    slave.setHreg(1, modbusData[1]);
    slave.setHreg(4, modbusData[4]);

    // detect command register change (keep existing flag for compatibility)
    modbusChanged = (modbusData[0] != lastModbusCommandRegister);

}
