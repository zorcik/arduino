
/* 
 Stepper Motor Control - one revolution
 
 This program drives a unipolar or bipolar stepper motor. 
 The motor is attached to digital pins 8 - 11 of the Arduino.
 
 The motor should revolve one revolution in one direction, then
 one revolution in the other direction.  
 
  
 Created 11 Mar. 2007
 Modified 30 Nov. 2009
 by Tom Igoe
 
 */

int del = 5;          

void setup() {
  
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  
  
  digitalWrite(9, HIGH);
  
}

void loop() {
  
  digitalWrite(10, HIGH);
  
  for (int i=0; i<200; i++)
  {
    digitalWrite(8, HIGH);
    delay(del);
    digitalWrite(8, LOW);
    delay(del);
  }

  delay(1000);
  
  digitalWrite(10, LOW);
  
  for (int i=0; i<200; i++)
  {
    digitalWrite(8, HIGH);
    delay(del);
    digitalWrite(8, LOW);
    delay(del);
  }
  
  delay(1000);
}

