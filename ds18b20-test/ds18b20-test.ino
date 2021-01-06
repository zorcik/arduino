#include <OneWire.h>
#include <DS18B20.h>
#include <EEPROM.h>
#include <ModbusRtu.h>
#define ID   11
// Numer pinu cyfrowego do którego podłaczyłęś czujniki
const byte ONEWIRE_PIN = 5;

OneWire onewire(ONEWIRE_PIN);
DS18B20 sensor(&onewire);

Modbus slave;
byte sensors[2][8];


void getAddressFromEEPROM()
{
  int addr = 0;
  for (int i=0; i<2; i++)
  {
    for (int k=0; k<8; k++)
    {
      sensors[i][k] = EEPROM.read(addr);
      addr++;
    }
  }

  if (sensors[0][0] != 0x28)
  {
    discover();
  }
  else
  {
    //Serial.println("Found addresses in EEPROM");
  }
}

void setup()
{
  pinMode(7, INPUT_PULLUP);
  delay(100);
  int resetEEPROM = digitalRead(7);
  if (!resetEEPROM)
  {
    int addr = 0;
    for (int i=0; i<16; i++)
    {
      EEPROM.write(addr, 0);
    }
  }
  slave = Modbus(ID, Serial, 2);
  slave.begin(9600);
  getAddressFromEEPROM();
  //slave.start();

  sensor.begin();
  sensor.request();
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
      //Serial.println(F("Błędny adres, sprawdz polaczenia"));
      break;
    }

    for (byte i=0; i<8; i++)
    {
      Serial.print(F("0x"));
      Serial.print(address[i], HEX);

      if (i < 7)
        Serial.print(F(", "));

      sensors[a][i] = address[i];
      EEPROM.write(addr, address[i]);
      addr++;
    }
    a++;
    Serial.println();
    if (a == 2)
    {
      return;
    }
  }

}

float temperature1 = 100;
float temperature2 = 100;

void getTemperature()
{
  if (sensor.available())
  {
    temperature1 = sensor.readTemperature(sensors[0]);

//    Serial.print("Temp1: ");
//    Serial.print(temperature1);
//    Serial.println(F(" 'C"));

    temperature2 = sensor.readTemperature(sensors[1]);

//    Serial.print("Temp2: ");
//    Serial.print(temperature2);
//    Serial.println(F(" 'C"));
  }

  sensor.request();
}

static const unsigned long REFRESH_INTERVAL = 1000; // ms
static unsigned long lastRefreshTime = 0;
int8_t state = 0;
  
void loop()
{
  if(millis() - lastRefreshTime >= REFRESH_INTERVAL)
  {
    lastRefreshTime += REFRESH_INTERVAL;
    getTemperature();
  }

  au16data[0] = round(temperature1*100);
  au16data[1] = round(temperature2*100);
  au16data[2] = slave.getInCnt();
  
  state = slave.poll( au16data, 3 );
}
