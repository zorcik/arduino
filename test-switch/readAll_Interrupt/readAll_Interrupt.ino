#include "Arduino.h"
#include "PCF8575.h"

#define INTERRUPT_PIN 3

// Function interrupt
void keyChangedOnPCF8575();

// Set i2c address
PCF8575 input1(0x27, INTERRUPT_PIN, keyChangedOnPCF8575);
PCF8575 input2(0x26);

PCF8575 output1(0x20);

PCF8575::DigitalInput input1states;
PCF8575::DigitalInput input2states;

boolean outputs1[] = {false, false, false, false, false, false, false, false,
                      false, false, false, false, false, false, false, false};
boolean outputs2[] = {false, false, false, false, false, false, false, false,
                      false, false, false, false, false, false, false, false};

void setup()
{
	Serial.begin(9600);
	input1.pinMode(P0, INPUT);
  input1.pinMode(P1, INPUT);
	input1.pinMode(P2, INPUT);
	input1.pinMode(P3, INPUT);
  input1.pinMode(P4, INPUT);
  input1.pinMode(P5, INPUT);
  input1.pinMode(P6, INPUT);
  input1.pinMode(P7, INPUT);
  input1.pinMode(P8, INPUT);
  input1.pinMode(P9, INPUT);
  input1.pinMode(P10, INPUT);
  input1.pinMode(P11, INPUT);
  input1.pinMode(P12, INPUT);
  input1.pinMode(P13, INPUT);
  input1.pinMode(P14, INPUT);
  input1.pinMode(P15, INPUT);
	input1.begin();
  input2.pinMode(P0, INPUT);
  input2.pinMode(P1, INPUT);
  input2.pinMode(P2, INPUT);
  input2.pinMode(P3, INPUT);
  input2.pinMode(P4, INPUT);
  input2.pinMode(P5, INPUT);
  input2.pinMode(P6, INPUT);
  input2.pinMode(P7, INPUT);
  input2.pinMode(P8, INPUT);
  input2.pinMode(P9, INPUT);
  input2.pinMode(P10, INPUT);
  input2.pinMode(P11, INPUT);
  input2.pinMode(P12, INPUT);
  input2.pinMode(P13, INPUT);
  input2.pinMode(P14, INPUT);
  input2.pinMode(P15, INPUT);
  input2.begin();
  
  output1.pinMode(P0, OUTPUT);
  output1.pinMode(P1, OUTPUT);
  output1.pinMode(P2, OUTPUT);
  output1.pinMode(P3, OUTPUT);
  output1.pinMode(P4, OUTPUT);
  output1.pinMode(P5, OUTPUT);
  output1.pinMode(P6, OUTPUT);
  output1.pinMode(P7, OUTPUT);
  output1.pinMode(P8, OUTPUT);
  output1.pinMode(P9, OUTPUT);
  output1.pinMode(P10, OUTPUT);
  output1.pinMode(P11, OUTPUT);
  output1.pinMode(P12, OUTPUT);
  output1.pinMode(P13, OUTPUT);
  output1.pinMode(P14, OUTPUT);
  output1.pinMode(P15, OUTPUT);
  output1.begin();  

	Serial.println("START");

}

bool keyChanged = false;
void loop()
{
	if (keyChanged){
    delay(20);
		PCF8575::DigitalInput d1 = input1.digitalReadAll();
    if (d1.p0 != input1states.p0 && d1.p0 == HIGH)
    {
      outputs1[0] = !outputs1[0];
    }
    if (d1.p1 != input1states.p1 && d1.p1 == HIGH)
    {
      outputs1[1] = !outputs1[1];
    }
    if (d1.p2 != input1states.p2 && d1.p2 == HIGH)
    {
      outputs1[2] = !outputs1[2];
    }
    if (d1.p3 != input1states.p3 && d1.p3 == HIGH)
    {
      outputs1[3] = !outputs1[3];
    }
    if (d1.p4 != input1states.p4 && d1.p4 == HIGH)
    {
      outputs1[4] = !outputs1[4];
    }
    if (d1.p5 != input1states.p5 && d1.p5 == HIGH)
    {
      outputs1[5] = !outputs1[5];
    }
    if (d1.p6 != input1states.p6 && d1.p6 == HIGH)
    {
      outputs1[6] = !outputs1[6];
    }
    if (d1.p7 != input1states.p7 && d1.p7 == HIGH)
    {
      outputs1[7] = !outputs1[7];
    }
    if (d1.p8 != input1states.p8 && d1.p8 == HIGH)
    {
      outputs1[8] = !outputs1[8];
    }
    if (d1.p9 != input1states.p9 && d1.p9 == HIGH)
    {
      outputs1[9] = !outputs1[9];
    }
    if (d1.p10 != input1states.p10 && d1.p10 == HIGH)
    {
      outputs1[10] = !outputs1[10];
    }
    if (d1.p11 != input1states.p11 && d1.p11 == HIGH)
    {
      outputs1[11] = !outputs1[11];
    }
    if (d1.p12 != input1states.p12 && d1.p12 == HIGH)
    {
      outputs1[12] = !outputs1[12];
    }
    if (d1.p13 != input1states.p13 && d1.p13 == HIGH)
    {
      outputs1[13] = !outputs1[13];
    }
    if (d1.p14 != input1states.p14 && d1.p14 == HIGH)
    {
      outputs1[14] = !outputs1[14];
    }
    if (d1.p15 != input1states.p15 && d1.p15 == HIGH)
    {
      outputs1[15] = !outputs1[15];
    }
    
		Serial.print("READ VALUE FROM PCF P1: ");
		Serial.print(d1.p0);
		Serial.print(" - ");
		Serial.print(d1.p1);
		Serial.print(" - ");
		Serial.print(d1.p2);
		Serial.print(" - ");
		Serial.print(d1.p3);
    Serial.print(" - ");
    Serial.print(d1.p4);
    Serial.print(" - ");
    Serial.print(d1.p5);
    Serial.print(" - ");
    Serial.print(d1.p6);
    Serial.print(" - ");
    Serial.print(d1.p7);
    Serial.print(" - ");
    Serial.print(d1.p8);
    Serial.print(" - ");
    Serial.print(d1.p9);
    Serial.print(" - ");
    Serial.print(d1.p10);
    Serial.print(" - ");
    Serial.print(d1.p11);
    Serial.print(" - ");
    Serial.print(d1.p12);
    Serial.print(" - ");
    Serial.print(d1.p13);
    Serial.print(" - ");
    Serial.print(d1.p14);
    Serial.print(" - ");
    Serial.println(d1.p15);
		//delay(500);
		keyChanged= false;

    uint16_t out = 0;

    for (uint8_t i = 0; i<16; i++)
    {
      if(outputs1[i])
      {
        out = out | (1 << i);
      }
    }

    Serial.println(out, BIN);

    input1states = d1;

    output1.digitalWriteAll(out);
    
	}
}

void keyChangedOnPCF8575(){
	// Interrupt called (No Serial no read no wire in this function, and DEBUG disabled on PCF library)
	 keyChanged = true;
}
