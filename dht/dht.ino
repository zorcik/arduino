#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>
#include <DHT.h>
#include <SPI.h>
#include <avr/sleep.h>
#include <avr/wdt.h>


#define DHTPIN A2

DHT dht;

// Pins for radio
const int rf_ce = 9;
const int rf_csn = 10;
const int voltage_pin = A3;
const int num_measurements = 64;
const uint32_t nodeNumber = 0xFF000001;

const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

// watchdog interrupt
ISR (WDT_vect) 
{
   wdt_disable();  // disable watchdog
}  // end of WDT_vect

void setup() {
  dht.setup(DHTPIN);
  #ifdef INTERNAL1V1
    analogReference(INTERNAL1V1);
  #else
    analogReference(INTERNAL);
  #endif
}

int measure_voltage()
{
    // Take the voltage reading 
    int i = num_measurements;
    uint32_t reading = 0;
    while(i--)
      reading += analogRead(voltage_pin);

    // Convert the voltage reading to volts*256
    return (reading / num_measurements);
}

int counter = 0;

void sendMessage(String message)
{
  RF24 radio(rf_ce,rf_csn);
  digitalWrite (SS, HIGH);
  SPI.begin ();
  digitalWrite (rf_ce, LOW); 
  digitalWrite (rf_csn, HIGH);
  radio.begin();
  // optionally, increase the delay between retries & # of retries
  radio.setRetries(15, 15);

  // optionally, reduce the payload size.  seems to improve reliability
  radio.setPayloadSize(8);

  radio.openWritingPipe (pipes[0]);
  radio.openReadingPipe (1, pipes[1]);
  bool ok = radio.write (&message, sizeof message);
  
  radio.startListening ();
  radio.powerDown ();
  
  SPI.end ();
}

void loop() {

  if ((++counter & 7) == 0)
  {
    float t = dht.getTemperature();
    float h = dht.getHumidity();
   
    // Sprawdzamy poprawność danych
    if (dht.getStatus())
    {
       delay(dht.getMinimumSamplingPeriod());  
    } 
    else
    {
      // pomiar wartości napięcia baterii, tutaj bo pomiar temperatury zużył baterię
      int voltage = measure_voltage();
  
      // tworzymy komunikat
      // nodeId[4], kod[2], temp[2], hum[2], voltage[1]
      String temp = String((uint16_t)(t*100));
      if (temp.length() == 1) temp = "0"+temp;
      String humid = String((uint16_t)(h*100));
      if (humid.length() == 1) humid = "0"+humid;
      
      String message = ""+String(nodeNumber)+"T1"+temp+humid+String(voltage);
      
      // wysyłamy wartości przez NRF
      
      sendMessage(message);
    }
  }  
    // arduino sleep
    // clear various "reset" flags
    MCUSR = 0;     
    // allow changes, disable reset
    WDTCSR = bit (WDCE) | bit (WDE);
    // set interrupt mode and an interval 
    WDTCSR = bit (WDIE) | bit (WDP3) | bit (WDP0);    // set WDIE, and 8 seconds delay
    wdt_reset();  // pat the dog
    
    set_sleep_mode (SLEEP_MODE_PWR_DOWN);  
    noInterrupts ();           // timed sequence follows
    sleep_enable();   

    // cancel sleep as a precaution
    sleep_disable();
}
