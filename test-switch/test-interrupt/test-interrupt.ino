const byte ledPin = 13;
const byte interruptPin = 3;
volatile byte state = LOW;

void setup() {
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), blink, CHANGE);
  Serial.println("Start");
}

bool ok = false;

void loop() {

  if (ok)
  {
    Serial.println("Przerwanie");
    ok = false;
  }
  
}

void blink() {
  ok = true;
}
