#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

int stp = 13;  // STEP
int dir = 12;  // KIERUNEK

int timeMin = 8;
int timePlus = 9;
int photosMin = 10;
int photosPlus = 11;
int buttonStart = 7;
int zeroTest = 6;

void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  pinMode(stp, OUTPUT);
  pinMode(dir, OUTPUT);   

  pinMode(timeMin, INPUT);
  pinMode(timePlus, INPUT);
  pinMode(photosMin, INPUT);
  pinMode(photosPlus, INPUT);
  pinMode(buttonStart, INPUT);
  pinMode(zeroTest, INPUT);
}

bool inProgress = false;
bool rewinding = false;
int maxSteps = 10000;
int calculatedDelay = 0;
int setSeconds = 3600;
int photos = 720;
int currentStep = 0;
int steps = 0;
int counter = 0;
int DIR = 0;

void start() {
  // liczymy opóźnienia
  calculatedDelay = setSeconds * 1000 / maxSteps;
  steps = maxSteps / photos;
  counter = 0;
  DIR = 0;
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
  delay(1);
  digitalWrite(stp, LOW);
  delay(1);
}

void progressLoop() {
  makeStep();
  delay(calculatedDelay/2);
  if (++counter == steps)
  {
    // takePhoto();
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
  DIR = 1;
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

