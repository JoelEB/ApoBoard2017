/*
 * IRremote: IRrecvDemo - demonstrates receiving IR codes with IRrecv
 * An IR detector/demodulator must be connected to the input RECV_PIN.
 * Version 0.1 July, 2009
 * Copyright 2009 Ken Shirriff
 * http://arcfn.com
 */
#include <IRremote.h>
#include <Adafruit_NeoPixel.h>

#define PIN 7
#define NumberLED 10

int RECV_PIN = 8;
int buttonVal;

int colorState = 0;

boolean fadeState = false;

int rainbowFlag = 0;

float truRed, truBlue, truGreen = 0;
float redVal, greenVal, blueVal = 0; //value between 0-255 for Reg Green Blue values
float lumi = 100;  //value between 1-100 for luminocity 

IRrecv irrecv(RECV_PIN);
decode_results results;

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_RGB     Pixels are wired for RGB bitstream
//   NEO_GRB     Pixels are wired for GRB bitstream
//   NEO_KHZ400  400 KHz bitstream (e.g. FLORA pixels)
//   NEO_KHZ800  800 KHz bitstream (e.g. High Density LED strip)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NumberLED, PIN, NEO_GRB + NEO_KHZ800);

long previousMillis = 0;        // will store last time strip was updated
long interval = 5000;           // interval at which color will stay at that color (milliseconds)
unsigned long currentMillis;
///////////////////////////////////////////////////////////
void setup()
{
  //pinMode(10, OUTPUT);//power and GND for IR Receiver
  //pinMode(9, OUTPUT);
  //digitalWrite(10, LOW);
  //digitalWrite(9, HIGH);
  
  strip.begin();
  strip.show();
  Serial.begin(9600);
  
  irrecv.enableIRIn(); // Start the receiver
  Serial.println("waiting...");
}  
/////////////////////////////////////////////////////////////////
void loop()
{
checkButtonPress();
}
//////////////////////////////////////////////////////////////////
void checkButtonPress()
{
  rainbowFlag = 0;
  if (irrecv.decode(&results)) 
  {
        if(results.value != 4294967295 || 0){
        buttonVal = results.value;
        buttonVal = abs(buttonVal);
  }
  checkButton();    
     
                            
  irrecv.resume(); // Receive the next value
  }
}
//////////////////////////////////////////////////////////////////
void checkButton()
{
  rainbowFlag = 0;
  if(buttonVal == 24481)//Up Arrow
  { 
      lumi+=10;
      if(lumi > 100)
      lumi = 100;
      delay(150);
      //Serial.println(lumi);
      
      redVal = truRed * (lumi/100); //calculate luminocity values
      greenVal = truGreen * (lumi/100);
      blueVal = truBlue * (lumi/100);
    
      fill(strip.Color((int)redVal, (int)greenVal, (int)blueVal), 50);
  }   
  if(buttonVal == 255)//Down Arrow
  {
      lumi-=10;
      if(lumi < 0)
      lumi = 0;
      delay(150);
      //Serial.println(lumi);
      
      redVal = truRed * (lumi/100); //calculate luminocity values
      greenVal = truGreen * (lumi/100);
      blueVal = truBlue * (lumi/100);
      
      fill(strip.Color((int)redVal, (int)greenVal, (int)blueVal), 50);
  }
  if(buttonVal == 4335)//Left Arrow
  {
      --colorState;
      if(colorState < 0)
      colorState = 7;
      delay(150);
      checkColor();
  }       
  if(buttonVal == 32641)// Right Arrow
  {
      ++colorState;
      if(colorState > 7)
      colorState = 0;
      delay(150);
      checkColor();
  }   
  if(buttonVal == 0xFF906F)//Red Button
  {
    rainbowFlag = 1;
      rainbow(100);
  } 
  if(buttonVal == 30855)//B Button
  { 
    fadeState = !fadeState;
    delay(150);
    Serial.println(fadeState);
  }
  if(buttonVal == 22695)//C Button
  {
    rainbowFlag = 1;
      rainbowCycle(50);
  }  
  if(buttonVal == 10201)//Power Button
  {
      fill(strip.Color(0, 0, 0), 0);//Off
  }    
  if(buttonVal == 8415)//Center Button
  {
     truRed = 255;
     truGreen = 255;
     truBlue = 255;
      redVal = truRed * (lumi/100); //calculate luminocity values
      greenVal = truGreen * (lumi/100);
      blueVal = truBlue * (lumi/100);
      fill(strip.Color((int)redVal, (int)greenVal, (int)blueVal), 0); //WHITE
  }

}
//////////////////////////////////////////////////////////////
void checkColor()
{
if(colorState == 0)//Pink
{
     truRed = 255;
     truGreen = 0;
     truBlue = 127;
      redVal = truRed * (lumi/100); //calculate luminocity values
      greenVal = truGreen * (lumi/100);
      blueVal = truBlue * (lumi/100);
      if(fadeState == false)
        fill(strip.Color((int)redVal, (int)greenVal, (int)blueVal), 0);
      if(fadeState == true)
        colorWipe(strip.Color((int)redVal, (int)greenVal, (int)blueVal), 100); 
}
if(colorState == 1)// Red
{
     truRed = 255;
     truGreen = 0;
     truBlue = 0;
      redVal = truRed * (lumi/100); //calculate luminocity values
      greenVal = truGreen * (lumi/100);
      blueVal = truBlue * (lumi/100);
      if(fadeState == false)
        fill(strip.Color((int)redVal, (int)greenVal, (int)blueVal), 0);
      if(fadeState == true)
        colorWipe(strip.Color((int)redVal, (int)greenVal, (int)blueVal), 100); 
}
if(colorState == 2)//Orange
{
     truRed = 255;
     truGreen = 127;
     truBlue = 0;
      redVal = truRed * (lumi/100); //calculate luminocity values
      greenVal = truGreen * (lumi/100);
      blueVal = truBlue * (lumi/100);
      if(fadeState == false)
        fill(strip.Color((int)redVal, (int)greenVal, (int)blueVal), 0);
      if(fadeState == true)
        colorWipe(strip.Color((int)redVal, (int)greenVal, (int)blueVal), 100); 
}
if(colorState == 3)//Yellow
{
     truRed = 127;
     truGreen = 255;
     truBlue = 0;
      redVal = truRed * (lumi/100); //calculate luminocity values
      greenVal = truGreen * (lumi/100);
      blueVal = truBlue * (lumi/100);
      if(fadeState == false)
        fill(strip.Color((int)redVal, (int)greenVal, (int)blueVal), 0);
      if(fadeState == true)
        colorWipe(strip.Color((int)redVal, (int)greenVal, (int)blueVal), 100); 
}
if(colorState == 4)//Green
{
     truRed = 0;
     truGreen = 255;
     truBlue = 0;
      redVal = truRed * (lumi/100); //calculate luminocity values
      greenVal = truGreen * (lumi/100);
      blueVal = truBlue * (lumi/100);
      if(fadeState == false)
        fill(strip.Color((int)redVal, (int)greenVal, (int)blueVal), 0);
      if(fadeState == true)
        colorWipe(strip.Color((int)redVal, (int)greenVal, (int)blueVal), 100); 
}
if(colorState == 5)//Teal
{
     truRed = 0;
     truGreen = 255;
     truBlue = 127;
      redVal = truRed * (lumi/100); //calculate luminocity values
      greenVal = truGreen * (lumi/100);
      blueVal = truBlue * (lumi/100);
      if(fadeState == false)
        fill(strip.Color((int)redVal, (int)greenVal, (int)blueVal), 0);
      if(fadeState == true)
        colorWipe(strip.Color((int)redVal, (int)greenVal, (int)blueVal), 100); 
}
if(colorState == 6)// Blue  
{
     truRed = 0;
     truGreen = 0;
     truBlue = 255;
      redVal = truRed * (lumi/100); //calculate luminocity values
      greenVal = truGreen * (lumi/100);
      blueVal = truBlue * (lumi/100);
      if(fadeState == false)
        fill(strip.Color((int)redVal, (int)greenVal, (int)blueVal), 0);
      if(fadeState == true)
        colorWipe(strip.Color((int)redVal, (int)greenVal, (int)blueVal), 100); 
}
if(colorState == 7)//Purple
{
     truRed = 127;
     truGreen = 0;
     truBlue = 255;
      redVal = truRed * (lumi/100); //calculate luminocity values
      greenVal = truGreen * (lumi/100);
      blueVal = truBlue * (lumi/100);
      if(fadeState == false)
        fill(strip.Color((int)redVal, (int)greenVal, (int)blueVal), 0);
      if(fadeState == true)
        colorWipe(strip.Color((int)redVal, (int)greenVal, (int)blueVal), 100); 
}



}

