#include <RF24.h>
#include "sleep.h"

RF24 radio(8,9);

// node number
uint16_t this_node;

const short num_measurements = 64;
unsigned int message;

// The pin our voltage sensor is on
const short sensor_pin = A0;

// Sleep constants.  In this example, the watchdog timer wakes up
// every 1s, and every 4th wakeup we power up the radio and send
// a reading.  In real use, these numbers which be much higher.
// Try wdt_8s and 7 cycles for one reading per minute.> 1
const wdt_prescalar_e wdt_prescalar = wdt_1s;
const short sleep_cycles_per_transmission = 4;

void setup(void)
{
  this_node = nodeconfig_read();

  Sleep.begin(wdt_prescalar,sleep_cycles_per_transmission);

  radio.begin();
}

void loop(void)
{
    // Take a reading.
    int i = num_measurements;
    message = 0;
    while(i--)
      message += analogRead(sensor_pin); 

    printf_P(PSTR("---------------------------------\n\r"));
    printf_P(PSTR("%lu: APP Sending %lu to %u...\n\r"),millis(),message,1);
    
    // Send it to the base
    RF24NetworkHeader header(/*to node*/ 0, /*type*/ 'S');
    bool ok = network.write(header,&message,sizeof(unsigned long));
    if (ok)
      printf_P(PSTR("%lu: APP Send ok\n\r"),millis());
    else
      printf_P(PSTR("%lu: APP Send failed\n\r"),millis());
     
    // Power down the radio.  Note that the radio will get powered back up
    // on the next write() call.
    radio.powerDown();

    // Sleep the MCU.  The watchdog timer will awaken in a short while, and
    // continue execution here.
    Sleep.go();
  }

  // Listen for a new node address
  nodeconfig_listen();
}