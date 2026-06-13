/*
 * ============================================================
 *  ROLLER SHUTTER CONTROLLER v2.0
 *  MCU: ATmega328P (Arduino Nano / Uno)
 * ============================================================
 *
 * Modbus RTU Holding Registers:
 *   HR 0 [W] Command       0=STOP, 2=OPEN, 4=CLOSE, 6=CALIBRATE
 *   HR 1 [R] State         0=STOP, 1=OPENING, 2=OPEN, 3=CLOSING,
 *                           4=CLOSED, 5=CALIBRATING
 *   HR 2 [W] Target Pos    0-100 (change triggers positioning)
 *   HR 3 [R] Current Pos   0-100 (real-time estimate)
 *   HR 4 [W] Address       50-80 (saved to EEPROM, auto-restart)
 *   HR 5 [R] UP_TIME       calibrated time in 100ms units
 *   HR 6 [R] DOWN_TIME     calibrated time in 100ms units
 *
 * EEPROM Layout:
 *   0x00 [1B]  Magic byte (0xA5 = valid data)
 *   0x01 [1B]  Modbus address (uint8_t)
 *   0x02 [4B]  UP_TIME in ms (uint32_t)
 *   0x06 [4B]  DOWN_TIME in ms (uint32_t)
 *
 * Pinout:
 *   A0 = UP relay    (active HIGH)
 *   A1 = DOWN relay  (active HIGH)
 *   A2 = DOWN button (active LOW, internal pull-up)
 *   A3 = UP button   (active LOW, internal pull-up)
 *   A4 = ACS712 current sensor — UP channel
 *   A5 = ACS712 current sensor — DOWN channel
 *   D0/D1 = UART (Modbus RTU via RS-485 converter)
 *
 * Libraries:
 *   ModbusSerial — https://github.com/epsilonrt/modbus-serial
 *   Bounce2      — https://github.com/thomasfredericks/Bounce2
 */

#include <avr/wdt.h>
#include <EEPROM.h>
#include <ModbusSerial.h>
#include <Bounce2.h>

// ============================================================
// Disable WDT early after reset (some bootloaders don't do this)
// Prevents boot loop if WDT triggered the reset.
// ============================================================
void wdt_init(void) __attribute__((naked)) __attribute__((section(".init3")));
void wdt_init(void) {
    MCUSR = 0;
    wdt_disable();
}

// ============================================================
// PIN DEFINITIONS
// ============================================================
#define UP_RELAY_PIN    A0
#define DOWN_RELAY_PIN  A1
#define DOWN_BTN_PIN    A2
#define UP_BTN_PIN      A3
#define ACS_UP_PIN      A4
#define ACS_DOWN_PIN    A5

// ============================================================
// CONFIGURATION — DEFAULTS & LIMITS
// ============================================================
#define DEFAULT_UP_TIME    30000UL    // Default full-open time (ms)
#define DEFAULT_DOWN_TIME  30000UL    // Default full-close time (ms)
#define DEFAULT_ADDRESS    50         // Default Modbus slave address
#define MIN_ADDRESS        50
#define MAX_ADDRESS        80

// ============================================================
// ACS712 CURRENT SENSING
// Adjust ACS_THRESHOLD experimentally for your motor/sensor!
// A larger value requires more current to be detected as "flowing".
// ============================================================
#define ACS_THRESHOLD         15       // Min peak-to-peak ADC value for current detection
#define ACS_WINDOW_MS         200UL    // Measurement window in ms (~4 cycles of 50Hz)
#define ACS_STARTUP_DELAY_MS  1000UL   // Ignore ACS this long after motor start

// ============================================================
// TIMING
// ============================================================
#define RELAY_SWITCH_DELAY_MS  10      // Dead-time between relay off and on (ms)
#define DEBOUNCE_MS            25      // Button debounce interval (ms)
#define CAL_PAUSE_MS           500UL   // Pause between calibration direction changes (ms)
#define CAL_SAFETY_TIMEOUT_MS  90000UL // Max time per calibration movement phase (ms)

// ============================================================
// DIRECTIONS
// ============================================================
#define DIR_UP    0
#define DIR_DOWN  1

// ============================================================
// DEVICE STATES (reported in Modbus HR 1)
// ============================================================
#define STATE_STOP        0
#define STATE_OPENING     1
#define STATE_OPEN        2
#define STATE_CLOSING     3
#define STATE_CLOSED      4
#define STATE_CALIBRATING 5

