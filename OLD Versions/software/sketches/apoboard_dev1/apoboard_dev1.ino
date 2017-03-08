/*
 Fading
 
 This example shows how to fade an LED using the analogWrite() function.
 
 The circuit:
 * LED attached from digital pin 9 to ground.
 
 Created 1 Nov 2008
 By David A. Mellis
 modified 30 Aug 2011
 By Tom Igoe
 
 http://arduino.cc/en/Tutorial/Fading
 
 This example code is in the public domain.
 
 */


int redpin = 3;
int greenpin = 9;
int bluepin = 10;

void setup() {
  Serial.begin(9600);
  pinMode(redpin, OUTPUT);
  pinMode(greenpin, OUTPUT);
  pinMode(bluepin, OUTPUT);  
}


void loop()  { 
  // fade in from min to max in increments of 5 points:
  for(int fadeValue = 0 ; fadeValue <= 255; fadeValue +=5) { 
    // sets the value (range from 0 to 255):
    analogWrite(redpin, fadeValue);         
    analogWrite(greenpin, fadeValue);         
    analogWrite(bluepin, fadeValue);         
    // wait for 30 milliseconds to see the dimming effect    
    delay(30);                            
  } 

  // fade out from max to min in increments of 5 points:
  for(int fadeValue = 255 ; fadeValue >= 0; fadeValue -=5) { 
    // sets the value (range from 0 to 255):
    analogWrite(redpin, fadeValue);         
    analogWrite(greenpin, fadeValue + 50);         
    analogWrite(bluepin, fadeValue + 100);        
    // wait for 30 milliseconds to see the dimming effect    
    delay(30);                            
  } 
  
  int sensorValue = analogRead(A0);
  Serial.println(sensorValue);
  
}