//////////////////////////////////////////////////////////////

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
}
//////////////////////////////////////////////////////////////
void fill(uint32_t c, uint8_t wait)
{
  for(uint16_t i=0; i<strip.numPixels(); i++) 
  {
      strip.setPixelColor(i, c);
      //delay(wait);
  }
  strip.show();
}
//////////////////////////////////////////////////////////////
void rainbow(uint8_t wait) 
{
  uint16_t i, j;
  
    do{
      for(j=0; j<256; j++) 
      {
        for(i=0; i<strip.numPixels(); i++) 
        {
          strip.setPixelColor(i, Wheel((i+j) & 255));
          checkRainbow();
          if(rainbowFlag == 0)
          break;
        }
        checkRainbow();
        if(rainbowFlag == 0)
          break;
        
        strip.show();
        delay(wait);
 
      }
      checkRainbow();
      if(rainbowFlag == 0)
       break;
    }while(rainbowFlag == 1);
}
//////////////////////////////////////////////////////////////
void checkRainbow()
{
  if (irrecv.decode(&results)) 
  {
        if(results.value != 4294967295 || 0)
        {
          buttonVal = results.value;
          buttonVal = abs(buttonVal);
          if(buttonVal != 0)
            rainbowFlag = 0;
        }                   
  irrecv.resume(); // Receive the next value
  }
}
//////////////////////////////////////////////////////////////
// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

    do{
      for(j=0; j<256*5; j++) 
      {
        for(i=0; i<strip.numPixels(); i++) 
        {
          strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
          checkRainbow();
          if(rainbowFlag == 0)
          break;
        }
        checkRainbow();
        if(rainbowFlag == 0)
          break;
        
        strip.show();
        delay(wait);
 
      }
      checkRainbow();
      if(rainbowFlag == 0)
       break;
    }while(rainbowFlag == 1);

}
//////////////////////////////////////////////////////////////
// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}