// ============================================================
// MODBUS COMMANDS (written to Modbus HR 0)
// ============================================================
#define CMD_STOP      0
#define CMD_OPEN      2
#define CMD_CLOSE     4
#define CMD_CALIBRATE 6

// ============================================================
// INTERNAL MOVE MODES
// ============================================================
#define MOVE_NONE       0    // Motor stopped
#define MOVE_MANUAL     1    // Button hold-to-run
#define MOVE_AUTO       2    // Timed movement (full travel or positioning)
#define MOVE_CALIBRATE  3    // Calibration in progress

// ============================================================
// CALIBRATION PHASES
// ============================================================
#define CAL_IDLE            0
#define CAL_GOING_UP_INIT   1   // Moving up to find top end-stop
#define CAL_PAUSE_1         2   // Pausing before going down
#define CAL_GOING_DOWN      3   // Moving down, measuring DOWN_TIME
#define CAL_PAUSE_2         4   // Pausing before going up
#define CAL_GOING_UP_FINAL  5   // Moving up, measuring UP_TIME

// ============================================================
// EEPROM LAYOUT
// ============================================================
#define EEPROM_MAGIC_ADDR    0   // 1 byte: magic value
#define EEPROM_ADDRESS_ADDR  1   // 1 byte: Modbus address
#define EEPROM_UPTIME_ADDR   2   // 4 bytes: UP_TIME (unsigned long)
#define EEPROM_DOWNTIME_ADDR 6   // 4 bytes: DOWN_TIME (unsigned long)
#define EEPROM_MAGIC_VALUE   0xA5

// ============================================================
// MODBUS REGISTER ADDRESSES
// ============================================================
#define HREG_COMMAND      0
#define HREG_STATE        1
#define HREG_TARGET_POS   2
#define HREG_CURRENT_POS  3
#define HREG_ADDRESS      4
#define HREG_UPTIME       5
#define HREG_DOWNTIME     6
#define HREG_COUNT        7

// ============================================================
// EEPROM: Load address (called during global variable init,
// before main() — uses low-level AVR eeprom_read_byte)
// ============================================================
uint8_t loadAddressFromEEPROM() {
    if (eeprom_read_byte((const uint8_t *)EEPROM_MAGIC_ADDR) == EEPROM_MAGIC_VALUE) {
        uint8_t addr = eeprom_read_byte((const uint8_t *)EEPROM_ADDRESS_ADDR);
        if (addr >= MIN_ADDRESS && addr <= MAX_ADDRESS) {
            return addr;
        }
    }
    return DEFAULT_ADDRESS;
}

// ============================================================
// GLOBAL VARIABLES
// ============================================================

// --- Modbus ---
uint8_t modbusAddress = loadAddressFromEEPROM();
ModbusSerial slave(Serial, modbusAddress, -1);

// --- Buttons ---
Bounce upButton;
Bounce downButton;
bool btnUp = false;      // true = pressed
bool btnDown = false;
bool waitFlag = false;   // prevents repeat triggers until buttons released

// --- Motor state ---
uint8_t deviceState = STATE_STOP;
uint8_t moveMode = MOVE_NONE;
uint8_t lastDirection = DIR_UP;

// --- Position tracking ---
int16_t currentPosition = 0;    // 0 = fully open, 100 = fully closed
int16_t targetPosition = -1;    // -1 = no target (full travel)

// --- Timing ---
unsigned long upTime = DEFAULT_UP_TIME;
unsigned long downTime = DEFAULT_DOWN_TIME;
unsigned long moveStartTime = 0;
unsigned long moveTargetDuration = 0;

// --- ACS712 current sensing ---
int16_t acsMax = 0;
int16_t acsMin = 1023;
unsigned long acsWindowStart = 0;
bool acsCurrentFlowing = true;
bool acsStartupGrace = false;

// --- Calibration ---
uint8_t calPhase = CAL_IDLE;
unsigned long calPhaseStart = 0;
unsigned long calMeasureStart = 0;

// --- Modbus register change detection ---
uint16_t prevCommand = CMD_STOP;
uint16_t prevTargetPos = 0;
uint16_t prevAddress = 0;

