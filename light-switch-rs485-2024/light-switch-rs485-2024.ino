// wersja płytki 3.1, w 3.0 połączyć D13 z A6
#include <ModbusSerial.h>
#include <Bounce2.h>

#define NUM_BUTTONS 8
const uint8_t BUTTON_PINS[NUM_BUTTONS] = {A0, A1, A2, A3, A4, A5, 2, 3};
const uint8_t OUT_PINS[NUM_BUTTONS] = {4, 5, 6, 7, 8, 9, 10, 13};
uint8_t STATES[NUM_BUTTONS] = {LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW};

int address = 50;
Bounce * buttons = new Bounce[NUM_BUTTONS];
ModbusSerial mb (Serial, address, -1);

void setup()
{
  Serial.begin(9600, MB_PARITY_EVEN);
  mb.config (9600);
  mb.setAdditionalServerData ("LAMP"); // for Report Server ID function (0x11)
  
  for (int i = 0; i < NUM_BUTTONS; i++) {
    buttons[i].attach( BUTTON_PINS[i] , INPUT_PULLUP  );       //setup the bounce instance for the current button
    buttons[i].interval(25);              // interval in ms
    pinMode(OUT_PINS[i], OUTPUT);
    digitalWrite(OUT_PINS[i], STATES[i]);
    mb.addCoil (i);
    mb.setCoil(i,STATES[i]);
    mb.addIsts(i);
    mb.setIsts(i,STATES[i]);
  }

}

boolean changed = false;

void loop()
{

  mb.task();
  
 for (int i = 0; i < NUM_BUTTONS; i++)  {
    // Update the Bounce instance :
    buttons[i].update();
    // If it fell, flag the need to toggle the LED
    if ( buttons[i].fell() ) {
      STATES[i] = !STATES[i];
      changed = true;
      mb.setCoil(i, STATES[i]);
    }
  }

  for (int i = 0; i < NUM_BUTTONS; i++)  {
    if (mb.Coil(i) != STATES[i])
    {
      STATES[i] = mb.Coil(i);
      changed = true;
    }
  }

  for (int i = 0; i < NUM_BUTTONS; i++)  {
    mb.setIsts(i,STATES[i]);
  }



if (changed)
{
  for (int i = 0; i < NUM_BUTTONS; i++)  {
      digitalWrite(OUT_PINS[i], STATES[i]);
  }
}
    
}
