//Beta code for the DIY interactive electronics Apogaea PCB. Based heavily on the on the Open Music Labs Arduino FHT library (http://wiki.openmusiclabs.com/wiki/ArduinoFHT)
//This code was developed for the Apogaea workshop by the Soundpuddle crew (soundpuddle.org), May 2013

#define LOG_OUT 1 // use the log output function
#define FHT_N 256 // set to 256 point fht


#include <FHT.h> // include the library

#include <avr/interrupt.h>

//Declarations for HSV function
float H,S,L,Rval,Gval,Bval;
void HSL(float H, float S, float L, float& Rval, float& Gval, float& Bval);
float ledhue, ledvalue;
float hoffset = 0.3;

//variables for smoothing function (i.e. low pass filter for RGB color channels)
float rsmoothfactor = 0.5; //this values controls the strength of smoothing. 1=infinite smoothing, 0=no smoothing
float gsmoothfactor = 0.75; //this values controls the strength of smoothing. 1=infinite smoothing, 0=no smoothing
float bsmoothfactor = 0.5; //this values controls the strength of smoothing. 1=infinite smoothing, 0=no smoothing
float valuesmoothfactor = 0.5;
float largestbinsmoothfactor = 0.5;
float largestbinsmooth;
float totalbinpowersmooth;
float Rvalsmooth;
float Gvalsmooth;
float Bvalsmooth;
float ledvaluesmooth;
float gain = 0.6; //this value controls the scaling of the LED brightness (varies from 0-1, smaller = less light)
float rgain = 0.75; //multiplier for red LED channel intensity (typically ranges 0 - 2)
float ggain = 0.2; //multiplier for green LED channel intensity (typically ranges 0 - 2)
float bgain = 1; //multiplier for blue LED channel intensity (typically ranges 0 - 2)

int fftstartingbin = 12;

//variables to store statistics on the FHT output (e.g. loudest frequency BIN, total acoustic power)
float largestbin;
float largestvalue;
float totalbinpower;
float averagebinpower;


//define the physical pins connected to the LEDs and momentary button
int redpin = 3;
int greenpin = 10;
int bluepin = 9;
int buttonpin = 2;

//variable to keep track of what mode we're in (controlled by the momentary button)
int buttonstate = 0;
int mode = 0;
int numberofmodes = 5; //total number of modes (controls the wrap-around of toggling modes)

//variable to keep track of our position in HSV color space for the rainbow cycle (mode 1)
float rainbowhue = 0;

//declarations for button debounce function
long debouncing_time = 250; //Debouncing Time in Milliseconds
volatile unsigned long last_micros;

//void setup is run once at boot time, to configure the microcontroller
void setup() {
  Serial.begin(115200); // use the serial port
  
  //declarations for the FFT functions
//  TIMSK0 = 0; // turn off timer0 for lower jitter
  ADCSRA = 0xe5; // set the adc to free running mode
  ADMUX = 0x40; // use adc0
  DIDR0 = 0x01; // turn off the digital input for adc0

//configure the physical pins of the microcontroller for the LEDs and momentary button
  pinMode(redpin, OUTPUT);
  pinMode(greenpin, OUTPUT);
  pinMode(bluepin, OUTPUT);
  pinMode(buttonpin, INPUT);
  digitalWrite(buttonpin, HIGH);

  // attach our interrupt pin to it's ISR
  attachInterrupt(0, debounceInterrupt, FALLING);

}

