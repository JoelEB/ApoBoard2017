#include <Adafruit_NeoPixel.h>

#define BUTTON_PIN 2
#define LED_PIN 4
#define NUMofLEDs 5
#define IR_RECEIVE 11
//IR Send is hardcoded in the IRremote library to pin 3

int NumberofColorModes = 5; 
int colorMode = 0;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMofLEDs, LED_PIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

void setup() 
{
  Serial.begin(9600);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  strip.begin();
  strip.setBrightness(40);
  strip.show(); // Initialize all pixels to 'off'
}
////////////////////////////////////////////////////////////////
void loop() 
{  
 if(!digitalRead(BUTTON_PIN))
 {
  colorMode++;
  if(colorMode > NumberofColorModes)
  {
    colorMode = 0;
  }
  delay(150);
 }
 checkColorMode();
}
///////////////////////////////////////
void checkColorMode()
{
  if(colorMode == 0)
  {
    colorWipe(strip.Color(0, 255, 0), 50);//green
  } 
  if(colorMode == 1)
  {
    colorWipe(strip.Color(125, 125, 0), 50);//yellow
  } 
  if(colorMode == 2)
  {
    colorWipe(strip.Color(255, 0, 0), 50);//Red
  } 
  if(colorMode == 3)
  {
    colorWipe(strip.Color(125, 0, 125), 50);//purple
  } 
  if(colorMode == 4)
  {
    colorWipe(strip.Color(0, 0, 255), 50);//blue
  } 
  if(colorMode == 5)
  {
    colorWipe(strip.Color(0, 125, 125), 50);//cyan
  } 
  
}
// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
}


