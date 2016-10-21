// include the library code:
#include <LiquidCrystal.h>

int volume = 100;
unsigned long currentTime;
unsigned long loopTime;
const int pin_A = 8;  // pin 8
const int pin_B = 9;  // pin 9
const int confirmButton = 10; // pin 10
unsigned char encoder_A;
unsigned char encoder_B;
unsigned char encoder_A_prev=0;
int buttonState = 0;
int menuLevel = 0;
int tmpValue = 0;

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

void setup() {
  lcd.begin(16, 2);
  lcd.print("volume");
  pinMode(pin_A, INPUT);
  pinMode(pin_B, INPUT);
  pinMode(confirmButton, INPUT);
  currentTime = millis();
  loopTime = currentTime; 
}

void readEncoder(int &value, int increment, int minValue, int maxValue)
{
  currentTime = millis();
  if(currentTime >= (loopTime + 5)){
    // 5ms since last check of encoder = 200Hz  
    encoder_A = digitalRead(pin_A);    // Read encoder pins
    encoder_B = digitalRead(pin_B);   
    if((!encoder_A) && (encoder_A_prev)){
      // A has gone from high to low 
      if(encoder_B) {
        // B is high so clockwise
        // increase the brightness, dont go over 255
        if (value+increment <= maxValue)
          value += increment;               
      }   
      else {
        // B is low so counter-clockwise      
        // decrease the brightness, dont go below 0
        if (value - increment >= minValue)
          value -= increment;              
      }   

    }   
    encoder_A_prev = encoder_A;     // Store value of A for next time    
    
    loopTime = currentTime;  // Updates loopTime
  }
}

bool confirmPressed()
{
  buttonState = digitalRead(confirmButton);
  if (buttonState == HIGH)
  {
    do
    {    
      delay(50);
      buttonState = digitalRead(confirmButton);
    }
    while(buttonState == HIGH);
    
    return true;
  }
  else
  {
    return false;
  }
}



void loop() {
  
  if (menuLevel == 0) // main menu. We can only set volume
  {
    lcd.setCursor(0, 0);
    lcd.print("RADIO           ");
    lcd.setCursor(0, 1);
    lcd.print("VOLUME: ");
    lcd.print(volume);
    lcd.print("     ");
    tmpValue = volume;
    readEncoder(volume, 1, 0, 63);
    if (volume != tmpValue)
    {
      // here we set volume level
    }

    if (confirmPressed())
    {
      menuLevel = 1;
    }
  }
  if (menuLevel == 1) // we are in the menu (frequency)
  {
    lcd.setCursor(0, 0);
    lcd.print("RADIO FREQUENCY ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
    readEncoder(menuLevel, 1, 1, 4);
    if (confirmPressed())
    {
      menuLevel = 11; // frequency
    }
  }
  if (menuLevel == 2) // we are in the menu
  {
    lcd.setCursor(0, 0);
    lcd.print("BALANCE         ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
    readEncoder(menuLevel, 1, 1, 4);
    if (confirmPressed())
    {
      menuLevel = 21; // balance
    }
  }
  if (menuLevel == 3) // we are in the menu
  {
    lcd.setCursor(0, 0);
    lcd.print("FADER           ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
    readEncoder(menuLevel, 1, 1, 4);
    if (confirmPressed())
    {
      menuLevel = 31; // fader
    }
  }
  if (menuLevel == 4) // we are in the menu
  {
    lcd.setCursor(0, 0);
    lcd.print("EXIT            ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
    readEncoder(menuLevel, 1, 1, 4);
    if (confirmPressed())
    {
      menuLevel = 0; // exit
    }
  }
}
