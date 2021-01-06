#include <Wire.h>

#define INTERRUPT_PIN 22

int address = 0x27;

uint16_t dataReceive; //16bit data received
uint16_t dataSend;  //16bit data sent


void setup() {

  Wire.begin(); 
  Serial.begin(9600);
  //turn pins to input in setup
  //if bit = 0, pin functions as input or an output that is off
  //if bit = 1, pin functions as an output that is on
  dataSend = word(B11111111,B11111111); //turn every pin to input
  pcf8575_write(dataSend); //turn the pcf8575 pins to input

}

void loop() {
  
  dataReceive = pcf8575_read(); //read the pcf8575 pins
  Serial.print("Read, ");
  Serial.println(dataReceive,BIN); //~ inverses the results, this removes the ambiguity of leading zero 
  delay(500);
  /*
  Read, 1111111111111101 - p1 pressed
  Read, 1111111111111011 - p2 pressed
  Read, 1111111111110111 - p3 pressed
  *
  Serial.println("----------------------");
  delay(300);
  */
  /*
  dataSend = word(B11111111,B11111111); //turn every pin to input
  pcf8575_write(dataSend);
  delay(500);
  dataSend = word(B00000000,B00000000); //turn every pin to input
  pcf8575_write(dataSend);
  delay(500);
  */
}

//custom functions -----------------------------

//I2C/TWI success (transaction was successful).
static const uint8_t ku8TWISuccess                   = 0;
//I2C/TWI device not present (address sent, NACK received).
static const uint8_t ku8TWIDeviceNACK                = 2;
//I2C/TWI data not received (data sent, NACK received).
static const uint8_t ku8TWIDataNACK                  = 3;
//I2C/TWI other error.
static const uint8_t ku8TWIError                     = 4;

uint8_t error;
void pcf8575_write(uint16_t dt){
  Wire.beginTransmission(address);
  Wire.write(lowByte(dt));
  Wire.write(highByte(dt));
  error = Wire.endTransmission();
  if(error == ku8TWISuccess){ //okay!
    Serial.println("OK, zapisałem");
  }
  else{ //we have an error
    //do something here if you like
  }
}
uint8_t hi,lo;
uint16_t pcf8575_read(){
  Wire.beginTransmission(address);
  error = Wire.endTransmission();
  if(error == ku8TWISuccess){ //okay!
    Wire.requestFrom(address,2); 
    if(Wire.available()){
      lo = Wire.read();
      hi = Wire.read();
      //char c = Wire.read(); // receive a byte as character
      //Serial.print(c);
      return(word(hi,lo)); //joing bytes 
    }
    else{//error
      //do something here if you like  
      Serial.println("ERROR");
    }
  }
  else{ //error
    //do something here if you like  
    Serial.println("ERROR2");
  }
}
