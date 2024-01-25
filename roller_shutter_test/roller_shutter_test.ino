#include <Bounce2.h>

Bounce upButton = Bounce();
Bounce downButton = Bounce();

void setup() {
  // put your setup code here, to run once:
  pinMode(A0, OUTPUT);
  pinMode(A1, OUTPUT);
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);

  upButton.attach(A2, INPUT);
    upButton.interval(25);
    downButton.attach(A3, INPUT);
    downButton.interval(25);

    Serial.begin(9600);
    Serial.setTimeout(100);  

    digitalWrite(2, HIGH);
    Serial.println("OK, dziala");
    digitalWrite(2, LOW);
}

int down = HIGH;
int up = HIGH;

void loop() {
  // put your main code here, to run repeatedly:
  upButton.update();
    downButton.update();

    if (upButton.changed())
    {
        up = upButton.read();
    }

    if (downButton.changed())
    {
        down = downButton.read();
    }

    if (up)
    {
      digitalWrite(A0, LOW);
    }
    else
    {
      digitalWrite(A0, HIGH);
    }

    if (down)
    {
      digitalWrite(A1, LOW);
    }
    else
    {
      digitalWrite(A1, HIGH);
    }

    digitalWrite(2, HIGH);
    Serial.println("OK, dziala");
    digitalWrite(2, LOW);
    delay(1000);
    
}
