#include <SoftwareSerial.h>
#include <Preferences.h>

#define MYPORT_TX 12
#define MYPORT_RX 13

#define SerialMon Serial
#define qr_code_length 30
#define max_lockers 24

SoftwareSerial qrPort;
Preferences preferences;

String getQRCodeForLockerString(byte locker)
{
  char name[5];
  sprintf(name, "l%d", locker);
  return preferences.getString(name);
}

void checkQRCode(byte code[qr_code_length])
{
    byte lockerCode[qr_code_length];
    for (int i=0; i<max_lockers; i++)
    {
        String t = getQRCodeForLockerString(i);
        t.getBytes(lockerCode, qr_code_length);
        if (memcmp(code, lockerCode, qr_code_length))
        {
            SerialMon.println("Code OK");
            return;
        }
    }
}

void setup() {

  SerialMon.begin(115200);
  delay(100);

  //check if EEPROM works
  preferences.begin("lockers", false); 
  
  byte test[30];
  String testS2 = getQRCodeForLockerString(0);
  SerialMon.println("EEPROM locker 0 test2: "+testS2);
  testS2.getBytes(test, 30);
  for (int i=0; i<30; i++)
  {
    SerialMon.print((char)test[i]);     
  }
  SerialMon.println("END");     
  
 

  qrPort.begin(9600, SWSERIAL_8N1, MYPORT_RX, MYPORT_TX, false);
  if (!qrPort) { // If the object did not initialize, then its configuration is invalid
    SerialMon.println("Invalid SoftwareSerial pin configuration, check config"); 
    while (1) { // Don't continue with invalid configuration
      delay (1000);
    }
  }   
}

byte QR[30];
byte incomingByte = 0;

void loop() {
  // put your main code here, to run repeatedly:
  if (qrPort.available() > 0)
  {
    
    if (qrPort.peek() == 0x44) // dane od sterownika
    {
      
      qrPort.read();
      qrPort.readBytes(QR, 30);

      SerialMon.print("Got QR data: ");
      for (int i=0; i<29; i++)
      {
        SerialMon.print((char)QR[i]);
      }
      SerialMon.println((char)QR[30]);

      checkQRCode(QR);
    }
    else
    {
      qrPort.read();
    }

  }
}
