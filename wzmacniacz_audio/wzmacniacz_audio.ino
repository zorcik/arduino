#include <IRLib.h>
#include <LiquidCrystal.h>
#include <Wire.h>


int volume = 10;
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
//TEA5767Radio radio = TEA5767Radio();

int activeChannel = 2;

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

void setup() {
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
  //Serial.begin(9600);
  //Serial.println("ir");
  My_Receiver.enableIRIn(); // Start the receiver
  //Serial.println("wire");
  Wire.begin();
  //Serial.println("initials");
  setInitials();
//  setRadioFrequency();
 //Serial.println("Conf ended");
}

void setRadioFrequency()
{
//  radio.setFrequency((float)frequency/10.0);
//  Serial.print("Set frequency to: ");
//  Serial.println((float)frequency/10.0);
}

void setChannel()
{
  Wire.beginTransmission(0x44);
  Wire.write(91+activeChannel);
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
  //Wire.write(0x45); // input 2, 11.25db gain, loud mode off
  Wire.write(0x6F); // bass flat
  Wire.write(0x7F); // treb flat
  //Wire.write(0xC0); // 0db attn RL
  //Wire.write(0xE0); // 0db attn RR
  //Wire.write(0x16); // vol atten to -40db
  Wire.write(128); // LF
  Wire.write(160); // RF
  Wire.write(192); // LR
  Wire.write(224); // RR
  //Wire.write(103); // Bass
  //Wire.write(119); // treble
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
        
      lcd.setCursor(0, 1);
      lcd.print("VOLUME: ");
      lcd.print(volume);
      lcd.print("     ");
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
