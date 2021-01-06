#include <Bounce2.h>
#include <Adafruit_PCD8544.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SPITFT.h>
#include <Adafruit_SPITFT_Macros.h>
#include <gfxfont.h>

#define MENU_LENGTH 6
#define MENU_ROW_HEIGHT 11
#define LCD_ROWS  4

int timerMode = 0;
int timerDelay = 0;
unsigned long maxStopwatch = 5;

boolean backlight = true;
int contrast = 50;
int test = 0;
int menuitem = 1;
int page = 1;
bool startState = LOW;
bool menuState = LOW;
int shotIndex = 0;

unsigned long startMillis;
unsigned long currentMillis;
unsigned long elapsedMillis;
String durMilliSec;
String shotsStrings[50];
long shotTimes[50];
int shots = 0;

const byte startButton = 8;
Bounce startDebouncer1 = Bounce();

const byte menuButton = 9;
Bounce menuDebouncer = Bounce();

const byte upButton = 10;
Bounce upDebouncer = Bounce();

const byte downButton = 11;
Bounce downDebouncer = Bounce();

const byte shotButton = 2;
Bounce shotDebouncer3 = Bounce();


Adafruit_PCD8544 display = Adafruit_PCD8544(7, 6, 5, 4, 3);

void setup() {
  // put your setup code here, to run once:
  pinMode(8, OUTPUT);
  pinMode(2, INPUT);
  pinMode(A0, OUTPUT);
  pinMode(12, OUTPUT);

  digitalWrite(12, HIGH); //Turn Backlight ON
  digitalWrite(1, LOW);

  display.begin();
  display.setContrast(contrast); //Set contrast to 50
  display.clearDisplay();
  display.display();

  pinMode(startButton, INPUT_PULLUP);
  // After setting up the button, setup the Bounce instance :
  startDebouncer1.attach(startButton);
  startDebouncer1.interval(5); // interval in ms

  pinMode(menuButton, INPUT_PULLUP);
  // After setting up the button, setup the Bounce instance :
  menuDebouncer.attach(menuButton);
  menuDebouncer.interval(5); // interval in ms

  pinMode(upButton, INPUT_PULLUP);
  // After setting up the button, setup the Bounce instance :
  upDebouncer.attach(upButton);
  upDebouncer.interval(5); // interval in ms

  pinMode(downButton, INPUT_PULLUP);
  // After setting up the button, setup the Bounce instance :
  downDebouncer.attach(downButton);
  downDebouncer.interval(5); // interval in ms

  pinMode(shotButton, INPUT);
  // After setting up the button, setup the Bounce instance :
  shotDebouncer3.attach(shotButton);
  shotDebouncer3.interval(5); // interval in ms

  firstInfo();
}

void firstInfo()
{
  display.setTextSize(1);
  display.clearDisplay();
  display.setTextColor(BLACK, WHITE);
  display.setCursor(0, 0);
  display.print("WCISNIJ START");
  display.display();
}

void waitInfo()
{
  display.setTextSize(1);
  display.clearDisplay();
  display.setTextColor(BLACK, WHITE);
  display.setCursor(0, 0);
  display.print("CZEKAJ...");
  display.display();
}

void clearShots()
{
  shots = 0;
  for (int i = 0; i < 50; i++)
  {
    shotsStrings[i] = "";
  }
}

void generateDelay()
{
  int del = 0;
  if (timerDelay == 0)
  {
    del = random(1000,4000);
  }
  else if (timerDelay == 1)
  {
    del = random(4000,10000);
  }
  else
  {
    del = timerDelay * 1000;
  }
  delay(del);
}

