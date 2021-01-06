//#define MY_RADIO_NRF24
//#define MY_REPEATER_FEATURE
#define MY_GATEWAY_SERIAL

#include <MySensor.h>
#include <SPI.h>
#include <Wire.h>
#include <PCF8574.h>

#define RELAY_ON 0                     
#define RELAY_OFF 1
#define noExpanders 2

class Relay         
{
public:                                     
  int buttonPin;               
  int relayPin;     
  boolean relayState;
  boolean buttonState = 1;
};

PCF8574 relays_exp[noExpanders];
PCF8574 buttons_exp[noExpanders];
Relay Relays[noExpanders][8];

MyMessage msg;

void setup(){
relays_exp[0].begin(0x38);
relays_exp[1].begin(0x39);
buttons_exp[0].begin(0x3B);
buttons_exp[1].begin(0x3F);

for ( int i = 0 ; i < noExpanders ; i++ )
{

  for ( int j = 0 ; j < 8 ; j++ )
  {
    Relays[i][j].buttonPin = j;
    Relays[i][j].relayPin = j;
    Relays[i][j].relayState = loadState(8*i+j); 
    buttons_exp[i].pinMode(j, INPUT_PULLUP);
    wait(100);
    relays_exp[i].pinMode(j, OUTPUT);
    relays_exp[i].digitalWrite(Relays[i][j].relayPin, Relays[i][j].relayState? RELAY_ON:RELAY_OFF);
    msg.setSensor(8*i+j);
    msg.setType(V_LIGHT);
    send(msg.set(Relays[i][j].relayState? true : false));
    wait(50);
}
}
}

void presentation()  {
sendSketchInfo("MultiRelayExpanderWithToggleSwitch", "0.9b");
wait(100);

for ( int i = 0 ; i < noExpanders ; i++ )
{
  for ( int j = 0 ; j < 8 ; j++ )
  {
    present(8*i+j, S_LIGHT);
  }
}
}

void loop()
{
for ( int i = 0 ; i < noExpanders ; i++ )
{
  for ( int j = 0 ; j < 8 ; j++ )
  { 
        if (buttons_exp[i].digitalRead(j) != Relays[i][j].buttonState)
        {
            Relays[i][j].relayState = !Relays[i][j].relayState;
            Relays[i][j].buttonState = !Relays[i][j].buttonState;
            relays_exp[i].digitalWrite(Relays[i][j].relayPin, Relays[i][j].relayState?RELAY_ON:RELAY_OFF);
            msg.setSensor(8*i+j);
            send(msg.set(Relays[i][j].relayState? true : false));
            saveState(8*i+j, Relays[i][j].relayState );
        }                 
   
  }
}
    wait(50);
}
void receive(const MyMessage &message){       
   if (message.type == V_LIGHT)
   {
     if (message.sensor < noExpanders * 8)
     {
       if (message.sensor < 8)
       {
         Relays[0][message.sensor].relayState = message.getBool();
         relays_exp[0].digitalWrite(Relays[0][message.sensor].relayPin, Relays[0][message.sensor].relayState? RELAY_ON:RELAY_OFF);
         saveState( message.sensor, Relays[0][message.sensor].relayState );
       }
       if (message.sensor >= 8)
       {
         Relays[1][message.sensor -8].relayState = message.getBool();
         relays_exp[1].digitalWrite(Relays[1][message.sensor -8].relayPin, Relays[1][message.sensor -8].relayState? RELAY_ON:RELAY_OFF);
         saveState( message.sensor, Relays[1][message.sensor -8].relayState );
       }
    }
   }
  wait(20);
}