//void loop is the main while loop for the program
void loop() {

  /////Mode 2 is the primary sound-reactive code, using the FFT function/////
  if (mode == 2) {

    while(1) { // reduces jitter
      S=0.99;
      if (mode != 2) {break;}
      cli();  // UDRE interrupt slows this way down on arduino1.0
      for (int i = 0 ; i < FHT_N ; i++) { // save 256 samples
        while(!(ADCSRA & 0x10)); // wait for adc to be ready
        ADCSRA = 0xf5; // restart adc
        byte m = ADCL; // fetch adc data
        byte j = ADCH;
        int k = (j << 8) | m; // form into an int
        k -= 0x0200; // form into a signed int
        k <<= 6; // form into a 16b signed int
        fht_input[i] = k; // put real data into bins
      }
      fht_window(); // window the data for better frequency response
      fht_reorder(); // reorder the data before doing the fht
      fht_run(); // process the data in the fht
      fht_mag_log(); // take the output of the fht
      //    fht_mag_lin();
      sei();
      //    Serial.write(255); // send a start byte
      //    Serial.write(fht_log_out, FHT_N/2); // send out the data

      //Determine the bin with the greatest value
      largestbin = 0;
      largestvalue = 0;
      totalbinpower = 0;
      for (int i = fftstartingbin; i < FHT_N/4; i++) {
          totalbinpower = totalbinpower + fht_log_out[i];
        if (fht_log_out[i] > largestvalue) {
          largestbin = i-fftstartingbin; 
          largestvalue = fht_log_out[i];
        }
//            Serial.print(fht_log_out[i]);
//            Serial.print(";");
      }
      averagebinpower = (totalbinpower / ((FHT_N/4) - fftstartingbin));

      //print out the results over the serial port (for debugging purposes)
      Serial.print(largestbin);
      Serial.print("--");
      Serial.print(largestvalue);
      Serial.print("--");
      Serial.print(totalbinpower);
      Serial.print("--");
      Serial.print(averagebinpower);
//      Serial.print("--");
//      Serial.print(mode);
//      Serial.print("--");

      //determine which bin the largest, and what is the value of that bin?
      
      largestbinsmooth = smooth(largestbin, largestbinsmoothfactor, largestbinsmooth);
      totalbinpowersmooth = smooth(totalbinpower, valuesmoothfactor, totalbinpowersmooth);
      
      largestbinsmooth = largestbinsmooth / ((FHT_N/4) - fftstartingbin);
      if (largestbinsmooth > 1) {
        largestbinsmooth = 1;
      }
      largestvalue = largestvalue / 1024;
      if (largestvalue > 1) {
        largestvalue = 1;
      }
      totalbinpower = totalbinpowersmooth / (255*((FHT_N/4) - fftstartingbin));
      if (totalbinpower > 1) {
        totalbinpower = 1;
      }
      averagebinpower = averagebinpower / 255;
      if (averagebinpower > 1) {
        averagebinpower = 1;
      }
      
      
      ledhue = ((-0.5*(-largestbinsmooth))+(-2*totalbinpower))+hoffset;
      ledvalue = 2*totalbinpower;
      
    //convert HSV color space to RGB color space
      HSL(ledhue,S,ledvalue,Rval,Gval,Bval);

      ///Run the RGB color channels through the low pass filter
      Rvalsmooth = smooth(Rval, rsmoothfactor, Rvalsmooth);
      Gvalsmooth = smooth(Gval, gsmoothfactor, Gvalsmooth);
      Bvalsmooth = smooth(Bval, bsmoothfactor, Bvalsmooth);

      //Write the RGB color channel values to the PWM driver for each pin
      analogWrite(redpin, Rvalsmooth);         
      analogWrite(greenpin, Gvalsmooth);         
      analogWrite(bluepin, Bvalsmooth);
      
      Serial.print("||");
      Serial.print(largestbinsmooth);
      Serial.print("--");
      Serial.print(totalbinpower);
      Serial.print("--");
      Serial.print(ledhue);
      Serial.print("--");
      Serial.print(ledvalue);
      Serial.print("||");
      Serial.print(Rval);
      Serial.print(";");
      Serial.print(Gval);
      Serial.print(";");
      Serial.print(Bval);      

      Serial.println();
    }
  }

  ///////////////////////////////////////////////////////////////////////
  /////Mode 2 is a simple cycle through a rainbow in HSV color space/////
  else if (mode == 1) {
    
    //increment through the hues of HSV color space (range from 0 - 1)
    S = 0.99;
    L = 0.5;
    rainbowhue = rainbowhue + 0.001;
    
    //wrap around back to zero when >1
    if (rainbowhue >= 1) {
      rainbowhue = 0;
    }
    
    //convert HSV color space to RGB color space
    HSL(rainbowhue,S,L,Rval,Gval,Bval);

    ///Run the RGB color channels through the low pass filter
//    Rvalsmooth = (Rvalsmooth * (1-rsmoothfactor)) + (rsmoothfactor * Rval);
//    Gvalsmooth = (Gvalsmooth * (1-gsmoothfactor)) + (gsmoothfactor * Gval);
//    Bvalsmooth = (Bvalsmooth * (1-bsmoothfactor)) + (bsmoothfactor * Bval);

    //Write the RGB color channel values to the PWM driver for each pin
    analogWrite(redpin, (Rval * rgain));         
    analogWrite(greenpin, (Gval * ggain));         
    analogWrite(bluepin, (Bval * bgain));
    
    //Pause the program to slow down the visual display
    delay(15);
  }
  
  else if (mode == 0) {
    analogWrite(redpin, 100);         
    analogWrite(greenpin, 40);         
    analogWrite(bluepin, 8);
  }
  else if (mode == 333) {
    analogWrite(redpin, 0);         
    analogWrite(greenpin, 50);         
    analogWrite(bluepin, 0);
  }
  else if (mode == 44) {
    analogWrite(redpin, 0);         
    analogWrite(greenpin, 0);         
    analogWrite(bluepin, 50);
  }
  else if (mode == 33) {
    analogWrite(redpin, 50);         
    analogWrite(greenpin, 50);         
    analogWrite(bluepin, 50);
  }
  else if (mode == 44) {
    analogWrite(redpin, 0);
  }
  
  //Read the button state, and change modes if it's pressed
//  buttonstate = digitalRead(buttonpin);
//  if (buttonstate == 0) {
//    delay(50);
//    buttonpress();
//  }
    

//print system information over the serial port (for debugging purposes)
  Serial.print(largestbin);
  Serial.print("---");
  Serial.print(largestvalue);
  Serial.print("---");
  Serial.print(";");
  Serial.print(Rval);
  Serial.print(";");
  Serial.print(Gval);
  Serial.print(";");
  Serial.print(Bval);
  Serial.print(";");
  Serial.print(rainbowhue);
  Serial.print(";");
  Serial.print(mode);
  Serial.println();
  
  delay(5);

}

