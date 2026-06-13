/*
 * ============================================================
 *  ROLLER SHUTTER CONTROLLER v2.0 — ESP32-C3
 *  MCU: ESP32-C3 (RISC-V, single-core)
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
 * EEPROM Layout (flash-backed):
 *   0x00 [1B]  Magic byte (0xA5 = valid data)
 *   0x01 [1B]  Modbus address (uint8_t)
 *   0x02 [4B]  UP_TIME in ms (uint32_t)
 *   0x06 [4B]  DOWN_TIME in ms (uint32_t)
 *
 * Pinout:
 *   GPIO 0  = UP relay    (active HIGH)
 *   GPIO 1  = DOWN relay  (active HIGH)
 *   GPIO 2  = DOWN button (active LOW, internal pull-up)
 *   GPIO 3  = UP button   (active LOW, internal pull-up)
 *   GPIO 4  = ACS712 current sensor — UP channel (ADC)
 *   GPIO 5  = ACS712 current sensor — DOWN channel (ADC)
 *   GPIO 10 = Modbus UART RX
 *   GPIO 20 = Modbus UART TX
 *   USB/UART0 = Debug serial (115200 baud)
 *
 * NOTE: GPIO 2 is a strapping pin on ESP32-C3. If the DOWN button
 *       is held during boot, it may affect boot mode. Consider
 *       adding an external pull-up or using a different GPIO.
 *
 * Serials:
 *   Serial  (UART0) — Debug output (115200, USB)
 *   mySerial (UART1) — Modbus RTU (9600, GPIO 10 RX / GPIO 20 TX)
 *
 * Libraries:
 *   ModbusSerial — https://github.com/epsilonrt/modbus-serial
 *   Bounce2      — https://github.com/thomasfredericks/Bounce2
 */

#include <WiFi.h>
#include <EEPROM.h>
#include <ModbusSerial.h>
#include <Bounce2.h>

// ============================================================
// DEBUG CONFIGURATION
// When true, debug messages are printed to Serial (UART0/USB).
// ============================================================
#define DEBUG_ENABLED  true
#define HARDWARE_VERSION 42

#if DEBUG_ENABLED
  #define DBG_PRINTF(...)  Serial.printf(__VA_ARGS__)
#else
  #define DBG_PRINTF(...)
#endif

// ============================================================
// PIN DEFINITIONS
// ============================================================
#if HARDWARE_VERSION < 42
    #define UP_RELAY_PIN    0
    #define DOWN_RELAY_PIN  1
    #define DOWN_BTN_PIN    2
    #define UP_BTN_PIN      3
    #define ACS_UP_PIN      4
    #define ACS_DOWN_PIN    5
#else
    #define UP_RELAY_PIN    5
    #define DOWN_RELAY_PIN  1
    #define DOWN_BTN_PIN    2
    #define UP_BTN_PIN      3
    #define ACS_UP_PIN      4
    #define ACS_DOWN_PIN    0
#endif
// ============================================================
// MODBUS SERIAL PINS (UART1)
// ============================================================
#define MODBUS_RX_PIN   10
#define MODBUS_TX_PIN   20

// ============================================================
// CONFIGURATION — DEFAULTS & LIMITS
// ============================================================
#define DEFAULT_UP_TIME    30000UL    // Default full-open time (ms)
#define DEFAULT_DOWN_TIME  30000UL    // Default full-close time (ms)
#define DEFAULT_ADDRESS    50         // Default Modbus slave address
#define MIN_ADDRESS        50
#define MAX_ADDRESS        80

// ============================================================
// ACS712 HARDWARE PRESENCE
// Set to false if ACS712 sensors are NOT installed on the board.
// When false: current sensing, end-stop detection and calibration
// are fully disabled. Only predefined/EEPROM travel times are used.
// ============================================================
#define ACS_INSTALLED      true

