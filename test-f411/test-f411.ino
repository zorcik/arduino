/*
  Blink onboard LED at 0.1 second interval
*/
void setup() {
  // initialize digital pin PB2 as an output.
  pinMode(PC13, OUTPUT); // LED connect to pin PC13
}
void loop() {
  digitalWrite(PC13, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(100);               // wait for 100mS
  digitalWrite(PC13, LOW);    // turn the LED off by making the voltage LOW
  delay(100);               // wait for 100mS
}
