void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(2, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  
  digitalWrite(2, HIGH);
  Serial.println("Test");
  digitalWrite(2, LOW);
  delay(1000);
}