// ============================================================
// EEPROM: Load calibrated times (called in setup)
// ============================================================
void loadTimesFromEEPROM() {
    if (EEPROM.read(EEPROM_MAGIC_ADDR) != EEPROM_MAGIC_VALUE) return;

    unsigned long t;
    EEPROM.get(EEPROM_UPTIME_ADDR, t);
    if (t >= 1000UL && t <= 120000UL) upTime = t;

    EEPROM.get(EEPROM_DOWNTIME_ADDR, t);
    if (t >= 1000UL && t <= 120000UL) downTime = t;
}

// ============================================================
// EEPROM: Save functions
// ============================================================
void saveTimesToEEPROM() {
    EEPROM.update(EEPROM_MAGIC_ADDR, EEPROM_MAGIC_VALUE);
    EEPROM.put(EEPROM_UPTIME_ADDR, upTime);
    EEPROM.put(EEPROM_DOWNTIME_ADDR, downTime);
}

void saveAddressToEEPROM(uint8_t addr) {
    EEPROM.update(EEPROM_MAGIC_ADDR, EEPROM_MAGIC_VALUE);
    EEPROM.update(EEPROM_ADDRESS_ADDR, addr);
}

// ============================================================
// SOFTWARE RESET (via WDT trick)
// ============================================================
void softwareReset() {
    wdt_enable(WDTO_15MS);
    while (1) {} // WDT fires → MCU resets
}

// ============================================================
// RELAY CONTROL
// ============================================================
void relayStop() {
    digitalWrite(UP_RELAY_PIN, LOW);
    digitalWrite(DOWN_RELAY_PIN, LOW);
}

void relayUp() {
    digitalWrite(DOWN_RELAY_PIN, LOW);
    delay(RELAY_SWITCH_DELAY_MS);
    digitalWrite(UP_RELAY_PIN, HIGH);
}

void relayDown() {
    digitalWrite(UP_RELAY_PIN, LOW);
    delay(RELAY_SWITCH_DELAY_MS);
    digitalWrite(DOWN_RELAY_PIN, HIGH);
}

// ============================================================
// ACS712: Reset tracking for new movement
// ============================================================
void resetACS() {
    acsMax = 0;
    acsMin = 1023;
    acsWindowStart = millis();
    acsCurrentFlowing = true;   // Assume current at start
    acsStartupGrace = true;     // Ignore readings during startup
}

// ============================================================
// MOTOR CONTROL: Helpers
// ============================================================
unsigned long getTimeForDirection(uint8_t dir) {
    return (dir == DIR_UP) ? upTime : downTime;
}

void startMotor(uint8_t direction) {
    if (direction == DIR_UP) {
        relayUp();
        deviceState = STATE_OPENING;
    } else {
        relayDown();
        deviceState = STATE_CLOSING;
    }
    lastDirection = direction;
    moveStartTime = millis();
    resetACS();
}

/**
 * Stop motor and calculate position from elapsed time.
 * Used for normal stops (timeout, button release, Modbus STOP).
 */
void stopMotor() {
    relayStop();

    // Update position based on how long the motor was running
    if (deviceState == STATE_OPENING || deviceState == STATE_CLOSING) {
        unsigned long elapsed = millis() - moveStartTime;
        unsigned long totalTime = getTimeForDirection(lastDirection);
        int16_t percentMoved = (int16_t)((elapsed * 100UL) / totalTime);
        if (percentMoved > 100) percentMoved = 100;

        if (lastDirection == DIR_UP) {
            currentPosition -= percentMoved;
        } else {
            currentPosition += percentMoved;
        }
        if (currentPosition < 0) currentPosition = 0;
        if (currentPosition > 100) currentPosition = 100;
    }

    deviceState = STATE_STOP;
    moveMode = MOVE_NONE;
    targetPosition = -1;
}

/**
 * Stop motor at a mechanical end-stop (ACS712 detected zero current).
 * Sets position to 0 (open) or 100 (closed) definitively.
 */
void stopMotorAtEndStop() {
    relayStop();

    if (lastDirection == DIR_UP) {
        currentPosition = 0;
        deviceState = STATE_OPEN;
    } else {
        currentPosition = 100;
        deviceState = STATE_CLOSED;
    }

    moveMode = MOVE_NONE;
    targetPosition = -1;
}

