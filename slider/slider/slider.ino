#include <Stepper.h>

const int stepsPerRevolution = 20; 
Stepper myStepper(stepsPerRevolution, D1,D2,D3,D4);

void setup() {
  // put your setup code here, to run once:
  myStepper.setSpeed(60);
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println("clockwise");
  myStepper.step(stepsPerRevolution);
  delay(500);
 
  Serial.println("counterclockwise");
  myStepper.step(-stepsPerRevolution);
  delay(500); 
}
