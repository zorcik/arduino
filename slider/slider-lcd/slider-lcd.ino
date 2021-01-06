#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

int stp = A0;  // STEP
int dir = 13;  // KIERUNEK
int enable = A1; // ENABLE

int timeMin = 10;
int timePlus = 6;
int photosMin = 9;
int photosPlus = 8;
int buttonStart = 7;
int zeroTest = 5;
int photoSwitch = 4;

void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  pinMode(stp, OUTPUT);
  pinMode(dir, OUTPUT);   
  pinMode(enable, OUTPUT);   
  pinMode(photoSwitch, OUTPUT);

  digitalWrite(enable, LOW);
  digitalWrite(photoSwitch, LOW);
  digitalWrite(stp, LOW);
  digitalWrite(dir, LOW);

  pinMode(timeMin, INPUT);
  pinMode(timePlus, INPUT);
  pinMode(photosMin, INPUT);
  pinMode(photosPlus, INPUT);
  pinMode(buttonStart, INPUT);
  pinMode(zeroTest, INPUT);
}

bool inProgress = false;
bool rewinding = false;
long maxSteps = 10000;
int calculatedDelay = 0;
int setSeconds = 3600;
int photos = 720;
long currentStep = 0;
long steps = 0;
long counter = 0;

void start() {
  // liczymy opóźnienia
  calculatedDelay = setSeconds * 1000 / maxSteps;
  steps = maxSteps / photos;
  counter = 0;
  inProgress = true;
}

void stopShooting() {
  inProgress = false;
  rewinding = true;
}

String niceTime() {
  int minutes = setSeconds/60;
  int hours = minutes/60;
  minutes = minutes - (hours*60);
  return ""+String(hours)+"h "+String(minutes)+"min";
}

void redrawLCD() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  // pierwsza linia
  display.setCursor(0,0);
  display.print("Photos: "); 
  display.print(photos); 
  display.setCursor(0,10);
  display.print("Time: "); 
  display.print(niceTime()); 
  display.setCursor(0,20);
  if (inProgress)
  {
    display.print("STARTED"); 
  }
  else if (rewinding)
  {
    display.print("GOING BACK"); 
  }
  else
  {
    display.print("STOPPED"); 
  }
  display.display();
}

void makeStep() {
  digitalWrite(stp, HIGH);
  delay(5);
  digitalWrite(stp, LOW);
  delay(5);
}

void takePhoto() {
  digitalWrite(photoSwitch, HIGH);
  delay(100);
  digitalWrite(photoSwitch, LOW);
}

void progressLoop() {
  digitalWrite(dir, LOW);  
  makeStep();
  delay(calculatedDelay/2);
  if (++counter == steps)
  {
    takePhoto();
    counter = 0;
  }
  delay(calculatedDelay/2);
  currentStep++;
  if (currentStep == maxSteps)
  {
    stopShooting();
  }

  if (digitalRead(buttonStart) == HIGH)
  {
    delay(50);
    inProgress = false;
    stopShooting();
  }  
}

void rewindingLoop() {
  digitalWrite(dir, HIGH);
  while (digitalRead(zeroTest) == LOW)
  {
    makeStep();
    delay(10);
  }
  rewinding = false;
  inProgress = false;
}

void stopLoop() {
  if (digitalRead(buttonStart) == HIGH)
  {
    delay(50);
    start();
  }
  
  if (digitalRead(timeMin) == HIGH)
  {
    delay(50);
    setSeconds -= 60;
    if (setSeconds < 500)
    {
      setSeconds = 500;
    }
  }
  
  if (digitalRead(timePlus) == HIGH)
  {
    delay(50);
    setSeconds += 60;
    if (setSeconds > 86400)
    {
      setSeconds = 86400;
    }
  }
  
  if (digitalRead(photosMin) == HIGH)
  {
    delay(50);
    photos -= 24;
    if (photos < 24)
    {
      photos = 24;
    }
  }
  
  if (digitalRead(photosPlus) == HIGH)
  {
    delay(50);
    photos += 24;
    if (photos > 7200)
    {
      photos = 7200;
    }
  }
  
}

void loop() {
  redrawLCD();

  if (inProgress)
  {
    progressLoop();
  }
  else if (rewinding)
  {
    rewindingLoop();
  }
  else
  {
    stopLoop();
  }
}