// ============================================================
// POSITION ESTIMATION (real-time, for Modbus register)
// ============================================================
int16_t getEstimatedPosition() {
    // Only estimate during active non-calibration movement
    if (deviceState != STATE_OPENING && deviceState != STATE_CLOSING) {
        return currentPosition;
    }

    unsigned long elapsed = millis() - moveStartTime;
    unsigned long totalTime = getTimeForDirection(lastDirection);
    int16_t percentMoved = (int16_t)((elapsed * 100UL) / totalTime);
    if (percentMoved > 100) percentMoved = 100;

    int16_t pos;
    if (lastDirection == DIR_UP) {
        pos = currentPosition - percentMoved;
    } else {
        pos = currentPosition + percentMoved;
    }

    if (pos < 0) pos = 0;
    if (pos > 100) pos = 100;
    return pos;
}

// ============================================================
// ACS712 CURRENT SENSING
// ============================================================

/**
 * Sample the ACS712 sensor and update peak-to-peak tracking.
 * Called every loop iteration when motor is running.
 * Every ACS_WINDOW_MS, evaluates whether current is flowing.
 */
void updateACS() {
    // Read the channel for the current direction of travel
    int16_t val;
    if (lastDirection == DIR_UP) {
        val = analogRead(ACS_UP_PIN);
    } else {
        val = analogRead(ACS_DOWN_PIN);
    }

    if (val > acsMax) acsMax = val;
    if (val < acsMin) acsMin = val;

    unsigned long now = millis();

    // Evaluate at end of each measurement window
    if ((now - acsWindowStart) >= ACS_WINDOW_MS) {
        acsCurrentFlowing = ((acsMax - acsMin) >= ACS_THRESHOLD);

        // Reset for next window
        acsMax = 0;
        acsMin = 1023;
        acsWindowStart = now;
    }

    // End startup grace period
    if (acsStartupGrace && (now - moveStartTime) >= ACS_STARTUP_DELAY_MS) {
        acsStartupGrace = false;
    }
}

/**
 * Check if mechanical end-stop has been reached.
 * Only valid after startup grace period and during active movement.
 */
bool isEndStopReached() {
    if (acsStartupGrace) return false;
    if (moveMode == MOVE_NONE && calPhase == CAL_IDLE) return false;
    return !acsCurrentFlowing;
}

// ============================================================
// CALIBRATION STATE MACHINE
// ============================================================

void abortCalibration() {
    relayStop();
    calPhase = CAL_IDLE;
    deviceState = STATE_STOP;
    moveMode = MOVE_NONE;
}

void startCalibration() {
    calPhase = CAL_GOING_UP_INIT;
    moveMode = MOVE_CALIBRATE;
    startMotor(DIR_UP);
    deviceState = STATE_CALIBRATING;   // Override startMotor's state
    calPhaseStart = millis();
}

/**
 * Non-blocking calibration state machine.
 * Sequence: UP→top → pause → DOWN (measure) → pause → UP (measure) → done.
 */
void updateCalibration() {
    if (calPhase == CAL_IDLE) return;

    unsigned long now = millis();

    // --- Handle pause phases (non-blocking timers) ---
    if (calPhase == CAL_PAUSE_1) {
        if ((now - calPhaseStart) >= CAL_PAUSE_MS) {
            calPhase = CAL_GOING_DOWN;
            startMotor(DIR_DOWN);
            deviceState = STATE_CALIBRATING;
            calPhaseStart = now;
            calMeasureStart = now;
        }
        return;
    }
    if (calPhase == CAL_PAUSE_2) {
        if ((now - calPhaseStart) >= CAL_PAUSE_MS) {
            calPhase = CAL_GOING_UP_FINAL;
            startMotor(DIR_UP);
            deviceState = STATE_CALIBRATING;
            calPhaseStart = now;
            calMeasureStart = now;
        }
        return;
    }

    // --- Safety timeout for movement phases ---
    if ((now - calPhaseStart) > CAL_SAFETY_TIMEOUT_MS) {
        abortCalibration();
        return;
    }

    // --- Wait for ACS712 to detect end-stop ---
    if (!isEndStopReached()) return;

    // --- End-stop reached: advance to next phase ---
    switch (calPhase) {
        case CAL_GOING_UP_INIT:
            // Reached top end-stop → pause before going down
            relayStop();
            calPhase = CAL_PAUSE_1;
            calPhaseStart = now;
            break;

        case CAL_GOING_DOWN:
            // Reached bottom end-stop → record DOWN_TIME
            downTime = now - calMeasureStart;
            relayStop();
            calPhase = CAL_PAUSE_2;
            calPhaseStart = now;
            break;

        case CAL_GOING_UP_FINAL:
            // Reached top end-stop → record UP_TIME, save, done
            upTime = now - calMeasureStart;
            relayStop();
            saveTimesToEEPROM();
            currentPosition = 0;
            deviceState = STATE_OPEN;
            moveMode = MOVE_NONE;
            calPhase = CAL_IDLE;
            break;
    }
}

