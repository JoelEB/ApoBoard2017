#include <stdlib.h>

#include <avr/pgmspace.h>

#include <EEPROM.h>
#include <avr/sleep.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <avr/power.h> // Comment out this line for non-AVR boards (Arduino Due, etc.)
#include "IRSerial-2014.h"
#define NumEffects 10
#include "colorsets.h"

#define NeoPIN 7// was 10
#define NeoLEDs 10 //was 10 //number of addressable LEDs
#define Button 2

uint8_t brightness = 10; //global brightness
#define BUTTON_PIN 2
#define debounce_time 10 // button debounce in ms
// IR Parameters
#define IR_RX 8 //was 8
#define IR_TX 9 //was 9
#define IR_BAUD 300
IRSerial irSerial(IR_RX, IR_TX, false, true);

#define SERIAL_BAUD 115200

/////////// GLOBALS /////////////////
bool button_shadow = false;
int button = BUTTON_PIN;
int photopot = A1;
int colorModeMax = 5;
int colorMode = 0;

#define PUSHED false
#define NOT_PUSHED true
#define WATCH_BUTTON true
#define IGNORE_BUTTON false
const int buttonWaitInterval = 6000;
unsigned long previousMicros = 0;
boolean previousButtonState = NOT_PUSHED;
boolean debouncedButtonState = NOT_PUSHED;
boolean bounceState = false;


class Neo_event {
  public:

    int8_t event_type [NeoLEDs];
    uint8_t init_r [NeoLEDs];
    uint8_t init_g [NeoLEDs];
    uint8_t init_b [NeoLEDs];
    uint8_t end_r [NeoLEDs];
    uint8_t end_g [NeoLEDs];
    uint8_t end_b [NeoLEDs];
    int8_t diff_r [NeoLEDs];
    int8_t diff_g [NeoLEDs];
    int8_t diff_b [NeoLEDs];

    uint16_t event_duration [NeoLEDs];
    long unsigned int event_starttime [NeoLEDs];
    uint8_t current_r[NeoLEDs], current_g[NeoLEDs], current_b[NeoLEDs];

    static const int8_t
    event_setcolor = 0x10,
    event_fadeto = 0x20,
    event_wait = 0xF0;

    boolean setcolor(uint8_t LEDnum, uint32_t Color) {
      LEDnum %= NeoLEDs;
      event_type[LEDnum] = event_setcolor;
      init_g[LEDnum] = (Color >> 17) & 0x7F;
      init_r[LEDnum] = (Color >> 9) & 0x7F;
      init_b[LEDnum] = (Color >> 1) & 0x7F;
    }

    boolean fadeto(uint8_t LEDnum, uint32_t Color, uint16_t duration) {
      LEDnum %= NeoLEDs;
      event_type[LEDnum] = event_fadeto;
      end_g[LEDnum] = (Color >> 17) & 0x7F;
      end_r[LEDnum] = (Color >> 9) & 0x7F;
      end_b[LEDnum] = (Color >> 1) & 0x7F;
      init_r[LEDnum] = current_r[LEDnum];
      init_g[LEDnum] = current_g[LEDnum];
      init_b[LEDnum] = current_b[LEDnum];

      diff_g[LEDnum] = end_g[LEDnum] - init_g[LEDnum];
      diff_r[LEDnum] = end_r[LEDnum] - init_r[LEDnum];
      diff_b[LEDnum] = end_b[LEDnum] - init_b[LEDnum];
      /*Serial.print("fadeto: [");
        Serial.print(LEDnum);
        Serial.print("] ");
        Serial.print(init_g[LEDnum],HEX);
        Serial.print(" -> ");
        Serial.println(end_g[LEDnum],HEX);*/
      event_starttime[LEDnum] = millis();
      event_duration[LEDnum] = duration;
    }

    uint8_t applybrightness(uint8_t in, uint8_t intensity) {

      return (uint16_t) (in * intensity) / 100;
    }