//function to convert HSV color space to RGB color space
void HSL(float H, float S, float L, float& Rval, float& Gval, float& Bval)
{
  float var_1;
  float var_2;
  float Hu=H+.33;
  float Hd=H-.33;
  if ( S == 0 )                       //HSL from 0 to 1
  {
    Rval = L * 255;                      //RGB results from 0 to 255
    Gval = L * 255;
    Bval = L * 255;
  }
  else
  {
    if ( L < 0.5 ) 
      var_2 = L * ( 1 + S );
    else           
      var_2 = ( L + S ) - ( S * L );

    var_1 = 2 * L - var_2;

    Rval = round(255 * Hue_2_RGB( var_1, var_2, Hu ));
    Gval = round(255 * Hue_2_RGB( var_1, var_2, H ));
    Bval = round(255 * Hue_2_RGB( var_1, var_2, Hd ));
    
    if (Rval < 0) {Rval = 0;}
    if (Gval < 0) {Gval = 0;}
    if (Bval < 0) {Bval = 0;}
  }

}

//function to debounce the switch input (e.g. prevent accidently count one physical button press multiple times)
void debounceInterrupt() {
  if((long)(micros() - last_micros) >= debouncing_time * 1000) {
    buttonpress();
    last_micros = micros();
  }
}

//function for what to do when a button press occurs
void buttonpress()
{
  mode++;
  if (mode >= numberofmodes) {
    mode = 0;
  }
  Serial.print("button press"); 
  Serial.print("  mode ="); 
  Serial.print(mode); 
  Serial.println("");
}

//helper code for the HSL function
float Hue_2_RGB( float v1, float v2, float vH )             //Function Hue_2_RGB
{
  if ( vH < 0 ) 
    vH += 1;
  if ( vH > 1 ) 
    vH -= 1;
  if ( ( 6 * vH ) < 1 ) 
    return ( v1 + ( v2 - v1 ) * 6 * vH );
  if ( ( 2 * vH ) < 1 ) 
    return ( v2 );
  if ( ( 3 * vH ) < 2 ) 
    return ( v1 + ( v2 - v1 ) * (.66-vH) * 6 );
  return ( v1 );
}

//very simple low-pass filter function
int smooth(int data, float filterVal, float smoothedVal){

  if (filterVal > 1){      // check to make sure param's are within range
    filterVal = .99;
  }
  else if (filterVal <= 0){
    filterVal = 0;
  }

  smoothedVal = (data * (1 - filterVal)) + (smoothedVal  *  filterVal);

  return (int)smoothedVal;
}