// ============================================================
// MODBUS COMMAND PROCESSING
// ============================================================
void processModbus() {
    uint16_t command   = slave.hreg(HREG_COMMAND);
    uint16_t targetPos = slave.hreg(HREG_TARGET_POS);
    uint16_t address   = slave.hreg(HREG_ADDRESS);

    // --- HR 0: Command register ---
    if (command != prevCommand) {
        switch (command) {
            case CMD_STOP:
                if (calPhase != CAL_IDLE) {
                    abortCalibration();
                } else if (moveMode != MOVE_NONE) {
                    stopMotor();
                }
                break;

            case CMD_OPEN:
                if (calPhase == CAL_IDLE) {
                    if (moveMode != MOVE_NONE) stopMotor();
                    startMotor(DIR_UP);
                    moveMode = MOVE_AUTO;
                    moveTargetDuration = upTime;
                    targetPosition = -1;
                }
                break;

            case CMD_CLOSE:
                if (calPhase == CAL_IDLE) {
                    if (moveMode != MOVE_NONE) stopMotor();
                    startMotor(DIR_DOWN);
                    moveMode = MOVE_AUTO;
                    moveTargetDuration = downTime;
                    targetPosition = -1;
                }
                break;

            case CMD_CALIBRATE:
                if (calPhase != CAL_IDLE) abortCalibration();
                if (moveMode != MOVE_NONE) stopMotor();
                startCalibration();
                break;
        }

        // Auto-clear: reset register to STOP so the same command
        // can be sent again without needing to write a different value first
        slave.setHreg(HREG_COMMAND, CMD_STOP);
        prevCommand = CMD_STOP;
    }

    // --- HR 2: Target position register ---
    if (targetPos != prevTargetPos) {
        prevTargetPos = targetPos;

        if (calPhase == CAL_IDLE && targetPos <= 100) {
            int16_t target = (int16_t)targetPos;
            int16_t current = getEstimatedPosition();

            if (target != current) {
                if (moveMode != MOVE_NONE) stopMotor();

                uint8_t direction;
                int16_t diff;
                if (target < current) {
                    direction = DIR_UP;
                    diff = current - target;
                } else {
                    direction = DIR_DOWN;
                    diff = target - current;
                }

                targetPosition = target;
                moveTargetDuration = (unsigned long)diff
                                     * getTimeForDirection(direction) / 100UL;
                startMotor(direction);
                moveMode = MOVE_AUTO;
            }
        }
    }

    // --- HR 4: Address register ---
    if (address != prevAddress) {
        prevAddress = address;

        if (address >= MIN_ADDRESS && address <= MAX_ADDRESS &&
            (uint8_t)address != modbusAddress) {
            // Stop everything before reset
            if (moveMode != MOVE_NONE) stopMotor();
            if (calPhase != CAL_IDLE) abortCalibration();
            relayStop();
            saveAddressToEEPROM((uint8_t)address);
            softwareReset(); // Does not return
        }
    }
}

// ============================================================
// BUTTON PROCESSING
// ============================================================
void processButtons() {
    upButton.update();
    downButton.update();

    if (upButton.changed())   btnUp   = (upButton.read() == LOW);
    if (downButton.changed()) btnDown = (downButton.read() == LOW);

    // --- Both buttons released ---
    if (!btnUp && !btnDown) {
        if (moveMode == MOVE_MANUAL) stopMotor();
        waitFlag = false;
        return;
    }

    // Block repeated actions until buttons are released
    if (waitFlag) return;

    // --- Any button during calibration → abort ---
    if (calPhase != CAL_IDLE) {
        abortCalibration();
        waitFlag = true;
        return;
    }

    // --- Both buttons pressed → full travel in last direction ---
    if (btnUp && btnDown) {
        if (moveMode != MOVE_NONE) stopMotor();
        uint8_t dir = lastDirection;
        startMotor(dir);
        moveMode = MOVE_AUTO;
        moveTargetDuration = getTimeForDirection(dir);
        targetPosition = -1;
        waitFlag = true;
        return;
    }

    // --- Single button pressed ---
    uint8_t pressedDir = btnUp ? DIR_UP : DIR_DOWN;

    // If auto-move is active → stop (toggle behavior)
    if (moveMode == MOVE_AUTO) {
        stopMotor();
        waitFlag = true;
        return;
    }

    // Start manual hold-to-run (with safety timeout)
    if (moveMode == MOVE_NONE) {
        startMotor(pressedDir);
        moveMode = MOVE_MANUAL;
        moveTargetDuration = getTimeForDirection(pressedDir);
    }
}