// ============================================================
// ACS712 CURRENT SENSING
// Adjust ACS_THRESHOLD experimentally for your motor/sensor!
// NOTE: ESP32-C3 uses 12-bit ADC (0-4095), so thresholds are
// ~4x larger than on ATmega328P (10-bit, 0-1023).
// ============================================================
#define ACS_THRESHOLD         60       // Min peak-to-peak ADC value for current detection
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
// ESP32 ADC
// ============================================================
#define ADC_MAX_VALUE    4095          // 12-bit ADC resolution

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
// EEPROM LAYOUT (flash-backed on ESP32)
// ============================================================
#define EEPROM_SIZE          16  // bytes to allocate
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
// GLOBAL VARIABLES
// ============================================================

// --- Modbus (UART1) ---
HardwareSerial modbusSerial(1);
ModbusSerial* slave = nullptr;
uint8_t modbusAddress = DEFAULT_ADDRESS;

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
int16_t acsMin = ADC_MAX_VALUE;
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

// --- Debug state tracking ---
#if DEBUG_ENABLED
uint8_t prevDebugState = 0xFF;
int16_t prevDebugPos = -1;
#endif

// ============================================================
// EEPROM: Load all settings (called in setup after EEPROM.begin)
// ============================================================
void loadFromEEPROM() {
    if (EEPROM.read(EEPROM_MAGIC_ADDR) != EEPROM_MAGIC_VALUE) {
        DBG_PRINTF("[EEPROM] No valid data (magic=0x%02X), using defaults\n",
                   EEPROM.read(EEPROM_MAGIC_ADDR));
        return;
    }

    uint8_t addr = EEPROM.read(EEPROM_ADDRESS_ADDR);
    if (addr >= MIN_ADDRESS && addr <= MAX_ADDRESS) {
        modbusAddress = addr;
    }

    unsigned long t;
    EEPROM.get(EEPROM_UPTIME_ADDR, t);
    if (t >= 1000UL && t <= 120000UL) upTime = t;

    EEPROM.get(EEPROM_DOWNTIME_ADDR, t);
    if (t >= 1000UL && t <= 120000UL) downTime = t;

    DBG_PRINTF("[EEPROM] Loaded: addr=%d, UP=%lu ms, DOWN=%lu ms\n",
               modbusAddress, upTime, downTime);
}

// ============================================================
// EEPROM: Save functions
// ============================================================
void saveTimesToEEPROM() {
    EEPROM.write(EEPROM_MAGIC_ADDR, EEPROM_MAGIC_VALUE);
    EEPROM.put(EEPROM_UPTIME_ADDR, upTime);
    EEPROM.put(EEPROM_DOWNTIME_ADDR, downTime);
    EEPROM.commit();
    DBG_PRINTF("[EEPROM] Saved times: UP=%lu ms, DOWN=%lu ms\n", upTime, downTime);
}

void saveAddressToEEPROM(uint8_t addr) {
    EEPROM.write(EEPROM_MAGIC_ADDR, EEPROM_MAGIC_VALUE);
    EEPROM.write(EEPROM_ADDRESS_ADDR, addr);
    EEPROM.commit();
    DBG_PRINTF("[EEPROM] Saved address: %d\n", addr);
}

// ============================================================
// SOFTWARE RESET
// ============================================================
void softwareReset() {
    DBG_PRINTF("[RESET] Restarting ESP32...\n");
    delay(100); // Allow debug message to flush
    ESP.restart();
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
#if ACS_INSTALLED
    acsMax = 0;
    acsMin = ADC_MAX_VALUE;
    acsWindowStart = millis();
    acsCurrentFlowing = true;   // Assume current at start
    acsStartupGrace = true;     // Ignore readings during startup
#endif
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

    DBG_PRINTF("[ACS] End-stop reached: dir=%s, pos=%d\n",
               lastDirection == DIR_UP ? "UP" : "DOWN", currentPosition);
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
        int16_t peakToPeak = acsMax - acsMin;
        acsCurrentFlowing = (peakToPeak >= ACS_THRESHOLD);

        DBG_PRINTF("[ACS] pp=%d thr=%d flowing=%s grace=%s\n",
                   peakToPeak, ACS_THRESHOLD,
                   acsCurrentFlowing ? "YES" : "NO",
                   acsStartupGrace ? "YES" : "no");

        // Reset for next window
        acsMax = 0;
        acsMin = ADC_MAX_VALUE;
        acsWindowStart = now;
    }

    // End startup grace period
    if (acsStartupGrace && (now - moveStartTime) >= ACS_STARTUP_DELAY_MS) {
        acsStartupGrace = false;
        DBG_PRINTF("[ACS] Startup grace period ended\n");
    }
}