    ////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////
    boolean wait(int waitfor, Adafruit_NeoPixel &strip ) {
      waitfor = waitfor > 0 ? waitfor : 0;
      uint32_t returnTime = millis() + waitfor;
      uint32_t out_color;
      for (int i = 0; i < NeoLEDs; i++) {
        switch (event_type[i]) {
          case event_setcolor :
            event_type[i] = 0;
            out_color = (((uint32_t) init_g[i]) << 16) + (((uint16_t) init_r[i]) << 8) + init_b[i];
            strip.setPixelColor(i, out_color) ;
            break;

          case event_fadeto :
            event_type[i] = -event_fadeto;
            break;
        }
      }
      strip.show();
      while (millis() < returnTime) {
        updateButton();

        for (int i = 0; i < NeoLEDs; i++) {
          int32_t eventprogress  = millis() - event_starttime[i];

          switch (event_type[i]) {
            case -event_fadeto :
              if (eventprogress < event_duration[i]) {
                current_r[i] = init_r[i] + (eventprogress * diff_r[i]) / event_duration[i];
                current_g[i] = init_g[i] + (eventprogress * diff_g[i]) / event_duration[i];
                current_b[i] = init_b[i] + (eventprogress * diff_b[i]) / event_duration[i];
              }
              else {
                init_r[i] = end_r[i];
                current_r[i] = end_r[i];
                init_g[i] = end_g[i];
                current_g[i] = end_g[i];
                init_b[i] = end_b[i];
                current_b[i] = end_b[i];
                event_type[i] = 0;
              }
              /*strip.setPixelColor(i, strip.Color(
                  applybrightness(current_g[i],brightness),
                  applybrightness(current_r[i],brightness),
                  applybrightness(current_b[i],brightness)
                  ));*/
              strip.setPixelColor(i, strip.Color(
                                    current_g[i],
                                    current_r[i],
                                    current_b[i]
                                  ));
              /*
                Serial.print(i);
                Serial.print(": ");
                Serial.print(end_g[i] - init_g[i],HEX);
                Serial.print(", ");
                Serial.print(diff_g[i],HEX);
                Serial.print(", ");
                Serial.println(current_g[i],HEX);
                delay(10); //slow down serial output
              */
              break;
          }
        }
        strip.show();
      }
    }


    void updateButton() {
      if (bounceState == WATCH_BUTTON) {
        boolean currentButtonState = digitalRead(BUTTON_PIN);
        if (previousButtonState != currentButtonState) {
          bounceState = IGNORE_BUTTON;
          previousMicros = micros();
        }
        previousButtonState = currentButtonState;
      }
      if (bounceState == IGNORE_BUTTON) {
        unsigned long currentMicros = micros();
        if ((unsigned long)(currentMicros - previousMicros) >= buttonWaitInterval) {
          debouncedButtonState = digitalRead(BUTTON_PIN);
          bounceState = WATCH_BUTTON;
        }
      }
    }

};



//-------------------------------------------------------------------------------------------------

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NeoLEDs, NeoPIN, NEO_GRB + NEO_KHZ800);

#define ENDLINE F("\n")

//experimental: try to get unique identifier from uninitiailized RAM
uint32_t PUF_hash()
{
  uint8_t const * p;
  uint8_t i;
  uint16_t hash = 0;

  uint8_t hash0 = 0;
  uint8_t hash1 = 0;

  p = (const uint8_t *)(0x8000);//was (8192 - 256);
  i = 0;
  do
  {
    hash0 ^= *p;
    p ++;
    hash1 ^= *p;
    p ++;
    i ++;
  }
  while ( i != 0 );
  hash = hash0 + (hash1 << 8);

  Serial.begin(115200);
  Serial.println(hash0, HEX);
  Serial.println(hash1, HEX);

  return (hash);
}


// Morse Code constants
unsigned char const morse[28] PROGMEM = {
  0x05,   // A  .-     00000101
  0x18,   // B  -...   00011000
  0x1A,   // C  -.-.   00011010
  0x0C,   // D  -..    00001100
  0x02,   // E  .      00000010
  0x12,   // F  ..-.   00010010
  0x0E,   // G  --.    00001110
  0x10,   // H  ....   00010000
  0x04,   // I  ..     00000100
  0x17,   // J  .---   00010111
  0x0D,   // K  -.-    00001101
  0x14,   // L  .-..   00010100
  0x07,   // M  --     00000111
  0x06,   // N  -.     00000110
  0x0F,   // O  ---    00001111
  0x16,   // P  .--.   00010110
  0x1D,   // Q  --.-   00011101
  0x0A,   // R  .-.    00001010
  0x08,   // S  ...    00001000
  0x03,   // T  -      00000011
  0x09,   // U  ..-    00001001
  0x11,   // V  ...-   00010001
  0x0B,   // W  .--    00001011
  0x19,   // X  -..-   00011001
  0x1B,   // Y  -.--   00011011
  0x1C,   // Z  --..   00011100
  0x01,   // space     00000001
  0x5A,   // @  .--.-. 01011010
};

//debugging macros
//#define SERIAL_TRACE
#ifdef SERIAL_TRACE
#define SERIAL_TRACE_LN(a) Serial.println(a);
#define SERIAL_TRACE(a) Serial.print(a);
#else
#define SERIAL_TRACE_LN(a)
#define SERIAL_TRACE(a)
#endif