void loop() {
  // put your main code here, to run repeatedly:

  startDebouncer1.update();

  if ( startDebouncer1.fell() ) {     // Call code if button transitions from HIGH to LOW
    startState = !startState;         // Toggle start button state
    if (startState)
    {
      clearShots();
      waitInfo();
      generateDelay();
      startMillis = millis();
      tone(A0, 2000, 500);
    }
  }

  if (startState)
  {
    timerStarted();
  }
  else
  {
    menuDebouncer.update();
    if (menuDebouncer.fell())
    {
      menuState = HIGH;
      while (menuState)
      {
        drawMenu();
        menuLoop();
      }
    }
  }
}


int menuPos = 0;
int menuScreen = 0;
int markerPos = 0;
int menuStartAt = 0;

String menu[6] = {
  "Tryb",
  "Delay",
  "Kontrast",
  "Stoper",
  "LED",
  "Exit",
};

void handleUpButton()
{
  upDebouncer.update();
  if (upDebouncer.fell())
  { 
    if (page == 2 && menuPos == 0) // tryb
    {
      timerMode--;
      if (timerMode == 0)
      {
        timerMode = 1;
      }
    }
    else if (page == 2 && menuPos == 1) // delay
    {
      timerDelay--;
      if (timerDelay < 0)
      {
        timerDelay = 0;
      }
    }
    else if (page == 2 && menuPos == 2) // kontrast
    {
      contrast--;
      if (contrast < 1)
      {
        contrast = 1;
      }
      setContrast();
    }
    else if (page == 2 && menuPos == 3) // stoper
    {
      maxStopwatch--;
      if (maxStopwatch < 1)
      {
        maxStopwatch = 1;
      }
    }
    else if (page == 1 && menuPos > 0) {
      menuPos--;

      if (menuStartAt > 0) {
        menuStartAt--;
      }

      drawMenu();
    }

    delay(100);
  }
}

void handleDownButton()
{
  downDebouncer.update();
  if (downDebouncer.fell())
  {
    if (page == 2 && menuPos == 0) // tryb
    {
      timerMode++;
      if (timerMode > 1)
      {
        timerMode = 0;
      }
    }
    else if (page == 2 && menuPos == 1) // delay
    {
      timerDelay++;
      if (timerDelay > 30)
      {
        timerDelay = 30;
      }
    }
    else if (page == 2 && menuPos == 2) // kontrast
    {
      contrast++;
      if (contrast > 99)
      {
        contrast = 99;
      }
      setContrast();
    }
    else if (page == 2 && menuPos == 3) // stoper
    {
      maxStopwatch++;
      if (maxStopwatch > 100)
      {
        maxStopwatch = 100;
      }
    }    
    else if (page == 1 && menuPos < MENU_LENGTH - 1) {
      menuPos++;

      if (menuPos > 3) {
        menuStartAt++;
      }
    }

    drawMenu();
    delay(100);
  }
}

void menuLoop()
{
  handleUpButton();
  
  handleDownButton();

  menuDebouncer.update();
  if (menuDebouncer.fell())
  {
    if (page == 2)
    {
      page = 1;
    }
    else if (menuPos == 0 || menuPos == 1 || menuPos == 2 || menuPos == 3)
    {
      page = 2;
    }
    else if (menuPos == 4) // LED
    {
      backlight = !backlight;
      turnBacklight();
    }
    else if (menuPos == 5) //EXIT
    {
      menuState = LOW;
      firstInfo();
    }
  }
}