/**
 * Check if mechanical end-stop has been reached.
 * Only valid after startup grace period and during active movement.
 */
bool isEndStopReached() {
#if !ACS_INSTALLED
    return false;              // No ACS → no end-stop detection
#else
    if (acsStartupGrace) return false;
    if (moveMode == MOVE_NONE && calPhase == CAL_IDLE) return false;
    return !acsCurrentFlowing;
#endif
}

// ============================================================
// CALIBRATION STATE MACHINE
// ============================================================

void abortCalibration() {
    relayStop();
    calPhase = CAL_IDLE;
    deviceState = STATE_STOP;
    moveMode = MOVE_NONE;
    DBG_PRINTF("[CAL] Calibration aborted\n");
}

void startCalibration() {
#if !ACS_INSTALLED
    DBG_PRINTF("[CAL] Calibration skipped (ACS not installed)\n");
    return;  // Calibration requires ACS712 sensors
#else
    DBG_PRINTF("[CAL] Starting calibration sequence\n");
    calPhase = CAL_GOING_UP_INIT;
    moveMode = MOVE_CALIBRATE;
    startMotor(DIR_UP);
    deviceState = STATE_CALIBRATING;   // Override startMotor's state
    calPhaseStart = millis();
#endif
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
            DBG_PRINTF("[CAL] Phase: GOING_DOWN (measuring DOWN_TIME)\n");
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
            DBG_PRINTF("[CAL] Phase: GOING_UP_FINAL (measuring UP_TIME)\n");
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
        DBG_PRINTF("[CAL] Safety timeout! Phase=%d\n", calPhase);
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
            DBG_PRINTF("[CAL] Top end-stop reached, pausing...\n");
            calPhase = CAL_PAUSE_1;
            calPhaseStart = now;
            break;

        case CAL_GOING_DOWN:
            // Reached bottom end-stop → record DOWN_TIME
            downTime = now - calMeasureStart;
            relayStop();
            DBG_PRINTF("[CAL] Bottom end-stop reached, DOWN_TIME=%lu ms\n", downTime);
            calPhase = CAL_PAUSE_2;
            calPhaseStart = now;
            break;

        case CAL_GOING_UP_FINAL:
            // Reached top end-stop → record UP_TIME, save, done
            upTime = now - calMeasureStart;
            relayStop();
            DBG_PRINTF("[CAL] Top end-stop reached, UP_TIME=%lu ms\n", upTime);
            saveTimesToEEPROM();
            currentPosition = 0;
            deviceState = STATE_OPEN;
            moveMode = MOVE_NONE;
            calPhase = CAL_IDLE;
            DBG_PRINTF("[CAL] Calibration complete!\n");
            break;
    }
}