//#define SERIAL_INFO
#ifdef SERIAL_INFO
#define SERIAL_INFO_LN(a) Serial.println(a);
#define SERIAL_INFO(a) Serial.print(a);
#else
#define SERIAL_INFO_LN(a)
#define SERIAL_INFO(a)
#endif
//end debugging macros



void readContestRunnerKeyAndGUID() {
#if MAKE_ME_A_CONTEST_RUNNER //make contest runner badge
  // Pull GUID and Private key from EEPROM
  for (char ndx = 0; ndx < 8; ndx++) {
    GUID[ndx] = EEPROM.read(UBER_CRYTPO_CONTEST_RUNNER_ADDR + 8 + ndx);
  }
  GUID[8] = 0;
  // Yes, this is big endian. I don't want to have to byte-swap
  // when building the EEPROM file from text strings.
  KEY[0] = (EEPROM.read(UBER_CRYTPO_CONTEST_RUNNER_ADDR + 0) << 8) + EEPROM.read(UBER_CRYTPO_CONTEST_RUNNER_ADDR + 1);
  KEY[1] = (EEPROM.read(UBER_CRYTPO_CONTEST_RUNNER_ADDR + 2) << 8) + EEPROM.read(UBER_CRYTPO_CONTEST_RUNNER_ADDR + 3);
  KEY[2] = (EEPROM.read(UBER_CRYTPO_CONTEST_RUNNER_ADDR + 4) << 8) + EEPROM.read(UBER_CRYTPO_CONTEST_RUNNER_ADDR + 5);
  KEY[3] = (EEPROM.read(UBER_CRYTPO_CONTEST_RUNNER_ADDR + 6) << 8) + EEPROM.read(UBER_CRYTPO_CONTEST_RUNNER_ADDR + 7);
#endif
}

// This is our receive buffer, for data we pull out of the
// IRSerial library.  This is _NOT_ the buffer the IRSerial
// library is using to receive bytes into.
// Our buffer only needs to be big enough to hold a staza
// to process.  Right now (2014) that's 12 bytes.  I'm over-sizing
// 'cuz why not?
#define RX_BUF_SIZE 32
unsigned char rxBuf[RX_BUF_SIZE];
unsigned char head;

// Increments a pointer into rxBuf, wrapping as appropriate.
unsigned char inc(unsigned char *p) {
  unsigned char q = *p;
  *p = (*p + 1) % RX_BUF_SIZE;
  return q;
}

// Returns the character in rxBuf[] that is p bytes offset from head.
// Deals with wrapping.  You'll probably want to pass a negative
// number as p.
unsigned char rxBufNdx(unsigned char p) {
  return rxBuf[(head + p) % RX_BUF_SIZE];
}

// Turns a 4 byte int (represented here as an array of bytes)
// into a "modified HEX" string.  tr/0-9a-f/A-P/   Makes it
// easier to send in morse code.
void intToStr(uint8_t *src, char *dst) {
  for (unsigned char ndx = 0; ndx < 8; ndx++) {
    dst[ndx] = ((src[ndx >> 1] >> (ndx % 2 ? 0 : 4)) & 0x0F) + 'A';
  }
  dst[8] = '\0';
}

void writeWord(uint8_t *buf) {
  char str[9];
  intToStr(buf, str);
  irSerial.println(str);
}

/* Processes the received buffer, pulls the 8 character "modified HEX"
   out of rxBuf[] (head points at the next byte after the end) and packs it into a
   4 byte array *packedBuf.  If provided, the original 8 "hex" characters
   are also copied into *strDst.
   Messages are of the form:   m/(0[xyab])([A-Z0-9]{8})\r\n/
   $1 is the header and specifies which message it is.
     0x = Alice's GUID beacon  (Alice -> Bob)
     0y = Bob's encrypted reply to Alice's GUID beacon  (Bob -> Alice)
     0a = Bob's GUID (Bob -> Alice, sent immediately after 0y)
     0b = Alice's encrypted reply to Bob's GUID  (Alice -> Bob)
     0w = Message from DarkNet Agent after you've solved the 6 part silk screen crypto
*/
unsigned char readWordFromBuf(uint8_t *packedBuf, unsigned char *strDst = 0) {
  // head points to the next character after the \r\n at the end
  // of our received bytes.  So head-10 points to the beginning of
  // our message, but it's a circular buffer, so we have to wrap.
  unsigned char rxNdx = (head - 10) % RX_BUF_SIZE;

  for (unsigned char ndx = 0; ndx < 8; ndx++, inc(&rxNdx)) {
    unsigned char packedPtr = ndx >> 1;  // index into *packedBuf
    unsigned char cur = rxBuf[rxNdx]; // current "HEX" character

    // Convert from our modified HEX into the actual nibble value.
    if (cur >= 'A' && cur <= 'P') {
      cur -= 'A';
    } else if (cur >= 'Q' && cur <= 'Z') {
      cur -= 'Q';
    } else if (cur >= '0' && cur <= '9') {
      cur = cur - '0' + 6;
    } else {
      Serial.print(F("readWordFromBuf() line noise: "));
      Serial.println(cur);
      return 0;  // Line noise.  Return and wait for the next oneo.
    }
    packedBuf[packedPtr] <<= 4;  // Shift up the previous nibble, filling with zeros
    packedBuf[packedPtr] |= (cur & 0x0F); // Add in the current nibble

    // If provided, also copy rxBuf into *strDst.
    if (strDst) {
      *(strDst++) = rxBuf[rxNdx];
    }
  }
  return 1;
}