// ============================================================
// SETUP
// ============================================================
void setup() {
    // Relay pins OFF first
    pinMode(UP_RELAY_PIN, OUTPUT);
    pinMode(DOWN_RELAY_PIN, OUTPUT);
    digitalWrite(UP_RELAY_PIN, LOW);
    digitalWrite(DOWN_RELAY_PIN, LOW);

    // Buttons with internal pull-ups
    upButton.attach(UP_BTN_PIN, INPUT_PULLUP);
    upButton.interval(DEBOUNCE_MS);
    downButton.attach(DOWN_BTN_PIN, INPUT_PULLUP);
    downButton.interval(DEBOUNCE_MS);

    // Load calibrated travel times from EEPROM
    loadTimesFromEEPROM();

    // Initialize Modbus RTU
    slave.config(9600);
    slave.setAdditionalServerData("BLIND");

    // Register holding registers
    for (uint8_t i = 0; i < HREG_COUNT; i++) {
        slave.addHreg(i);
    }

    // Set initial output register values
    slave.setHreg(HREG_STATE, deviceState);
    slave.setHreg(HREG_CURRENT_POS, currentPosition);
    slave.setHreg(HREG_ADDRESS, modbusAddress);
    slave.setHreg(HREG_UPTIME, (uint16_t)(upTime / 100UL));
    slave.setHreg(HREG_DOWNTIME, (uint16_t)(downTime / 100UL));

    // Initialize change detection baselines
    prevCommand = CMD_STOP;
    prevTargetPos = 0;
    prevAddress = modbusAddress;

    // Enable Watchdog Timer — 2 second timeout
    wdt_enable(WDTO_2S);
}

// ============================================================
// MAIN LOOP
// ============================================================
void loop() {
    wdt_reset();

    // 1. Process incoming Modbus frames
    slave.task();

    // 2. Update ACS712 current sensing (only when motor is active)
    if (moveMode != MOVE_NONE || calPhase != CAL_IDLE) {
        updateACS();
    }

    // 3. Run calibration state machine
    if (calPhase != CAL_IDLE) {
        updateCalibration();
    }

    // 4. Process Modbus register changes
    processModbus();

    // 5. Process physical buttons (higher priority — runs after Modbus)
    processButtons();

    // 6. Check auto-move timeout (full travel or positioning)
    if (moveMode == MOVE_AUTO) {
        if ((millis() - moveStartTime) >= moveTargetDuration) {
            relayStop();
            if (targetPosition >= 0) {
                // Position move completed → set exact target position
                currentPosition = targetPosition;
                deviceState = STATE_STOP;
            } else {
                // Full travel completed → set end position
                if (lastDirection == DIR_UP) {
                    currentPosition = 0;
                    deviceState = STATE_OPEN;
                } else {
                    currentPosition = 100;
                    deviceState = STATE_CLOSED;
                }
            }
            moveMode = MOVE_NONE;
            targetPosition = -1;
        }
    }

    // 7. Check manual movement safety timeout
    if (moveMode == MOVE_MANUAL) {
        if ((millis() - moveStartTime) >= moveTargetDuration) {
            stopMotor();
        }
    }

    // 8. Check ACS712 end-stop detection (normal movement only)
    if ((moveMode == MOVE_AUTO || moveMode == MOVE_MANUAL) && isEndStopReached()) {
        stopMotorAtEndStop();
    }

    // 9. Update Modbus output registers
    slave.setHreg(HREG_STATE, deviceState);
    slave.setHreg(HREG_CURRENT_POS, (uint16_t)getEstimatedPosition());
    slave.setHreg(HREG_ADDRESS, modbusAddress);
    slave.setHreg(HREG_UPTIME, (uint16_t)(upTime / 100UL));
    slave.setHreg(HREG_DOWNTIME, (uint16_t)(downTime / 100UL));
}