void drawMenu()
{
  display.clearDisplay();
  display.setTextSize(1);  

  if (page == 1)
  {
    for (int i = menuStartAt; i < (menuStartAt + LCD_ROWS); i++) {
      int markerY = (i - menuStartAt) * MENU_ROW_HEIGHT;
  
      if (i == menuPos) {
        display.setTextColor(WHITE, BLACK);
        display.fillRect(0, markerY, display.width(), MENU_ROW_HEIGHT, BLACK);
      } else {
        display.setTextColor(BLACK, WHITE);
        display.fillRect(0, markerY, display.width(), MENU_ROW_HEIGHT, WHITE);
      }
  
      if (i >= MENU_LENGTH) {
        continue;
      }
  
      display.setCursor(2, markerY + 2);
      display.print(menu[i]);
    }
  }
  else
  {
    display.setTextColor(BLACK, WHITE);
    display.setCursor(15, 0);
    if (menuPos == 0) // tryb
    {
      display.print("TRYB");
      display.drawFastHLine(0, 10, 83, BLACK);
      display.setCursor(0, 25);
      if (timerMode == 0)
      {
        display.print("COMSTOCK");
      }
      else
      {
        display.print("TIMER");
      }
    }
    else if (menuPos == 1) // delay
    {
      display.print("DELAY");
      display.drawFastHLine(0, 10, 83, BLACK);
      display.setCursor(10, 25);
      display.setTextSize(2);
      if (timerDelay == 0)
      {
        display.print("1-4s");
      }
      else if (timerDelay == 1)
      {
        display.print("4-10s");
      }
      else
      {
        display.print(String(timerDelay)+" s");
      }
    }
    else if (menuPos == 3) // stoper
    {
      display.print("Stoper");
      display.drawFastHLine(0, 10, 83, BLACK);
      display.setCursor(0, 25);
      display.setTextSize(2);
      display.print(maxStopwatch);
    }
    else if (menuPos == 2) // contrast
    {
      display.print("Kontrast");
      display.drawFastHLine(0, 10, 83, BLACK);
      display.setCursor(0, 25);
      display.setTextSize(2);
      display.print(contrast);
    }
    
  }

  display.display();
}

void timerStarted()
{
  currentMillis = millis();
  elapsedMillis = (currentMillis - startMillis);
  unsigned long durMS = (elapsedMillis % 1000);     //Milliseconds
  unsigned long durSS = (elapsedMillis / 1000);  //Seconds
  durMilliSec = timeMillis(durSS, durMS);
  showDisplay();
  listenToShots();
  if (timerMode == 1) // stopwatch
  {
    if (durSS == maxStopwatch)
    {
      startState = !startState;
      tone(A0, 2000, 500);
    }
  }
}

void listenToShots()
{
  shotDebouncer3.update();
  if (shotDebouncer3.rose())
  {
    shotsStrings[shots] = durMilliSec;
    shotTimes[shots] = elapsedMillis;
    shots++;
    shotIndex = shots;
  }
}

void showDisplay()
{
  display.setTextSize(1);
  display.clearDisplay();
  display.setTextColor(BLACK, WHITE);
  display.setCursor(15, 0);
  display.print(durMilliSec);
  drawLastShots();
  display.display();
}

void drawLastShots()
{
  if (shots >= 2)
  {
    display.setCursor(0, 40);
    display.print("" + String(shots) + ": " + shotsStrings[shots - 1]);
    display.setCursor(0, 30);
    display.print("" + String(shots - 1) + ": " + shotsStrings[shots - 2]);
  }
  else if (shots > 0)
  {
    display.setCursor(0, 40);
    display.print("" + String(shots) + ": " + shotsStrings[shots - 1]);
  }

  if (shots > 0)
  {
    display.setTextSize(2);
    display.setCursor(0, 12);
    display.print(shotsStrings[shots - 1]);
  }
}

String timeMillis(unsigned long Sectime, unsigned long MStime)
{
  String dataTemp = "";

  if (Sectime < 10)
  {
    dataTemp = dataTemp + "00" + String(Sectime) + ":";
  }
  else if (Sectime < 100)
  {
    dataTemp = dataTemp + "0" + String(Sectime) + ":";
  }
  else
  {
    dataTemp = dataTemp + String(Sectime) + ":";
  }

  dataTemp = dataTemp + String(MStime);

  return dataTemp;
}

void setContrast()
{
  display.setContrast(contrast);
  display.display();
}

void turnBacklight()
{
  if (backlight)
    digitalWrite(12, LOW);
  else
    digitalWrite(12, HIGH);
}

