/**
 * Simple preamplifier built on top of TDA7313
 * With radio built on top of TEA5767, AUX for TV and 3rd channel for BLE Audio
 * 
 * @author Jacek Partyka, WebLogic.pl
 * @copyright 2016 WebLogic.pl
 */

#include <TEA5767Radio.h>
#include <IRLib.h>
#include <LiquidCrystal.h>
#include <Wire.h>


#define MENU_POSITIONS 8

int volume = 35;
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
int lastDecodedValue = 0;
IRrecv My_Receiver(7);
IRdecode My_Decoder;
bool isOn = true;
int frequency = 1049;
int tmpFreq = 1049;
TEA5767Radio radio = TEA5767Radio();

int balance = 0;
int fader = 0;
int gain = 0;
int bass = 0;
int treb = 0;
int loud = 0;

int lf = 31;
int rf = 31;
int lr = 31;
int rr = 31;

byte activeChannel = 2;

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

void setup() { 
  byte full[8] = {0b11111,
 0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111};
byte empt[8] = {0b11111,
 0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b11111};
byte on5[8] = {0b11111,
 0b10000,
  0b10000,
  0b10000,
  0b10000,
  0b10000,
  0b10000,
  0b11111};
byte two5[8] = {0b11111,
  0b11000,
  0b11000,
  0b11000,
  0b11000,
  0b11000,
  0b11000,
  0b11111};
byte three5[8] = {0b11111,
  0b11100,
  0b11100,
  0b11100,
  0b11100,
  0b11100,
  0b11100,
  0b11111};
byte four5[8] = {0b11111,
 0b11110,
  0b11110,
  0b11110,
  0b11110,
  0b11110,
  0b11110,
  0b11111};
byte arr[8] = {0b10000,
 0b11000,
  0b11100,
  0b11110,
  0b11110,
  0b11100,
  0b11000,
  0b10000};
  lcd.createChar(0, full);
  lcd.createChar(1, empt);
  lcd.createChar(2, on5);
  lcd.createChar(3, two5);
  lcd.createChar(4, three5);
  lcd.createChar(5, four5);
  lcd.createChar(6, arr);
  
  pinMode(pin_A, INPUT);
  pinMode(pin_B, INPUT);
  pinMode(confirmButton, INPUT);
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  pinMode(6, OUTPUT);
  lcd.begin(16, 2);
  lcd.print("WITA WZMACNIACZ!");
  delay(1000);
  onOff();
  currentTime = millis();
  loopTime = currentTime; 
  My_Receiver.enableIRIn(); // Start the receiver
  Wire.begin();
  setInitials();
  setRadioFrequency();
}

void drawLinearBar(int value, int maxi)
{
  lcd.setCursor(0, 1);
  lcd.write(byte(6)); // arrow
  // mamy 15 znaków na progress więc liczymy
  byte chars = (int)((float)value/maxi*15);
  byte rest = (int)(((float)value/maxi*15-chars)*5);
  for (byte i=0; i<chars; i++)
  {
    lcd.write(byte(0));
  }
  if (rest == 0)
    lcd.write(" ");
  else if (rest == 1)
    lcd.write(byte(2));
  else if (rest == 2)
    lcd.write(byte(3));
  else if (rest == 3)
    lcd.write(byte(4));
  else if (rest == 4)
    lcd.write(byte(5));
  for (byte i = chars; i < 14; i++)
  {
    lcd.write(" ");
  }
}

void drawBalanceBar(int value)
{
  lcd.setCursor(0, 1);
  for (int i=-7; i<(value < 0 ? value : 0); i++)
  {
    lcd.write(" ");
  }

  for (int i=value; i<0; i++)
  {
    lcd.write(byte(0));
  }
  
  lcd.write(byte(0)); // zero

  for (int i=0; i<value; i++)
  {
    lcd.write(byte(0));
  }

  for (int i=(value < 0 ? 0 : value); i<7; i++)
  {
    lcd.write(" ");
  }
  
}

void setRadioFrequency()
{
  radio.setFrequency((float)frequency/10.0);
}