// ============================================================
// MODBUS COMMAND PROCESSING
// ============================================================
void processModbus() {
    uint16_t command   = slave->hreg(HREG_COMMAND);
    uint16_t targetPos = slave->hreg(HREG_TARGET_POS);
    uint16_t address   = slave->hreg(HREG_ADDRESS);

    // --- HR 0: Command register ---
    if (command != prevCommand) {
        switch (command) {
            case CMD_STOP:
                DBG_PRINTF("[CMD] STOP\n");
                if (calPhase != CAL_IDLE) {
                    abortCalibration();
                } else if (moveMode != MOVE_NONE) {
                    stopMotor();
                }
                break;

            case CMD_OPEN:
                DBG_PRINTF("[CMD] OPEN\n");
                if (calPhase == CAL_IDLE) {
                    if (moveMode != MOVE_NONE) stopMotor();
                    startMotor(DIR_UP);
                    moveMode = MOVE_AUTO;
                    moveTargetDuration = upTime;
                    targetPosition = -1;
                }
                break;

            case CMD_CLOSE:
                DBG_PRINTF("[CMD] CLOSE\n");
                if (calPhase == CAL_IDLE) {
                    if (moveMode != MOVE_NONE) stopMotor();
                    startMotor(DIR_DOWN);
                    moveMode = MOVE_AUTO;
                    moveTargetDuration = downTime;
                    targetPosition = -1;
                }
                break;

            case CMD_CALIBRATE:
                DBG_PRINTF("[CMD] CALIBRATE\n");
#if ACS_INSTALLED
                if (calPhase != CAL_IDLE) abortCalibration();
                if (moveMode != MOVE_NONE) stopMotor();
                startCalibration();
#else
                DBG_PRINTF("[CMD] Calibration ignored (ACS not installed)\n");
#endif
                break;

            default:
                DBG_PRINTF("[CMD] Unknown command: %d\n", command);
                break;
        }

        // Auto-clear: reset register to STOP so the same command
        // can be sent again without needing to write a different value first
        slave->setHreg(HREG_COMMAND, CMD_STOP);
        prevCommand = CMD_STOP;
    }

    // --- HR 2: Target position register ---
    if (targetPos != prevTargetPos) {
        prevTargetPos = targetPos;

        DBG_PRINTF("[POS] Target position received: %d\n", targetPos);

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

                DBG_PRINTF("[POS] Moving %s: %d -> %d (%lu ms)\n",
                           direction == DIR_UP ? "UP" : "DOWN",
                           current, target, moveTargetDuration);
            } else {
                DBG_PRINTF("[POS] Already at target, ignoring\n");
            }
        }
    }

    // --- HR 4: Address register ---
    if (address != prevAddress) {
        prevAddress = address;

        if (address >= MIN_ADDRESS && address <= MAX_ADDRESS &&
            (uint8_t)address != modbusAddress) {
            DBG_PRINTF("[ADDR] Changing address: %d -> %d\n", modbusAddress, address);
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
        DBG_PRINTF("[BTN] Both pressed: full travel %s\n",
                   dir == DIR_UP ? "UP" : "DOWN");
        return;
    }

    // --- Single button pressed ---
    uint8_t pressedDir = btnUp ? DIR_UP : DIR_DOWN;

    // If auto-move is active → stop (toggle behavior)
    if (moveMode == MOVE_AUTO) {
        stopMotor();
        waitFlag = true;
        DBG_PRINTF("[BTN] Auto-move interrupted\n");
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
    // --- Debug serial (UART0 / USB) ---
    Serial.begin(115200);
    WiFi.disconnect(true);
  
    // Turn off the Wi-Fi radio
    WiFi.mode(WIFI_OFF);
    delay(500); // Allow USB serial to initialize
    DBG_PRINTF("\n========================================\n");
    DBG_PRINTF(" ROLLER SHUTTER CONTROLLER v2.0\n");
    DBG_PRINTF(" MCU: ESP32-C3\n");
    DBG_PRINTF("========================================\n");

    // --- Relay pins OFF first ---
    pinMode(UP_RELAY_PIN, OUTPUT);
    pinMode(DOWN_RELAY_PIN, OUTPUT);
    digitalWrite(UP_RELAY_PIN, LOW);
    digitalWrite(DOWN_RELAY_PIN, LOW);
    DBG_PRINTF("[BOOT] Relays initialized (OFF)\n");

    // --- Buttons with internal pull-ups ---
    upButton.attach(UP_BTN_PIN, INPUT_PULLUP);
    upButton.interval(DEBOUNCE_MS);
    downButton.attach(DOWN_BTN_PIN, INPUT_PULLUP);
    downButton.interval(DEBOUNCE_MS);
    DBG_PRINTF("[BOOT] Buttons initialized (A2/A3 with pull-up)\n");

    // --- EEPROM (flash-backed) ---
    EEPROM.begin(EEPROM_SIZE);
    loadFromEEPROM();
    DBG_PRINTF("[BOOT] Config: addr=%d, UP=%lu ms, DOWN=%lu ms\n",
               modbusAddress, upTime, downTime);

    // --- Modbus RTU (UART1) ---
    slave = new ModbusSerial(modbusSerial, modbusAddress, -1);
    slave->config(9600);
    modbusSerial.begin(9600, SERIAL_8N1, MODBUS_RX_PIN, MODBUS_TX_PIN);
    slave->setAdditionalServerData("BLIND");

    // Register holding registers
    for (uint8_t i = 0; i < HREG_COUNT; i++) {
        slave->addHreg(i);
    }

    // Set initial output register values
    slave->setHreg(HREG_STATE, deviceState);
    slave->setHreg(HREG_CURRENT_POS, currentPosition);
    slave->setHreg(HREG_ADDRESS, modbusAddress);
    slave->setHreg(HREG_UPTIME, (uint16_t)(upTime / 100UL));
    slave->setHreg(HREG_DOWNTIME, (uint16_t)(downTime / 100UL));

    // Initialize change detection baselines
    prevCommand = CMD_STOP;
    prevTargetPos = 0;
    prevAddress = modbusAddress;

    DBG_PRINTF("[BOOT] Modbus initialized: addr=%d, UART1 RX=%d TX=%d\n",
               modbusAddress, MODBUS_RX_PIN, MODBUS_TX_PIN);

    DBG_PRINTF("[BOOT] Setup complete. ACS installed: %s\n",
               ACS_INSTALLED ? "YES" : "NO");
    DBG_PRINTF("========================================\n\n");
}

// ============================================================
// MAIN LOOP
// ============================================================
void loop() {
    //esp_task_wdt_reset();

    // 1. Process incoming Modbus frames
    slave->task();

    // 2. Update ACS712 current sensing (only when motor is active)
#if ACS_INSTALLED
    if (moveMode != MOVE_NONE || calPhase != CAL_IDLE) {
        updateACS();
    }
#endif

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
                DBG_PRINTF("[MOVE] Position target reached: %d%%\n", currentPosition);
            } else {
                // Full travel completed → set end position
                if (lastDirection == DIR_UP) {
                    currentPosition = 0;
                    deviceState = STATE_OPEN;
                } else {
                    currentPosition = 100;
                    deviceState = STATE_CLOSED;
                }
                DBG_PRINTF("[MOVE] Full travel completed: pos=%d\n", currentPosition);
            }
            moveMode = MOVE_NONE;
            targetPosition = -1;
        }
    }

    // 7. Check manual movement safety timeout
    if (moveMode == MOVE_MANUAL) {
        if ((millis() - moveStartTime) >= moveTargetDuration) {
            DBG_PRINTF("[MOVE] Manual safety timeout\n");
            stopMotor();
        }
    }

    // 8. Check ACS712 end-stop detection (normal movement only)