unsigned char isValidWord() {
  // Check for valid framing.
  if (rxBufNdx(-1) != '\n' || rxBufNdx(-2) != '\r' || rxBufNdx(-12) != '0') {
    // Probably in the middle of receiving, nothing wrong.
    // Don't log anything, just return.
    return false;
  }

  // We have a good framing. Future failures will be reported.
  char c = rxBufNdx(-11);
  if (c != 'x' && c != 'y' && c != 'a' && c != 'b') {
    Serial.println(F("Bad rx header: "));
    Serial.println(c);
    return false;
  }
  for (int i = -10; i < -2; i++) {
    c = rxBufNdx(i);

    // If it's not a letter and not a number
    if (!(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') && !(c >= '0' && c <= '9')) {
      Serial.println(F("Bad rx data: "));
      Serial.println(i);
      Serial.println(F(" "));
      Serial.println(c);
      return false;
    }
  }
  return true;
}

// Reads more characters from the IR and processes them as they come in.
// This differs from processIR() below in that it waits 5000ms for a message
// to come in, rather than returning immediately.  So only call this if
// you're already in the middle of an exchange and know you want to wait.
unsigned char readWordFromIR() {
  head = 0;
  unsigned long start = millis();
  while (millis() - start < 5000) {  // koop for no more than 5 seconds
    if (!irSerial.available())
      continue;
    unsigned char c = irSerial.read();
    rxBuf[head] = c;
    head = (head + 1) % RX_BUF_SIZE;
    if (isValidWord())
      return 1;
  }
  return 0;
}
void clearRxBuf() {
  for (int ndx = 0; ndx < RX_BUF_SIZE; ndx++)
    rxBuf[ndx] = '-';
  head = 0;
}

/* The IRSerial library feeds us characters as they're received.
   processIR() puts the character on the buffer and sees
   if we have a properly formatted IR message. If we
   do, it kicks off the appropriate process, whether we are
   Alice or Bob.  If it doesn't it returns quickly so you can
   be doing other things (like LED animation!)
*/
void processIR(unsigned char c, Neo_event &ne) {
  //  Serial.write(c);
  //  Serial.print(" ");
  if (c == 0xFF) return;
  Serial.write(c); //SPECTER dbug
  rxBuf[head] = c;
  head = (head + 1) % RX_BUF_SIZE;

  // isValidWord() will print an error message if it finds
  // a good header, but an otherwise malformed packet.
  // Otherwise, a false return code just means it didn't
  // find a valid packet header, we're probably in the middle
  // of receiving a packet.
  if (!isValidWord()) return;

  // SPECTERSignal that we are receiving a packet and attempting an exchange
  //flashRandom(5, 5);  //delay, clones
  unsigned char flag = rxBufNdx(-11);
  int msgAddr = -1;
  if (flag == 'x') {
    Serial.println(F("Received Beacon"));
    uint8_t r =  char2tob(rxBufNdx(-10), rxBufNdx(-9));
    uint8_t g =  char2tob(rxBufNdx(- 8), rxBufNdx(-7));
    uint8_t b =  char2tob(rxBufNdx(- 6), rxBufNdx(-5));

    //OLD    neo.event_type = 100; //100 is a dummy positive value
    //    neo.init_r = r;
    //    neo.init_g = g;
    //    neo.init_b = b;
    //    neo.end_r = neo.end_g = neo.end_b = 0;
    //    neo.event_millis = 500;
  }

  clearRxBuf();
}

unsigned char char2tob(char hexH, char hexL) {
  return ((convertchartobyte(hexH) << 4) + convertchartobyte(hexL));
}

uint8_t convertchartobyte(char cur) {
  if (cur >= 'A' && cur <= 'F') {
    cur -= 'A' - 10;
  } else if (cur >= 'a' && cur <= 'f') {
    cur -= 'a' - 10;
  } else if (cur >= '0' && cur <= '9') {
    cur -= '0';
  } else {
    Serial.print(F("readWordFromBuf() line noise: "));
    Serial.println(cur);
    return 0;  // Line noise.  Return and wait for the next oneo.
  }
  return (cur);
}

/*
  //////////////////////////////////////////
  // code to calibrate oscillator
  static void calibrateOscillator(void)
  {
  SERIAL_INFO("Before calibrate: ");SERIAL_INFO_LN(OSCCAL);
  char step = 128;
  char trialValue = 0, optimumValue;
  int   x, optimumDev, targetValue = (unsigned)(1499 * (double)F_CPU / 10.5e6 + 0.5);
  //Serial.print("Target: " );Serial.println(targetValue);
    do{
        OSCCAL = trialValue + step;
        x = usbMeasureFrameLength();    // proportional to current real frequency
        //Serial.print("X: "); Serial.println(x);
        //Serial.print("S: ");Serial.println(step);
        //Serial.print("TV: ");Serial.println(trialValue);
        //Serial.print("OS: ");Serial.println(OSCCAL);
        if(x < targetValue)             // frequency still too low
            trialValue += step;
        step >>= 1;
    }while(step > 0);
    optimumValue = trialValue;
    //Serial.println(trialValue);
    optimumDev = x; // this is certainly far away from optimum
    for(OSCCAL = trialValue - 1; OSCCAL <= trialValue + 1; OSCCAL++){
      //Serial.println(OSCCAL);
        x = usbMeasureFrameLength() - targetValue;
        if(x < 0)
            x = -x;
        if(x < optimumDev){
            optimumDev = x;
            optimumValue = OSCCAL;
        }
    }
    OSCCAL = optimumValue;
    SERIAL_INFO("After Calibrate: ");SERIAL_INFO_LN(OSCCAL);
  }

  void usbEventResetReady(void)
  {
  //Serial.println("usbEventReset");
  cli();  // usbMeasureFrameLength() counts CPU cycles, so disable interrupts.
  calibrateOscillator();
  sei();
   //eeprom_write_byte(0, OSCCAL);   // store the calibrated value in EEPROM
  }

*/
/*
  void flashRandom(int wait, uint8_t howmany) {

  for (uint16_t i = 0; i < howmany; i++) {
    // pick a random favorite color!
    int c = random(FAVCOLORS);
    int red = ColorSets[c][0];
    int green = ColorSets[c][1];
    int blue = ColorSets[c][2];

    // get a random pixel from the list
    int j = random(strip.numPixels());

    // now we will 'fade' it in 5 steps
    for (int x = 0; x < 5; x++) {
      int r = red * (x + 1); r /= 5;
      int g = green * (x + 1); g /= 5;
      int b = blue * (x + 1); b /= 5;

      strip.setPixelColor(j, strip.Color(r, g, b));
      strip.show();
      delay(wait);
    }
    // & fade out in 5 steps
    for (int x = 5; x >= 0; x--) {
      int r = red * x; r /= 5;
      int g = green * x; g /= 5;
      int b = blue * x; b /= 5;

      strip.setPixelColor(j, strip.Color(r, g, b));
      strip.show();
      delay(wait);
    }
  }
  // NeoLEDs will be off when done (they are faded to 0)
  }

*/



void delayAndReadIRSpecter(int pauseFor, Neo_event &ne) {
  pauseFor = pauseFor > 0 ? pauseFor : 0;
  uint32_t returnTime = millis() + pauseFor;

  while (millis() < returnTime) {
    /*if (digitalRead(BUTTON_PIN) == LOW) {
      long buttonStart = millis();
      while(digitalRead(BUTTON_PIN) == LOW && false) {
        if (millis() - buttonStart > 2000) {
          buttonStart = 0;  // Flag to not dump USB
          break;
        }
      }

      if (buttonStart) {
        // Dump database to USB
        dumpDatabaseToUSB();
      }
      }
    */

    int ret;
    if (ret = irSerial.available()) {
      processIR(irSerial.read(), ne);
    }
  }
}


Neo_event neo;
Colorsets colorset;

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
void setup()
{
  uint32_t  PUFhash_result = PUF_hash();
  strip.begin();
  strip.setBrightness(0xff);
  for (int kill = 0; kill < NeoLEDs; kill ++) {
    strip.setPixelColor(kill, 0);
  }
  strip.show(); // Initialize all pixels to 'off'

  Serial.begin(SERIAL_BAUD);
  Serial.print("\n");
  Serial.print(F("PUFhash = "));
  Serial.print((PUFhash_result >> 16), HEX);
  Serial.println(PUFhash_result & 0xFFFF, HEX);

  // Setup various serial ports  // A modified version of SoftwareSerial to handle inverted logic // (Async serial normally idles in the HIGH state, which would burn // through battery on our IR, so inverted logic idles in the LOW state.) // Also modified to modulate output at 38kHz instead of just turning the // LED on.  Otherwise, it's a pretty standard SoftwareSerial library.
  irSerial.begin(IR_BAUD);
  irSerial.listen();
  digitalWrite(IR_TX, LOW); // For some reason, the TX line starts high and wastes some battery.
  pinMode(IR_RX, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.println("Starting kernel loop");

}
/////////////////////////////////////////////////////////////
/////////////////               /////////////////////////////
///////////////// E f f e c t s /////////////////////////////
/////////////////               /////////////////////////////
/////////////////////////////////////////////////////////////

uint8_t effect_counterA = 0; //global effect_counter for all animations

/////////////////////////////////////////////////////////////
const uint8_t portal_effect_anim[][2] = {  //turn on 2 LEDs per frame (period), if 0xFF then turn all off (period * 4)
  {3, 8},
  {4, 9},
  {5, 0},
  {0xFF, 0xFF}, //turn all leds off
  {2, 7},
  {1, 6},
  {0, 5},
  {0xFF, 0xFF}, //turn all leds off
};

#define portal_effect_anim_length 8

void NeoEffect_portal(uint8_t colorsetnum, Colorsets colorset, int period) {

  uint8_t ON = effect_counterA % portal_effect_anim_length;
  uint8_t OFF = (effect_counterA - 1) % portal_effect_anim_length;

  if (portal_effect_anim[ON][0] != 0xFF)
  {
    neo.fadeto( portal_effect_anim[ON][0], colorset.getFG(colorsetnum, 0) , period);
    neo.fadeto( portal_effect_anim[ON][1], colorset.getFG(colorsetnum, 0) , period);
    if ( portal_effect_anim[OFF][0] != 0xFF ) {
      neo.fadeto( portal_effect_anim[OFF][0], colorset.getBG(colorsetnum, 0) , period);
      neo.fadeto( portal_effect_anim[OFF][1], colorset.getBG(colorsetnum, 0) , period);
    }
    neo.wait(period, strip);
  }
  else
  {
    for (OFF = 0; OFF < NeoLEDs; OFF++)
    {
      neo.fadeto( OFF, colorset.getBG(0, 0), period << 2); //fade in half the time
    }
    neo.wait(period << 2, strip);
    FGcounter ++;
    BGcounter ++;
  }
  effect_counterA ++;
}
////////////////////////////////////////////////////////////////////////////////
/*       5
      4     6
   3           7
   2           8
      1     9
         0      */

const uint8_t portal2_effect_anim[] = {0, 1, 2, 0xFF, 7, 6, 5, 0xFF, 0, 9, 8, 0xFF, 3, 4, 5, 0xFF};

#define portal2_effect_anim_length 16

void NeoEffect_portal2(uint8_t colorsetnum, Colorsets colorset, int period) {

  uint8_t ON = effect_counterA % portal2_effect_anim_length;
  uint8_t OFF = (effect_counterA - 1) % portal2_effect_anim_length;

  if (portal2_effect_anim[ON] != 0xFF)
  {
    neo.fadeto( portal2_effect_anim[ON], colorset.getFG(colorsetnum, 0) , period);
    if ( portal2_effect_anim[OFF] != 0xFF ) 
    {
      neo.fadeto( portal2_effect_anim[OFF], colorset.getBG(colorsetnum, 0) , period);
    }
      if (ON == 0)
      {
        FGcounter ++;
        BGcounter ++;
      }
    neo.wait(period, strip);
  }

  else
  {
    for (OFF = 0; OFF < NeoLEDs; OFF++)
    {
      neo.fadeto( OFF, colorset.getBG(0, 0), period << 2); //fade in half the time
    }
    neo.wait(period << 2, strip);

  }
  effect_counterA ++;
}
////////////////////////////////////////////////////////////////////////////////
const uint8_t spider_effect_anim[] =
{5, 2, 7, 4, 9, 6, 1, 8, 3, 0};


#define spider_effect_anim_length 11

void NeoEffect_spider(uint8_t colorsetnum, Colorsets colorset, int period)
{
  uint8_t ON = effect_counterA % spider_effect_anim_length;
  uint8_t OFF = (effect_counterA - 1) % spider_effect_anim_length;

  neo.fadeto( spider_effect_anim[ON], colorset.getFG(colorsetnum, 1) , period);
  neo.fadeto( spider_effect_anim[OFF], colorset.getBG(colorsetnum, 0) , period >> 1);
  neo.wait(period, strip);
  if (spider_effect_anim[ON] == 0) {
    FGcounter ++;
    BGcounter ++;
  }
  effect_counterA ++;
}
////////////////////////////////////////////////////////////////////////////////
void NeoEffect_spider2(uint8_t colorsetnum, Colorsets colorset, int period) {

  uint8_t ON = effect_counterA % spider_effect_anim_length;
  uint8_t OFF = (effect_counterA - 1) % spider_effect_anim_length;

  neo.fadeto( spider_effect_anim[ON], colorset.getFG(colorsetnum, 0) , period << 1);
  neo.fadeto( spider_effect_anim[OFF], colorset.getBG(0 , 0) , period);
  neo.wait(period << 1, strip);
  effect_counterA ++;
  FGcounter ++;
  BGcounter ++;
}
////////////////////////////////////////////////////////////////////////////////
const uint8_t cylon_effect_anim[][2] = {  //turn on 2 LEDs per frame (period), if 0xFF then turn all off (period * 4)
  {2, 3},
  {1, 4},
  {0, 5},
  {9, 6},
  {8, 7},
  {0xFF, 0xFF}, //turn all leds off
  {8, 7},
  {9, 6},
  {0, 5},
  {1, 4},
  {2, 3},
  {0xFF, 0xFF}, //turn all leds off

};

#define cylon_effect_anim_length 12

void NeoEffect_cylon(uint8_t colorsetnum, Colorsets colorset, int period) {

  uint8_t ON = effect_counterA % cylon_effect_anim_length;
  uint8_t OFF = (effect_counterA - 1) % cylon_effect_anim_length;

  if (cylon_effect_anim[ON][0] != 0xFF) {
    neo.fadeto( cylon_effect_anim[ON][0], colorset.getFG(colorsetnum, 0) , period);
    neo.fadeto( cylon_effect_anim[ON][1], colorset.getFG(colorsetnum, 0) , period);
    if (cylon_effect_anim[OFF][0] != 0xFF) {
      neo.fadeto( cylon_effect_anim[OFF][0], colorset.getBG(colorsetnum, 0) , period);
      neo.fadeto( cylon_effect_anim[OFF][1], colorset.getBG(colorsetnum, 0) , period);
    }
    neo.wait(period, strip);
  }
  else {
    for (OFF = 0; OFF < NeoLEDs; OFF++) {
      neo.fadeto( OFF, colorset.getBG(0, 0), period << 2); //fade in half the time
    }
    neo.wait(period << 2, strip);
    FGcounter ++;
    BGcounter ++;
  }
  effect_counterA ++;
}
////////////////////////////////////////////////////////////////////////////////
/*       5
      4     6
   3           7
   2           8
      1     9
         0

*/
const uint8_t cylon2_effect_anim[][2] = {  //turn on 2 LEDs per frame (period), if 0xFF then turn all off (period * 4)
  {2, 3},
  {1, 4},
  {0, 5},
  {9, 6},
  {8, 7},
  {9, 6},
  {0, 5},
  {1, 4},

};

#define cylon2_effect_anim_length 8
void NeoEffect_cylon2(uint8_t colorsetnum, Colorsets colorset, int period) {

  uint8_t ON = effect_counterA % cylon2_effect_anim_length;
  uint8_t OFF = (effect_counterA - 1) % cylon2_effect_anim_length;

  neo.fadeto( cylon2_effect_anim[ON][0], colorset.getFG(colorsetnum, 0) , period);
  neo.fadeto( cylon2_effect_anim[ON][1], colorset.getFG(colorsetnum, 0) , period);
  neo.fadeto( cylon2_effect_anim[OFF][0], colorset.getBG(colorsetnum, 0) , period);
  neo.fadeto( cylon2_effect_anim[OFF][1], colorset.getBG(colorsetnum, 0) , period);
  neo.wait(period, strip);
  if (ON == 0 || ON == 5)
    FGcounter ++;
  if (ON == 5)
    BGcounter ++;
  effect_counterA ++;
}
////////////////////////////////////////////////////////////////////////////////
const uint8_t waterfall_effect_anim[][2] = {};

#define waterfall_effect_anim_length 8


void NeoEffect_waterfall(uint8_t colorsetnum, Colorsets colorset, int period)
{
  uint8_t ON = effect_counterA % waterfall_effect_anim_length;
  uint8_t OFF = (effect_counterA - 1) % waterfall_effect_anim_length;
}
////////////////////////////////////////////////////////////////////////////////
/*       5
      4     6
   3           7
   2           8
      1     9
         0
*/
const uint8_t zigzag_effect_anim[] = {2, 4, 0, 6, 8, 7, 9, 5, 1, 3};

#define zigzag_effect_anim_length 10

void NeoEffect_zigzag(uint8_t colorsetnum, Colorsets colorset, int period)
{
  uint8_t ON = effect_counterA % zigzag_effect_anim_length;
  uint8_t OFF = (effect_counterA - 1) % zigzag_effect_anim_length;


  neo.fadeto( zigzag_effect_anim[ON], colorset.getFG(colorsetnum, 0) , period);
  neo.fadeto( zigzag_effect_anim[OFF], colorset.getBG(colorsetnum , 0) , period << 1);
  neo.wait(period, strip);

  if (ON == 0)
  {
    FGcounter ++;
    BGcounter ++;
  }


  effect_counterA ++;
}
////////////////////////////////////////////////////////////////////////////////
/*       5
     4     6
  3           7
  2           8
     1     9
        0
*/
const uint8_t infinity_effect_anim[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 5, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5};

#define infinity_effect_anim_length 22

void NeoEffect_infinity(uint8_t colorsetnum, Colorsets colorset, int period)
{
  uint8_t ON = effect_counterA % infinity_effect_anim_length;
  uint8_t OFF = (effect_counterA - 1) % infinity_effect_anim_length;


  neo.fadeto( infinity_effect_anim[ON], colorset.getFG(colorsetnum, 0) , period);
  neo.fadeto( infinity_effect_anim[OFF], colorset.getBG(colorsetnum , 0) , period << 1);
  neo.wait(period, strip);

  if (ON == 0)
  {
    FGcounter ++;
    BGcounter ++;
  }


  effect_counterA ++;
}

////////////////////////////////////////////////////////////////////////////////
/*       5
     4     6
  3           7
  2           8
     1     9
        0
*/
const uint8_t NumSmileyFaces = 5;
const bool smileyFaces[][10] = {
  {1, 1, 1, 0, 1, 0, 1, 0, 1, 1}, // basic smile and two eyes
  {1, 1, 1, 0, 0, 0, 0, 0, 1, 1}, // no eyes, just smile
  {1, 1, 1, 0, 0, 0, 1, 0, 1, 1}, // smile and left wink
  {1, 1, 1, 0, 1, 0, 0, 0, 1, 1}, // smile and right wink
  {1, 1, 1, 0, 0, 0, 0, 0, 1, 1}, // no eyes, just smile
  {1, 0, 0, 0, 1, 0, 1, 0 ,0, 0} // small mouth, two eyes
};
uint8_t smiley_rotation = 0;
void NeoEffect_smiley(uint8_t colorsetnum, Colorsets colorset, int period) {
  uint8_t face = 0;
  uint8_t r = random(0x100);
  if ( r > 0xF0 )
    face = random(NumSmileyFaces - 1) + 1;
  if ( r < 0x04 )
    smiley_rotation = random(2) + 9;

  if ( r == 0x80) {
    FGcounter ++;
    BGcounter ++;
  }
  draw_smiley(face, smiley_rotation, colorsetnum, colorset, period);
  //strip.setBrightness((r % 0x10) + 0x30);
  strip.setBrightness(30);
  neo.wait(period, strip);
  effect_counterA ++;
  //smiley_rotation ++;
}

void draw_smiley(uint8_t LEDimg, uint8_t rot, uint8_t colorsetnum, Colorsets colorset, int period) {
  for (uint8_t i = 0; i < 10; i++) {
    if (smileyFaces[LEDimg][i] == 1) neo.fadeto( (i + rot) % NeoLEDs, colorset.getFG(colorsetnum, 0) , period >> 4);
    else neo.fadeto( (i + rot) % NeoLEDs , colorset.getBG(0, 0) , period);
  }
}





////////////////////////////////////////////////////////////////////////////////
uint8_t current_effect = 0;
uint8_t colorsetnum = 6;

void loop()
{
  switch (current_effect) {
    case 0: NeoEffect_smiley (colorsetnum, colorset, 300); break;
    case 1: NeoEffect_spider (colorsetnum, colorset, 100); break;
    case 2: NeoEffect_spider2(colorsetnum, colorset, 10);  break;
    case 3: NeoEffect_cylon  (colorsetnum, colorset, 200);  break;
    case 4: NeoEffect_cylon2 (colorsetnum, colorset, 200);  break;
    case 5: NeoEffect_zigzag (colorsetnum, colorset, 350);  break;
    case 6: NeoEffect_infinity(colorsetnum, colorset, 100);  break;
    case 7: NeoEffect_portal (colorsetnum, colorset, 200); break;
    case 8: NeoEffect_portal2 (colorsetnum, colorset, 150); break;
  }
  /*neo.wait(10, strip); //dummy wait to set debuncedButtonState
  if (!debouncedButtonState) {
    debouncedButtonState = 1;
    current_effect = (current_effect + 1) % NumEffects;
    //Serial.print("Effect = ");
    //Serial.print("\t");
    //Serial.println(current_effect);
  }
  */
}

