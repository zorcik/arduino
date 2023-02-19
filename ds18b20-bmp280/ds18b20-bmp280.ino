#include <OneWire.h>
#include <DS18B20.h>
#include <ModbusRtu.h>
#include <Adafruit_BMP280.h>
#include <BMP280_DEV.h>
#include <EEPROM.h>

#define ID   14
// Numer pinu cyfrowego do którego podłaczyłęś czujniki
const byte ONEWIRE_PIN = 5;
BMP280_DEV bmp;

OneWire onewire(ONEWIRE_PIN);
DS18B20 sensor(&onewire);

Modbus slave;
byte sensors[8];

float difference = 0;
bool calculateDifference = false;

// aby dokonać kalibracji należy zewrzeć pin8 z masą podczas uruchomienia

void setup()
{
  pinMode(7, OUTPUT);
  pinMode(8, INPUT_PULLUP);
  bool resetEEPROM = digitalRead(8);
  if (!resetEEPROM)
  {
    calculateDifference = true;
  }
  else
  {
    EEPROM.get(0, difference);
  }
  
  digitalWrite(7, HIGH);
  delay(100);
  discover();
  slave = Modbus(ID, Serial, 2);
  slave.begin(9600);

  sensor.begin();
  sensor.request();

  if (!bmp.begin(BMP280_I2C_ALT_ADDR)) {
    digitalWrite(7, LOW);
    while (1);
  }

  bmp.setTimeStandby(TIME_STANDBY_500MS);
  bmp.startNormalConversion();
}

uint16_t au16data[3];

void discover()
{
  byte address[8];
  int a = 0;
  int addr = 0;
  onewire.reset_search();
  while(onewire.search(address))
  {
    if (address[0] != 0x28)
      continue;

    if (OneWire::crc8(address, 7) != address[7])
    {
      break;
    }

    for (byte i=0; i<8; i++)
    {
      sensors[i] = address[i];
    }
    return;
  }

}

float temperature1 = 100;
float temperature2 = 100;
float pressure = 100;

void getTemperature()
{
  if (sensor.available())
  {
    temperature1 = sensor.readTemperature(sensors);
  }

  sensor.request();
}

void getBMP()
{
  bmp.getTempPres(temperature2, pressure);
}

static const unsigned long REFRESH_INTERVAL = 1000; // ms
static unsigned long lastRefreshTime = 0;
int8_t state = 0;
int calcInterval = 0;

 
void loop()
{
  if(millis() - lastRefreshTime >= REFRESH_INTERVAL)
  {
    lastRefreshTime += REFRESH_INTERVAL;
    getTemperature();
    getBMP();
    if (calculateDifference && calcInterval++ > 5)
    {
      difference = temperature1 - temperature2;
      EEPROM.put(0, difference);
      calculateDifference = false;
    }
    temperature1 -= difference;
  }

  au16data[0] = round(temperature1*100);
  au16data[1] = round(temperature2*100);
  au16data[2] = round(pressure*100);
  au16data[3] = slave.getInCnt();
  
  state = slave.poll( au16data, 4 );
}
