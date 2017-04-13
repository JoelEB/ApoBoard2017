/*
 * IRremote: IRsendDemo - demonstrates sending IR codes with IRsend
 * An IR LED must be connected to Arduino PWM pin 3.
 * Version 0.1 July, 2009
 * Copyright 2009 Ken Shirriff
 * http://arcfn.com
 */


#include <IRremote.h>

int RECV_PIN = 11;

IRrecv irrecv(RECV_PIN);
IRsend irsend;

decode_results results;

int count = 0;
////////////////////////////////////////////////////////
void setup()
{
  Serial.begin(9600);
  irrecv.enableIRIn(); // Start the receiver
}
////////////////////////////////////////////////////////
void loop() 
{
  checkIR();
  
  if(count == 100)
  {
    sendIR();
    count = 0;//reset count
  }
  else
  {
    count++;
  }
  
}
////////////////////////////////////////////////////////
void checkIR()
{
  
  if (irrecv.decode(&results)) 
  {
    Serial.println(results.value, HEX);
    irrecv.resume(); // Receive the next value
  }
  delay(100);
}
////////////////////////////////////////////////////////
void sendIR()
{
  for (int i = 0; i < 3; i++) 
  {
    irsend.sendSony(0x069, 12);
    delay(40);
  }
}