void setChannel()
{
  Wire.beginTransmission(0x44);
  Wire.write(63+((3-gain)*8)+((1-loud)*4)+activeChannel);
  Wire.endTransmission();
}

void setVolume()
{
  Wire.beginTransmission(0x44);
  Wire.write(63-volume);
  Wire.endTransmission();
}

void setInitials()
{
  Wire.beginTransmission(0x44);
  Wire.write(0x6F); // bass flat
  Wire.write(0x7F); // treb flat
  Wire.write(128); // LF
  Wire.write(160); // RF
  Wire.write(192); // LR
  Wire.write(224); // RR
  Wire.write(63-volume);
  Wire.write(91+activeChannel);
  Wire.endTransmission();
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
        if (value+increment <= maxValue)
          value += increment;               
      }   
      else {
        // B is low so counter-clockwise      
        if (value - increment >= minValue)
          value -= increment;              
      }   

    }   
    encoder_A_prev = encoder_A;     // Store value of A for next time    
    
    loopTime = currentTime;  // Updates loopTime
  }
}

void onOff()
{
  isOn = !isOn;
  if (!isOn)
  {
    digitalWrite(6, LOW);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("   WYLACZONY!   ");
  }
  else
  {
    digitalWrite(6, HIGH);
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

void setTrebble()
{
  Wire.beginTransmission(0x44);
  if (treb < 0)
     Wire.write(112+7+treb);
  else
     Wire.write(112+15-treb);
  Wire.endTransmission();
}

void setBass()
{
  Wire.beginTransmission(0x44);
  if (bass < 0)
     Wire.write(96+7+bass);
  else
     Wire.write(96+15-bass);
  Wire.endTransmission();
}

void setBalance(int incr)
{
  if (balance < 0 || (balance == 0 && incr > 0))
  {
     lf += incr;
     lr += incr;
  }
  else if (balance > 0 || (balance == 0 && incr < 0))
  {
     rf -= incr;
     rr -= incr;
  }
  setAttenuators();
}

void setAttenuators()
{
  Wire.beginTransmission(0x44);
  int tmp_lf = (lf < 0 ? 0 : lf);
  int tmp_rf = (rf < 0 ? 0 : rf);
  int tmp_lr = (lr < 0 ? 0 : lr);
  int tmp_rr = (rr < 0 ? 0 : rr);  
  Wire.write(128+31-tmp_lf);
  Wire.write(160+31-tmp_rf);
  Wire.write(192+31-tmp_lr);
  Wire.write(224+31-tmp_rr);
  Wire.endTransmission();
}

void setFader(int incr)
{
  if (fader < 0 || (fader == 0 && incr > 0))
  {
    lf += incr;
    rf += incr;
  }
  else if (fader > 0 || (fader == 0 && incr < 0))
  {
     lr -= incr;
     rr -= incr;
  }
  setAttenuators();
}


void loop() {

  if (isOn)
  {
    if (menuLevel == 0) // main menu. We can only set volume
    {
      lcd.setCursor(0, 0);
      if (activeChannel == 1)
        lcd.print("RADIO           ");
      else if (activeChannel == 2)
        lcd.print("TELEWIZJA       ");
      else if (activeChannel == 3)
        lcd.print("BLUETOOTH       ");

      drawLinearBar(volume, 63);
      
      tmpValue = volume;
      readEncoder(volume, 1, 0, 63);
      if (volume != tmpValue)
      {
        // here we set volume level
        setVolume();
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
      readEncoder(menuLevel, 1, 1, MENU_POSITIONS);
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
      readEncoder(menuLevel, 1, 1, MENU_POSITIONS);
      if (confirmPressed())
      {
        menuLevel = 21; // balance
      }
    }
    if (menuLevel == 3) // we are in the menu
    {
      lcd.setCursor(0, 0);
      lcd.print("FADER            ");
      lcd.setCursor(0, 1);
      lcd.print("                ");
      readEncoder(menuLevel, 1, 1, MENU_POSITIONS);
      if (confirmPressed())
      {
        menuLevel = 31; // fader
      }
    }
    if (menuLevel == 4) // we are in the menu
    {
      lcd.setCursor(0, 0);
      lcd.print("LOUDNESS            ");
      lcd.setCursor(0, 1);
      lcd.print("                ");
      readEncoder(menuLevel, 1, 1, MENU_POSITIONS);
      if (confirmPressed())
      {
        menuLevel = 41; // exit
      }
    }
    if (menuLevel == 5) // we are in the menu
    {
      lcd.setCursor(0, 0);
      lcd.print("GAIN            ");
      lcd.setCursor(0, 1);
      lcd.print("                ");
      readEncoder(menuLevel, 1, 1, MENU_POSITIONS);
      if (confirmPressed())
      {
        menuLevel = 51; // exit
      }
    }
    if (menuLevel == 6) // we are in the menu
    {
      lcd.setCursor(0, 0);
      lcd.print("BASS            ");
      lcd.setCursor(0, 1);
      lcd.print("                ");
      readEncoder(menuLevel, 1, 1, MENU_POSITIONS);
      if (confirmPressed())
      {
        menuLevel = 61; // exit
      }
    }        
    if (menuLevel == 7) // we are in the menu
    {
      lcd.setCursor(0, 0);
      lcd.print("TREBBLE            ");
      lcd.setCursor(0, 1);
      lcd.print("                ");
      readEncoder(menuLevel, 1, 1, MENU_POSITIONS);
      if (confirmPressed())
      {
        menuLevel = 71; // exit
      }
    }    
    if (menuLevel == 8) // we are in the menu
    {
      lcd.setCursor(0, 0);
      lcd.print("EXIT            ");
      lcd.setCursor(0, 1);
      lcd.print("                ");
      readEncoder(menuLevel, 1, 1, MENU_POSITIONS);
      if (confirmPressed())
      {
        menuLevel = 0; // exit
      }
    }
    
    if (menuLevel == 11) // set the frequency
    {
      lcd.setCursor(0, 0);
      lcd.print("RADIO FREQUENCY ");
      lcd.setCursor(0, 1);
      lcd.print((float)frequency/10.0);
      lcd.print(" MHz           ");
      tmpFreq = frequency;
      readEncoder(frequency, 1, 88, 110);
      if (tmpFreq != frequency)
      {
        setRadioFrequency();
      }
      if (confirmPressed())
      {
        menuLevel = 1; // frequency
      }
    }   
    
    if (menuLevel == 21) // set balance
    {
      lcd.setCursor(0, 0);
      lcd.print("BALANCE: ");
      lcd.print(balance);
      lcd.print("     ");
      /*lcd.setCursor(0, 1);
      lcd.print(balance);
      lcd.print("           ");*/
      drawBalanceBar((int)balance/4);
      tmpValue = balance;
      readEncoder(balance, 1, -31, 31);
      if (tmpValue != balance)
      {
         setBalance(balance-tmpValue);
      }
      if (confirmPressed())
      {
        menuLevel = 2; // balance
      }
    }    
    if (menuLevel == 31) // set fader
    {
      lcd.setCursor(0, 0);
      lcd.print("FADER: ");
      lcd.print(fader);
      lcd.print("       ");
      drawBalanceBar((int)fader/4);
      tmpValue = fader;
      readEncoder(fader, 1, -31, 31);
      if (tmpValue != fader)
      {
         setFader(fader-tmpValue);
      }
      if (confirmPressed())
      {
        menuLevel = 3; // fader
      }
    }       
    if (menuLevel == 41) // set loudness
    {
      lcd.setCursor(0, 0);
      lcd.print("LOUDNESS      ");
      lcd.setCursor(0, 1);
      lcd.print(loud == 0 ? "OFF" : "ON");
      lcd.print("           ");
      tmpValue = loud;
      readEncoder(loud, 1, 0, 1);
      if (tmpValue != loud)
      {
        setChannel();
      }
      if (confirmPressed())
      {
        menuLevel = 4; // loudness
      }
    }  
    if (menuLevel == 51) // set gain
    {
      lcd.setCursor(0, 0);
      lcd.print("GAIN      ");
      lcd.setCursor(0, 1);
      lcd.print(gain);
      lcd.print("           ");
      tmpValue = gain;
      readEncoder(gain, 1, 0, 3);
      if (tmpValue != gain)
      {
        setChannel();
      }
      if (confirmPressed())
      {
        menuLevel = 5; // balance
      }
    }    
    if (menuLevel == 61) // set bass
    {
      lcd.setCursor(0, 0);
      lcd.print("BASS      ");
      drawBalanceBar(bass);
      tmpValue = bass;
      readEncoder(bass, 1, -7, 7);
      if (tmpValue != bass)
      {
        setBass();
      }
      if (confirmPressed())
      {
        menuLevel = 6; // balance
      }
    } 
    if (menuLevel == 71) // set trebble
    {
      lcd.setCursor(0, 0);
      lcd.print("TREBBLE      ");
      drawBalanceBar(treb);
      tmpValue = treb;
      readEncoder(treb, 1, -7, 7);
      if (tmpValue != treb)
      { 
        setTrebble();
      }
      if (confirmPressed())
      {
        menuLevel = 7; // trebble
      }
    } 
  }

  if (My_Receiver.GetResults(&My_Decoder)) {
    My_Decoder.decode();    //Decode the data
    //My_Decoder.DumpResults(); //Show the results on serial monitor
   // Serial.print("Rozkaz: ");
   // Serial.println(My_Decoder.value);

    if ((My_Decoder.value == 5452 || My_Decoder.value == 7500 || My_Decoder.value == 7244 || My_Decoder.value == 5196 || My_Decoder.value == 6156 || My_Decoder.value == 4108) && My_Decoder.value != lastDecodedValue) // on/off
    {
      onOff();
    }

    if (isOn)
    {
      if (My_Decoder.value == 5136 || My_Decoder.value == 7184) // vol plus
      {
        if (volume < 62)
        {
          volume +=1;
          setVolume();
        }
      }

      if (My_Decoder.value == 5137 || My_Decoder.value == 7185) // vol down
      {
        if (volume > 0)
        {
          volume -= 1;
          setVolume();
        }
      }

      if ((My_Decoder.value == 7295 || My_Decoder.value == 5247) && My_Decoder.value != lastDecodedValue) // radio on
      {
        activeChannel = 1;
        setChannel();
      }
      
      if ((My_Decoder.value == 6207 || My_Decoder.value == 4159) && My_Decoder.value != lastDecodedValue) // tv
      {
        activeChannel = 2;
        setChannel();
      }

      if ((My_Decoder.value == 7551 || My_Decoder.value == 5503) && My_Decoder.value != lastDecodedValue) // bt
      {
        activeChannel = 3;
        setChannel();
      }

    }

    lastDecodedValue = My_Decoder.value;

    delay(150);
    
    My_Receiver.resume();     //Restart the receiver
  }

  buttonState = digitalRead(A0); // on/off
  if (buttonState == HIGH)
  {
    onOff();
    do
    {    
      delay(50);
      buttonState = digitalRead(A0);
    }
    while(buttonState == HIGH);
  }

  buttonState = digitalRead(A1); // radio
  if (buttonState == HIGH)
  {
    activeChannel = 1;
    setChannel();
    do
    {    
      delay(50);
      buttonState = digitalRead(A1);
    }
    while(buttonState == HIGH);
  }  
  buttonState = digitalRead(A2); // tv
  if (buttonState == HIGH)
  {
    activeChannel = 2;
    setChannel();
    do
    {    
      delay(50);
      buttonState = digitalRead(A2);
    }
    while(buttonState == HIGH);
  }  
  buttonState = digitalRead(A3); // BT
  if (buttonState == HIGH)
  {
    activeChannel = 3;
    setChannel();
    do
    {    
      delay(50);
      buttonState = digitalRead(A3);
    }
    while(buttonState == HIGH);
  }  

 
}