#if ACS_INSTALLED
    if ((moveMode == MOVE_AUTO || moveMode == MOVE_MANUAL) && isEndStopReached()) {
        stopMotorAtEndStop();
    }
#endif

    // 9. Update Modbus output registers
    slave->setHreg(HREG_STATE, deviceState);
    slave->setHreg(HREG_CURRENT_POS, (uint16_t)getEstimatedPosition());
    slave->setHreg(HREG_ADDRESS, modbusAddress);
    slave->setHreg(HREG_UPTIME, (uint16_t)(upTime / 100UL));
    slave->setHreg(HREG_DOWNTIME, (uint16_t)(downTime / 100UL));

    // 10. Debug: print state/position changes
#if DEBUG_ENABLED
    uint8_t curState = deviceState;
    int16_t curPos = getEstimatedPosition();
    if (curState != prevDebugState || curPos != prevDebugPos) {
        static const char* stateNames[] = {
            "STOP", "OPENING", "OPEN", "CLOSING", "CLOSED", "CALIBRATING"
        };
        const char* name = (curState <= 5) ? stateNames[curState] : "?";
        DBG_PRINTF("[REG] state=%d(%s) pos=%d%%\n", curState, name, curPos);
        prevDebugState = curState;
        prevDebugPos = curPos;
    }
#endif
}
