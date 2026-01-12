#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <SPI.h>
#include <RF24.h>
#include <nRF24L01.h>

// https://forum.arduino.cc/t/simple-nrf24l01-2-4ghz-transceiver-demo/405123/4

RF24 radio(7,8); // CE, CSN 7, 8

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 0;

/*

    Pakiet:

    0:
        10 - identyfikacja
        11 - start gry
        12 - koniec gry
        13 - pobierz statystyki
        14 - ustawienia podów

    1:  ID [0-10] - numer PODa ( 0 - bez znaczenia, np start/koniec/statystyki)
    2:  KOLOR R [0,1]
    3:  KOLOR G [0,1]
    4:  KOLOR B [0,1]
    5:  CZAS gry w minutach
    6:  + CZAS gry w sekundach
    7:  CZAS zapalenia PODa w sekundach
    8:  CZAS wyłączenia PODa w sekunach po zgaśnięci / wciśnięciu
    

*/


// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

// TODO: change UUIDs
#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

const byte address[10] = "ADDRESS01";

#define LEDPIN_R 5
#define LEDPIN_G 9
#define LEDPIN_B 10

void processMessage(char message[])
{
    if (message[0] == 0)
    {
       Serial.println("Got 0");    
    }
    Serial.print("Got value: ");
    Serial.print(message[0], HEX);
    Serial.print(" , ");
    Serial.print(message[1], HEX);
    Serial.println(" END");
}

class MyServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer)
    {
        deviceConnected = true;
    };
    void onDisconnect(BLEServer *pServer)
    {
        deviceConnected = false;
    }
};

class MyCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic) override
    {
        String rxValueString = pCharacteristic->getValue();
        //String rxValueString = String(rxValue.c_str());
        if (rxValueString.length() > 0)
        {
            Serial.println(rxValueString);
            if (digitalRead(LEDPIN_R) == HIGH)
            {
              digitalWrite(LEDPIN_R, LOW);
            }
            else
            {
              digitalWrite(LEDPIN_R, HIGH);
            }
            char charBuf[20];
            rxValueString.toCharArray(charBuf, 20);
            processMessage(charBuf);
        }
    }
};

boolean setupBluetooth()
{
    // Create the BLE Device
    BLEDevice::init("POD Trainer");

    // Create the BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create the BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Create a BLE Characteristic
    pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        BLECharacteristic::PROPERTY_NOTIFY);

    pTxCharacteristic->addDescriptor(new BLE2902());

    BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        BLECharacteristic::PROPERTY_WRITE);

    pRxCharacteristic->setCallbacks(new MyCallbacks());

    // Start the service
    pService->start();

    // Start advertising
    pServer->getAdvertising()->start();
    Serial.println("Waiting a client connection to notify...");

    return true;
}

void loopBluetooth()
{
    if (deviceConnected)
    {
        // case: device connected
        std::string value = std::to_string(txValue);
        Serial.println(value.c_str());
        pTxCharacteristic->setValue(value.c_str());
        pTxCharacteristic->notify(); 
        txValue++;
        delay(10); // bluetooth stack will go into congestion, if too many packets are sent
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected)
    {
        // case: device disconnected
        delay(500);                  // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected)
    {
        // do stuff here on first connect
        oldDeviceConnected = deviceConnected;
    }
    delay(1000);
}

void sendMessage(String message)
{
    pTxCharacteristic->setValue(message.c_str());
    pTxCharacteristic->notify();
}

void messageHandler(String message)
{
    // ADD YOUR CODE HERE
    Serial.println(message);
}

void setup()
{
  Serial.begin(9600);
  setupBluetooth();
  pinMode(LEDPIN_R, OUTPUT);
  pinMode(LEDPIN_G, OUTPUT);
  pinMode(LEDPIN_B, OUTPUT);
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MIN);
  radio.stopListening();
}

void loop()
{
  loopBluetooth();
}
