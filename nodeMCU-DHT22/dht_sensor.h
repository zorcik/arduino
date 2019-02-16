// Example testing sketch for various DHT humidity/temperature sensors
// Written by ladyada, public domain

#include "DHT.h"

#define DHTPIN D4     // what digital pin we're connected to

// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

// Connect pin 1 (on the left) of the sensor to +5V
// NOTE: If using a board with 3.3V logic like an Arduino Due connect pin 1
// to 3.3V instead of 5V!
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

// Initialize DHT sensor.
// Note that older versions of this library took an optional third parameter to
// tweak the timings for faster processors.  This parameter is no longer needed
// as the current DHT reading algorithm adjusts itself to work on faster procs.
DHT dht(DHTPIN, DHTTYPE);

struct DHT_data {
  float temp = 0;
  float hum = 0;
} DHT_readings;

float dht_temp;

void setup_dht() {
  dht.begin();
}

void read_DHT() {
  float temp = 0;
  float hum = 0;
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  hum = dht.readHumidity();
  // Read temperature as Celsius (the default)
  temp = dht.readTemperature();

//  Serial.print("Temp: ");
//  Serial.print(temp);
//  Serial.print("Hum: ");
//  Serial.println(hum);

  // Check if any reads failed and exit early (to try again).
  if (isnan(hum) || isnan(temp) ) {
//    Serial.println("Failed to read from DHT sensor!");
    return;
  }

//  Serial.println("Reading OK");

  DHT_readings.hum = hum;
  DHT_readings.temp = temp;
}

