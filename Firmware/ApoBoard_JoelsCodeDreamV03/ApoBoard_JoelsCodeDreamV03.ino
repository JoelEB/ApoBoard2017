
#include <stdlib.h>

#include <avr/pgmspace.h>

#include <EEPROM.h>
#include <avr/sleep.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <avr/power.h> // Comment out this line for non-AVR boards (Arduino Due, etc.)
#include "IRSerial-2014.h"
#include "colorsets.h"

#define NeoPIN 7// was 10
#define NeoLEDs 10 //was 10 //number of addressable LEDs
#define Button 2
#define MaxGenes 20

uint8_t brightness = 25; //global brightness
#define BUTTON_PIN 2
#define debounce_time 10 // button debounce in ms
// IR Parameters
#define IR_RX 8 //was 8
#define IR_TX 9 //was 9
#define IR_BAUD 300
IRSerial ir(IR_RX, IR_TX, false, true);

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
unsigned long previousMicros = 0 , previousPreviousMicros = 0;
boolean previousButtonState = NOT_PUSHED;
boolean debouncedButtonState = NOT_PUSHED;
unsigned long debouncedButtonHeld = 0;
boolean bounceState = WATCH_BUTTON;


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
      current_g[LEDnum] = end_g[LEDnum] = init_g[LEDnum] = (Color >> 17) & 0x7F;
      current_r[LEDnum] = end_r[LEDnum] = init_r[LEDnum] = (Color >> 9) & 0x7F;
      current_b[LEDnum] = end_b[LEDnum] = init_b[LEDnum] = (Color >> 1) & 0x7F;

    }

    uint32_t getcolor (uint8_t i) {
      return
        ((uint32_t)current_g[i] << 16) |
        ((uint32_t)current_r[i] <<  8) |
        (uint32_t)current_b[i]

        ;
    }
    boolean setcolor_now(uint8_t LEDnum, uint32_t Color, Adafruit_NeoPixel &strip ) {
      LEDnum %= NeoLEDs;
      // now instant  was  event_type[LEDnum] = event_setcolor;
      current_g[LEDnum] = end_g[LEDnum] = init_g[LEDnum] = (Color >> 17) & 0x7F;
      current_r[LEDnum] = end_r[LEDnum] = init_r[LEDnum] = (Color >> 9) & 0x7F;
      current_b[LEDnum] = end_b[LEDnum] = init_b[LEDnum] = (Color >> 1) & 0x7F;
      strip.setPixelColor(LEDnum, strip.Color(
                            end_g[LEDnum],
                            end_r[LEDnum],
                            end_b[LEDnum]
                          )) ;

    }

    boolean fadeto(uint8_t LEDnum, uint32_t Color, uint16_t duration) {
      LEDnum %= NeoLEDs;
      end_g[LEDnum] = (Color >> 17) & 0x7F;
      end_r[LEDnum] = (Color >> 9) & 0x7F;
      end_b[LEDnum] = (Color >> 1) & 0x7F;
      init_r[LEDnum] = current_r[LEDnum];
      init_g[LEDnum] = current_g[LEDnum];
      init_b[LEDnum] = current_b[LEDnum];

      diff_g[LEDnum] = end_g[LEDnum] - init_g[LEDnum];
      diff_r[LEDnum] = end_r[LEDnum] - init_r[LEDnum];
      diff_b[LEDnum] = end_b[LEDnum] - init_b[LEDnum];
      /*Serial.print(F("fadeto: [");
        Serial.print(LEDnum);
        Serial.print(F("] ");
        Serial.print(init_g[LEDnum],HEX);
        Serial.print(F(" -> ");
        Serial.println(end_g[LEDnum],HEX);*/
      event_starttime[LEDnum] = millis();
      event_duration[LEDnum] = duration;
      event_type[LEDnum] = event_fadeto;

    }

    uint8_t applybrightness(uint8_t in, uint8_t intensity) {

      return (uint16_t) (in * intensity) / 100;
    }

    ////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////
    boolean wait(int waitfor, Adafruit_NeoPixel &strip ) {
      waitfor = waitfor > 0 ? waitfor : 0;
      uint32_t returnTime = millis() + waitfor;
      for (int i = 0; i < NeoLEDs; i++) {
        switch (event_type[i]) {
          case event_setcolor :
            event_type[i] = 0;
            strip.setPixelColor(i, strip.Color(
                                  end_g[i],
                                  end_r[i],
                                  end_b[i]
                                )) ;
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
                Serial.print(F(": ");
                Serial.print(end_g[i] - init_g[i],HEX);
                Serial.print(F(", ");
                Serial.print(diff_g[i],HEX);
                Serial.print(F(", ");
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
          previousPreviousMicros = previousMicros;
          bounceState = IGNORE_BUTTON;
          previousMicros = micros();
        }
        previousButtonState = currentButtonState;
      }
      if (bounceState == IGNORE_BUTTON) {
        unsigned long currentMicros = micros();
        if ((unsigned long)(currentMicros - previousMicros) >= buttonWaitInterval) {
          debouncedButtonState = digitalRead(BUTTON_PIN);
          if (debouncedButtonState)
            debouncedButtonHeld = (currentMicros - previousPreviousMicros);
          bounceState = WATCH_BUTTON;
        }
      }

    }

};



//-------------------------------------------------------------------------------------------------

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NeoLEDs, NeoPIN, NEO_GRB + NEO_KHZ800);
Neo_event neo;
Colorsets colorset;
#define MaxEffects 15 // please keep this up-to-date!
#define eeprom_NumGenes 0
#define eeprom_CRC16    1 //16-bit checksum of genes
#define eeprom_genes_start 10
uint16_t all_genes [MaxGenes]; //effect | colorset
int8_t NumGenes;

int check_serial_cmd() {
  static char buffer[4];
  if (readline(Serial.read(), buffer, 4) != -1) { //forgive me for the return fail value
    Serial.print(F("CMD: "));
    Serial.println(buffer);
    if (!strcmp(buffer, "p"))
      return serial_cmd_program();
    else if (!strcmp(buffer, "m"))
      serial_cmd_lameegg();
    else if (!strcmp(buffer, "?"))
      serial_cmd_help();
    else if (!strcmp(buffer, "r")) {
      if (EEPROM.read(eeprom_NumGenes) <= MaxGenes) {
        dump_eeprom_genes();

      }
      else {
        Serial.print(F("read error: NumGenes="));
        Serial.println(NumGenes);
      }
    }
    else if (!strcmp(buffer, "w"))
      corrupt_gene0();
    else
      Serial.println(F("unknown command"));
  }
}

void serial_cmd_help() {
  Serial.println(F("Serial command help:"));
  Serial.println(F("p : program"));
  Serial.println(F("r : dump eeprom"));
  Serial.println(F("w : wipe genes (reversible)"));
  Serial.println(F("m : fallout 4"));
}

int serial_cmd_program() {
  static char buffer[6];
  Serial.println(F("Program mode.  Enter a gene #"));
  while (readline(Serial.read(), buffer, 6) < 0) {}
  int8_t gene = atoi(buffer);
  if (gene >= 0 && gene < MaxGenes) {
    Serial.println(F("Enter an effect #"));
    while (readline(Serial.read(), buffer, 6) < 0) {}
    int8_t effect = atoi(buffer);
    if (effect >= 0 && effect < MaxEffects) {
      Serial.println(F("Enter a colorset #"));
      while (readline(Serial.read(), buffer, 6) < 0) {}
      int8_t colorset = atoi(buffer);
      if (colorset >= 0 && colorset < MaxColorsets) {
        uint16_t genome = geneof(effect, colorset);
        Serial.print(F("Setting gene #"));
        Serial.print(gene);
        Serial.print(F(" to "));
        Serial.println(genome, HEX);
        all_genes[gene] = genome;
        NumGenes = gene + 1;
        copy_genes_to_EEPROM(all_genes, NumGenes);
        return NumGenes;

      }
    }
    else return -1;
  }
  else return -1;
}

//no go here. bad code! does not work!
void serial_cmd_lameegg() {
  Serial.println(F("Loading"));
  uint8_t thiscolorset = random(MaxColorsets);
  while (true) {
    NeoEffect_loading (thiscolorset, colorset, 200 );
    Serial.print(F("."));
  }
}

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

  return (hash);
}

//CRC-8 - based on the CRC8 formulas by Dallas/Maxim
//code released under the therms of the GNU GPL 3.0 license
byte CRC8(const byte * data, byte len) {
  byte crc = 0x00;
  while (len--) {
    byte extract = *data++;
    for (byte tempI = 8; tempI; tempI--) {
      byte sum = (crc ^ extract) & 0x01;
      crc >>= 1;
      if (sum) {
        crc ^= 0x8C;
      }
      extract >>= 1;
    }
  }
  return crc;
}

//Check routine taken from
//http://web.mit.edu/6.115/www/miscfiles/amulet/amulet-help/xmodem.htm
uint16_t CRC16(uint16_t arr[], int count)
{

  uint16_t  crc = 0;
  uint8_t i;
  int a;

  while (count-- >= 0)
  {
    //Serial.print("CRC of byte #");
    //Serial.println(a);
    crc ^= arr[a];
    a ++;
    for (i = 8; i > 0 ; i--) {
      if (crc & 1)
        crc = (crc >> 1) ^ 0xA001;
      else
        crc = (crc >> 1);
    }
  }
  return (crc);
}

uint16_t
crc16_update(uint16_t crc, uint8_t a)
{
  int i;

  crc ^= a;
  for (i = 0; i < 8; ++i)
  {
    if (crc & 1)
      crc = (crc >> 1) ^ 0xA001;
    else
      crc = (crc >> 1);
  }

  return crc;
}

int8_t readline(uint8_t readch, char *buffer, int len)
{
  static uint8_t pos = 0;
  uint8_t rpos;
  if (readch != 0xFF) {
    switch (readch) {
      case '\n': // Ignore new-lines
      case '\r': // Return on CR
        rpos = pos;
        pos = 0;  // Reset position index ready for next time
        return rpos;
      default:
        if (pos < len - 1) {
          buffer[pos++] = readch;
          buffer[pos] = 0;
        }
    }
  }
  // No end of line has been found, so return -1.
  return -1;
}

/* EXAMPLE for readline useage
  void setup()
  {
  Serial.begin(9600);
  }

  void looq()
  {
  static char buffer[80];
  if (readline(Serial.read(), buffer, 80) > 0) {
    Serial.print("You entered: >");
    Serial.print(buffer);
    Serial.println("<");
  }
  }

*/



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
  ir.println(str);
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
    if (!ir.available())
      continue;
    unsigned char c = ir.read(); //old defcon code
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
void processIR(unsigned char c, Neo_event & ne) {
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



void delayAndReadIRSpecter(int pauseFor, Neo_event & ne) {
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
    if (ret = ir.available()) {
      processIR(ir.read(), ne); //old defcon code
    }
  }
}


/////////////////////////////////////////////////////////////
/////////////////               /////////////////////////////
///////////////// S E T U P     /////////////////////////////
/////////////////               /////////////////////////////
/////////////////////////////////////////////////////////////
uint8_t current_gene = 0;

void setup()
{
  uint32_t  PUFhash_result = PUF_hash();
  strip.begin();
  strip.setBrightness(30);
  for (int kill = 0; kill < NeoLEDs; kill ++) {
    strip.setPixelColor(kill, 0);
  }
  strip.show(); // Initialize all pixels to 'off'

  Serial.begin(SERIAL_BAUD);
  Serial.setTimeout(10); //used in check_serial_cmd
  Serial.print("\n");
  Serial.print(F("PUFhash = "));
  Serial.print((PUFhash_result >> 16), HEX);
  Serial.println(PUFhash_result & 0xFFFF, HEX);

  // Setup various serial ports  // A modified version of SoftwareSerial to handle inverted logic // (Async serial normally idles in the HIGH state, which would burn // through battery on our IR, so inverted logic idles in the LOW state.) // Also modified to modulate output at 38kHz instead of just turning the // LED on.  Otherwise, it's a pretty standard SoftwareSerial library.
  ir.begin(IR_BAUD);
  ir.listen();
  digitalWrite(IR_TX, LOW); // For some reason, the TX line starts high and wastes some battery.
  pinMode(IR_RX, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  NumGenes = copy_EEPROM_to_genes();
  debouncedButtonHeld = 0;
  Serial.println("Starting PROPAGAND-EYE kernel:");

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

void NeoEffect_portal(uint8_t colorsetnum, Colorsets & colorset, int period, uint8_t brightness) {

  uint8_t ON = effect_counterA % portal_effect_anim_length;
  uint8_t OFF = (effect_counterA - 1) % portal_effect_anim_length;

  if (portal_effect_anim[ON][0] != 0xFF)
  {
    neo.fadeto( portal_effect_anim[ON][0], colorset.getFG(colorsetnum) , period);
    neo.fadeto( portal_effect_anim[ON][1], colorset.getFG(colorsetnum) , period);
    if ( portal_effect_anim[OFF][0] != 0xFF ) {
      neo.fadeto( portal_effect_anim[OFF][0], colorset.getBG(colorsetnum) , period);
      neo.fadeto( portal_effect_anim[OFF][1], colorset.getBG(colorsetnum) , period);
    }
    neo.wait(period, strip);
  }
  else
  {
    for (OFF = 0; OFF < NeoLEDs; OFF++)
    {
      neo.fadeto( OFF, colorset.getBG(0), period << 2); //fade in half the time
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

void NeoEffect_portal2(uint8_t colorsetnum, Colorsets & colorset, int period,  uint8_t brightness) {

  uint8_t ON = effect_counterA % portal2_effect_anim_length;
  uint8_t OFF = (effect_counterA - 1) % portal2_effect_anim_length;

  if (portal2_effect_anim[ON] != 0xFF)
  {
    neo.fadeto( portal2_effect_anim[ON], colorset.getFG(colorsetnum) , period);
    if ( portal2_effect_anim[OFF] != 0xFF )
    {
      neo.fadeto( portal2_effect_anim[OFF], colorset.getBG(colorsetnum) , period);
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
      neo.fadeto( OFF, colorset.getBG(0), period << 2); //fade in half the time
    }
    neo.wait(period << 2, strip);

  }
  effect_counterA ++;
}
////////////////////////////////////////////////////////////////////////////////
const uint8_t spider_effect_anim[] =
{5, 2, 7, 4, 9, 6, 1, 8, 3, 0};


#define spider_effect_anim_length 11

void NeoEffect_spider(uint8_t colorsetnum, Colorsets & colorset, int period,  uint8_t brightness)
{
  uint8_t ON = effect_counterA % spider_effect_anim_length;
  uint8_t OFF = (effect_counterA - 1) % spider_effect_anim_length;

  neo.fadeto( spider_effect_anim[ON], colorset.getFG(colorsetnum) , period);
  neo.fadeto( spider_effect_anim[OFF], colorset.getBG(colorsetnum) , period >> 1);
  neo.wait(period, strip);
  if (spider_effect_anim[ON] == 0) {
    FGcounter ++;
    BGcounter ++;
  }
  effect_counterA ++;
}
////////////////////////////////////////////////////////////////////////////////
void NeoEffect_spider2(uint8_t colorsetnum, Colorsets & colorset, int period, uint8_t brightness) {

  uint8_t ON = effect_counterA % spider_effect_anim_length;
  uint8_t OFF = (effect_counterA - 1) % spider_effect_anim_length;

  neo.fadeto( spider_effect_anim[ON], colorset.getFG(colorsetnum) , period << 1);
  neo.fadeto( spider_effect_anim[OFF], colorset.getBG(0) , period);
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

void NeoEffect_cylon(uint8_t colorsetnum, Colorsets & colorset, int period,  uint8_t brightness) {

  uint8_t ON = effect_counterA % cylon_effect_anim_length;
  uint8_t OFF = (effect_counterA - 1) % cylon_effect_anim_length;

  if (cylon_effect_anim[ON][0] != 0xFF) {
    neo.fadeto( cylon_effect_anim[ON][0], colorset.getFG(colorsetnum) , period);
    neo.fadeto( cylon_effect_anim[ON][1], colorset.getFG(colorsetnum) , period);
    if (cylon_effect_anim[OFF][0] != 0xFF) {
      neo.fadeto( cylon_effect_anim[OFF][0], colorset.getBG(colorsetnum) , period);
      neo.fadeto( cylon_effect_anim[OFF][1], colorset.getBG(colorsetnum) , period);
    }
    neo.wait(period, strip);
  }
  else {
    for (OFF = 0; OFF < NeoLEDs; OFF++) {
      neo.fadeto( OFF, colorset.getBG(0), period << 2); //fade in half the time
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
void NeoEffect_cylon2(uint8_t colorsetnum, Colorsets & colorset, int period,  uint8_t brightness) {

  uint8_t ON = effect_counterA % cylon2_effect_anim_length;
  uint8_t OFF = (effect_counterA - 1) % cylon2_effect_anim_length;

  neo.fadeto( cylon2_effect_anim[ON][0], colorset.getFG(colorsetnum) , period);
  neo.fadeto( cylon2_effect_anim[ON][1], colorset.getFG(colorsetnum) , period);
  neo.fadeto( cylon2_effect_anim[OFF][0], colorset.getBG(colorsetnum) , period);
  neo.fadeto( cylon2_effect_anim[OFF][1], colorset.getBG(colorsetnum) , period);
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


void NeoEffect_waterfall(uint8_t colorsetnum, Colorsets & colorset, int period, uint8_t brightness)
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

void NeoEffect_zigzag(uint8_t colorsetnum, Colorsets & colorset, int period, uint8_t brightness)
{
  uint8_t ON = effect_counterA % zigzag_effect_anim_length;
  uint8_t OFF = (effect_counterA - 1) % zigzag_effect_anim_length;


  neo.fadeto( zigzag_effect_anim[ON], colorset.getFG(colorsetnum) , period);
  neo.fadeto( zigzag_effect_anim[OFF], colorset.getBG(colorsetnum) , period << 1);
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

void NeoEffect_infinity(uint8_t colorsetnum, Colorsets & colorset, int period,  uint8_t brightness)
{
  uint8_t ON = effect_counterA % infinity_effect_anim_length;
  uint8_t OFF = (effect_counterA - 1) % infinity_effect_anim_length;


  neo.fadeto( infinity_effect_anim[ON], colorset.getFG(colorsetnum) , period);
  neo.fadeto( infinity_effect_anim[OFF], colorset.getBG(colorsetnum) , period << 1);
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

#define NumSmileyFaces
PROGMEM const uint8_t smileyFaces[NumSmileyFaces][10] = {  //memopt
  {1, 1, 1, 0, 1, 0, 1, 0, 1, 1}, // basic smile and two eyes
  {1, 1, 1, 0, 0, 0, 0, 0, 1, 1}, // no eyes, just smile
  {1, 1, 1, 0, 0, 0, 1, 0, 1, 1}, // smile and left wink
  {1, 1, 1, 0, 1, 0, 0, 0, 1, 1}, // smile and right wink
  {1, 0, 0, 0, 1, 0, 1, 0, 0, 0} // small mouth, two eyes
};
uint8_t smiley_rotation = 0;
bool update_smiley = true;
uint8_t smiley_face = 0;

void NeoEffect_smiley(uint8_t colorsetnum, Colorsets & colorset, int period,  uint8_t brightness) {
  uint8_t r = random(0x100);
  if ( r > 0xF0 ) {
    if (r < 0xF3) {
      smiley_face = random(NumSmileyFaces - 1) + 1;
      update_smiley = true;
    }
    else if (smiley_face != 1) {
      smiley_face = 1; //maybe do other smiley_face, but mostly just blink both eyes.
      update_smiley = true;
    }
  }
  //return to normal smiley face after if another expression was set
  else if (smiley_face != 0) {
    smiley_face = 0;
    update_smiley = true;
  }
  if ( r < 0x04 ) {
    smiley_rotation = random(2) + 9;
    update_smiley = true;
  }
  if ( r == 0x80) {
    FGcounter ++;
    BGcounter ++;
    update_smiley = true;
  }
  strip.setBrightness(30);
  if (update_smiley) {
    for (uint8_t i = 0; i < NeoLEDs; i++) {
      uint8_t led = (i + smiley_rotation) % NeoLEDs;
      if (pgm_read_byte(&(smileyFaces[smiley_face][i])))
        neo.fadeto( led, colorset.getFG(colorsetnum) , period >> 4);
      else
        neo.fadeto( led , colorset.getBG(0) , period);
    }
    update_smiley = false;
  }
  neo.wait(period, strip);
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

#define pacmanNumFrames 9
PROGMEM const uint8_t pacmanFaces[pacmanNumFrames][10]  = {  //memopt
  {1, 1, 1, 1, 2, 1, 1, 1, 1, 1}, // pacman closed mouth
  {1, 1, 1, 1, 2, 1, 1, 1, 1, 1}, // pacman closed mouth
  {1, 1, 1, 1, 2, 1, 1, 1, 1, 1}, // pacman closed mouth
  {1, 1, 1, 1, 2, 1, 1, 0, 0, 1}, // pacman middle mouth
  {1, 1, 1, 1, 2, 1, 0, 0, 0, 0}, // pacman open mouth
  {0, 1, 1, 1, 2, 0, 0, 0, 0, 0}, // pacman moar open mouth
  {0, 1, 1, 1, 2, 0, 0, 0, 0, 0}, // pacman moar open mouth
  {1, 1, 1, 1, 2, 1, 0, 0, 0, 0}, // pacman open mouth
  {1, 1, 1, 1, 2, 1, 1, 0, 0, 1}, // pacman middle mouth

};
uint8_t pacman_rotation = 0;

void NeoEffect_pacmania(uint8_t colorsetnum, Colorsets & colorset, int period, uint8_t brightness) {
  uint8_t r = random(0x100);
  if ( r < 0x01 ) {
    pacman_rotation = (r % 2) * 5;
  }
  if ( r == 0x80) {
    FGcounter ++;
    BGcounter ++;
  }

  for (uint8_t i = 0; i < NeoLEDs; i++) {
    uint8_t led = (i + pacman_rotation) % NeoLEDs;
    uint8_t pixel = pgm_read_byte(&(pacmanFaces[effect_counterA % pacmanNumFrames][i]));
    if (pixel) {
      neo.setcolor_now( led, colorset.getFG(colorsetnum, pixel), strip );
    }
    else
      neo.setcolor_now( led , colorset.getBG(0), strip );
  }
  strip.show();
  neo.wait(period, strip);

  effect_counterA ++;
}



#define eyeblinkNumFrames 2
PROGMEM const uint8_t eyeblink[eyeblinkNumFrames][10]  = {  //memopt
  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, // eyeblink open
  {1, 1, 1, 0, 0, 0, 0, 0, 1, 1}, // eyeblink closed
};
uint8_t current_frame;
uint8_t eyeframe_counter = 0;
uint8_t eyeblink_frames [3] = {1, 0, 1};

void NeoEffect_eyeblink(uint8_t colorsetnum, Colorsets & colorset, int period, uint8_t brightness) {
  uint8_t r = random(25);
  if ( r % 5 ) {
    current_frame = 1;
    eyeframe_counter = 3;
  }
  else if (!eyeframe_counter)
    current_frame = 0;
  if (eyeframe_counter) {
    current_frame = eyeblink_frames[eyeframe_counter - 1 ];
    eyeframe_counter--;
    if (!eyeframe_counter)  {
      if (r == 0) {
        FGcounter++;
        BGcounter++;
      }
    }
  }
  for (uint8_t led = 0; led < NeoLEDs; led++) {
    uint8_t pixel = pgm_read_byte(&(eyeblink[current_frame][led]));
    if (pixel) {
      neo.setcolor_now( led, colorset.getFG(colorsetnum, pixel + current_frame), strip );
    }
    else
      neo.setcolor_now( led , colorset.getBG(0), strip );
  }
  strip.show();
  neo.wait(period, strip);

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
// optimized away by SPECTER   const uint8_t loading_effect_anim[] PROGMEM = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}; //memopt

#define loading_effect_anim_length 10

void NeoEffect_loading (uint8_t colorsetnum, Colorsets & colorset, int period)//, uint8_t brightness)
{
  uint8_t ON = effect_counterA % loading_effect_anim_length;
  uint8_t OFF = (effect_counterA - 1) % loading_effect_anim_length;


  neo.fadeto( ON, colorset.getFG(colorsetnum) , period);
  neo.fadeto( OFF, colorset.getBG(colorsetnum) , period << 2);
  neo.wait(period, strip);

  if (ON == 0)
  {
    FGcounter ++;
    BGcounter ++;
  }

  strip.setBrightness( brightness );

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
void NeoEffect_stephen (uint8_t colorsetnum, Colorsets & colorset, int period)//, uint8_t brightness)
{
  uint8_t firstLEDON = effect_counterA % loading_effect_anim_length;
  uint8_t firstLEDOFF = (effect_counterA - 1) % loading_effect_anim_length;

  uint8_t secondLEDON = ~effect_counterA % loading_effect_anim_length;
  uint8_t secondLEDOFF = ~(effect_counterA - 1) % loading_effect_anim_length;


  neo.fadeto( firstLEDON, colorset.getFG(colorsetnum) , period);
  neo.fadeto( firstLEDOFF, colorset.getBG(colorsetnum) , period << 2);

  
  neo.fadeto( secondLEDON, colorset.getFG(colorsetnum) , period);
  neo.fadeto( secondLEDOFF, colorset.getBG(colorsetnum) , period << 2);
  neo.wait(period, strip);

  if (firstLEDON == 0)
  {
    FGcounter ++;
    BGcounter ++;

    if(random(10)) {
      NeoEffect_BufferedFlash(colorset.getFG(colorsetnum), random(250,500));
      FGcounter++;
      NeoEffect_BufferedFlash(colorset.getFG(colorsetnum), random(1000, 2000));
      FGcounter++;
      NeoEffect_BufferedFlash(colorset.getFG(colorsetnum), 1500);
    }
  }

  strip.setBrightness( brightness );

  effect_counterA ++;
}

uint32_t rgb2color(uint8_t r, uint8_t g, uint8_t b)
{
  return ((((uint32_t) g) << 16) + (((uint16_t) r) << 8) + b);
}


uint16_t poolofradiance_buffers [NeoLEDs][2][3];
uint8_t poolofradiance_current_buffer = 0;
void NeoEffect_poolofradiance (uint8_t colorsetnum, Colorsets & colorset, int period, uint8_t brightness) {
  uint8_t poolofradiance_other_buffer = poolofradiance_current_buffer ^ 1;
  for (uint8_t i = 0; i < NeoLEDs; i++) {
    for (uint8_t c = 0; c < 3; c++) {
      uint16_t accumulator =
        (poolofradiance_buffers[(i - 1) % NeoLEDs] [poolofradiance_current_buffer] [c] * .3) +
        (poolofradiance_buffers[(i + 1) % NeoLEDs] [poolofradiance_current_buffer] [c] * .3) +
        poolofradiance_buffers[i % NeoLEDs]        [poolofradiance_current_buffer] [c];
      accumulator *= .55;
      poolofradiance_buffers[i] [poolofradiance_other_buffer] [c] = accumulator;
    }
  }
  if (!random(5)) {
    uint8_t i = random(10);
    uint32_t color = colorset.getFG(colorsetnum);
    poolofradiance_buffers[i][poolofradiance_other_buffer][0] =  (color >> 8)  & 0xFF00;
    poolofradiance_buffers[i][poolofradiance_other_buffer][1] =  color  & 0xFF00;
    poolofradiance_buffers[i][poolofradiance_other_buffer][2] =  (color << 8);
    FGcounter ++;
  }
  for (uint8_t i = 0; i < NeoLEDs; i++) {
    neo.fadeto( i,
                rgb2color(
                  (poolofradiance_buffers[i][poolofradiance_other_buffer][0] >> 8) ,
                  (poolofradiance_buffers[i][poolofradiance_other_buffer][1] >> 8) ,
                  (poolofradiance_buffers[i][poolofradiance_other_buffer][2] >> 8) ),
                period);
  }
  strip.setBrightness(brightness);
  neo.wait(period , strip);
  poolofradiance_current_buffer ^= 1;
}

//flash white
uint32_t NeoEffect_BufferedFlash_buffer [NeoLEDs]; //saves what was there before to fade back to it.
void NeoEffect_BufferedFlash (uint32_t flashcolor, int period) {
  for (uint8_t i = 0; i < NeoLEDs; i++) {
    NeoEffect_BufferedFlash_buffer[i] = neo.getcolor(i);
    neo.setcolor_now(i, flashcolor , strip);
  }
  strip.show();
  for (uint8_t i = 0; i < NeoLEDs; i++) {
    neo.fadeto(i, NeoEffect_BufferedFlash_buffer[i], period); // fade back to what was there
  }
  neo.wait(period, strip);
}



uint8_t pacgame_state = 0;
int8_t pacgame_pacpos = 5;
int8_t pacgame_ghostpos = 5;
bool pacgame_ppstate = true; //power pellet flash state

void NeoEffect_pacgame(uint8_t colorsetnum, Colorsets & colorset, int period, uint8_t brightness) {
  strip.setBrightness(brightness);
  //script for pacgame:
  //1: fade in pacman(yellow)(@5) and 5 dots(white)(@6,8,0,2,4)
  //2: move pacman 3 spaces(6,7,8) eating two dots(6,8) along way
  //3: qhost(red) emerges
  //4: pacman(9,0,1,2,3,4) and ghost(5,6,7,8,9,0,1) move forward
  //5: pacman eats power-pellet(4) ghost turnes blue/lightblue flashing
  //6: pacman(3,2,1,0,9,8,7,6,5) and flashing ghost(0,9,8,7,6,5) move in oposite direction
  // That's 22 frames of animation. 220 bytes of table data would be needed.
  int pacperiod = 0;
  switch (pacgame_state) {
    case 0:
      pacgame_pacpos = 5;
      for (uint8_t x = 0; x <= 8; x += 2) {
        neo.fadeto(x, BRIGHT_WHITE, period << 2); //draw dots
      }
      neo.fadeto(pacgame_pacpos, YELLOW, period << 2); //draw pukmon
      neo.wait((period << 1) + period, strip);
      pacgame_state ++;
      break;
    case 1:
      pacgame_movepac(1, pacperiod);
      if (pacgame_pacpos == 8) {
        pacgame_ghostpos = 5;
        pacgame_state ++;
      }
      break;
    case 2:
      pacgame_movepac(1, pacperiod);
      pacgame_moveghost(1, RED, pacperiod);
      if (pacgame_pacpos == 4) {
        pacgame_state ++;
      }
      break;
    case 3:
      pacgame_movepac(-1, pacperiod);
      pacgame_moveghost(-1, BLUE, pacperiod);
      Serial.println(pacgame_ghostpos);
      if (pacgame_ghostpos == 4) {
        pacgame_state ++;
        neo.fadeto(pacgame_ghostpos, BLACK, pacperiod);
      }
      break;
    case 4:
      pacgame_movepac(-1, pacperiod);
      if (pacgame_pacpos == 5) {
        pacgame_state = 0;
      }
      break;
  }
  pacgame_blinkpp(pacperiod);
  neo.wait(period >> 1, strip);
  pacgame_blinkpp(pacperiod);
  neo.wait(period >> 1, strip);
}

void pacgame_blinkpp(int period) {
  pacgame_ppstate = pacgame_ppstate ^ 0x1;
  if (pacgame_state < 3) {
    if (pacgame_ppstate)
      neo.fadeto(4, BRIGHT_WHITE, period);
    else
      neo.fadeto(4, BLACK, period);
  }
}

void pacgame_movepac(int8_t dir, int period) {
  neo.fadeto(pacgame_pacpos, BLACK, period); //erase pukmon
  pacgame_pacpos = (pacgame_pacpos + dir) % NeoLEDs;
  if (pacgame_pacpos < 0) pacgame_pacpos = 9;
  neo.fadeto(pacgame_pacpos, YELLOW, period); //draw pukmon
}

void pacgame_moveghost(int8_t dir, uint32_t color, int period) {
  neo.fadeto(pacgame_ghostpos, BLACK, period); //erase pukmon
  pacgame_ghostpos = (pacgame_ghostpos + dir) % NeoLEDs;
  if (pacgame_ghostpos < 0) pacgame_ghostpos = 9;
  neo.fadeto(pacgame_ghostpos, color, period); //draw pukmon
}


///////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////                    ///////////////////////////////////////
//////////////////////////////////// ir RX & TX callers ///////////////////////////////////////
////////////////////////////////////                    ///////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
#define RXframe_len 5
uint8_t RXframe [RXframe_len];
bool RXframe_full = false;
uint8_t RXframeByte = RXframe_len;
uint32_t RXframe_valid_until = 0;
const uint32_t RXframe_valid_timeout = 5000; //5 second time limit on RXframe full
#define RXframeStartByte 0xFF

uint8_t check_IRRX() {
  while (ir.available()) {
    uint8_t rx = ir.read();
    //Serial.println(rx, HEX);
    if (!RXframe_full) {
      if (rx == RXframeStartByte) {
        RXframeByte = 0;
        //Serial.println(F("RX frame start");
      }
      if (RXframeByte < RXframe_len) {
        RXframe[RXframeByte] = rx;
        RXframeByte ++;
        //Serial.println(RXframeByte);
        if (RXframeByte == RXframe_len) {
          //check for validity
          if (rx == CRC8(RXframe, RXframe_len - 1)) {
            /*
              Serial.print(F("Valid RX:");
              Serial.print(RXframe[1], HEX);
              Serial.print(F(",");
              Serial.print(RXframe[2], HEX);
              Serial.print(F(",");
              Serial.println(RXframe[3], HEX);
            */
            RXframe_full = true;
            RXframe_valid_until = millis() + RXframe_valid_timeout;
            return RXframe[1]; //return command recieved
          }
          //else Serial.println(F("BAD CRC");
        }
      }
      else {
        //Serial.println(F("BadStart");
      }
    }
    else if (millis() > RXframe_valid_until) {
      RXframe_full = false;
      //Serial.println(F("RXframe timed out");
    }
  }
  return 0;
}

#define IRTXcommand_genetics 0x10
#define IRTXcommand_setgenetics 0x27 //arbitrary id # ;-)

#define TXframe_len 4
uint8_t TXframe [TXframe_len];

void send_IRTXgenetics(uint16_t genes) {
  TXframe[0] = RXframeStartByte;
  TXframe[1] = IRTXcommand_genetics;
  TXframe[2] = genes >> 8;
  TXframe[3] = genes & 0xFF;
  for (uint8_t i = 0; i < TXframe_len; i++) {
    while (!ir.write_SPECTER(TXframe[i])) {}
    neo.wait(1, strip); //byte intergap
  }
  while (!ir.write_SPECTER(CRC8(TXframe, TXframe_len))) {}
}

void send_IRTXsetgenetics(uint16_t genes) {
  TXframe[0] = RXframeStartByte;
  TXframe[1] = IRTXcommand_setgenetics;
  TXframe[2] = genes >> 8;
  TXframe[3] = genes & 0xFF;
  for (uint8_t i = 0; i < TXframe_len; i++) {
    while (!ir.write_SPECTER(TXframe[i])) {}
    neo.wait(1, strip); //byte intergap
  }
  while (!ir.write_SPECTER(CRC8(TXframe, TXframe_len))) {}
}

uint32_t diag_valid_until = 0;
// recieve any byte over IR and display on LEDs
void IR_diagnostics( void ) {
  if (ir.rxdatavalid) {
    diag_valid_until = millis() + 1000;
    uint8_t bin  = ir.rxdata;
    for (uint8_t i = 0; i < 8; i++) {
      if (bin & 0x80) {
        strip.setPixelColor(i, 0x0000FF00) ; //red = bits
      }
      else
        strip.setPixelColor(i , 0);
      bin <<= 1;
    }
  }
  if (millis() > diag_valid_until) {
    for (uint8_t i = 0; i < 8; i++) {
      strip.setPixelColor(i , random(16));
    }
    strip.setPixelColor(8, 0);
    strip.setPixelColor(9, 0);

  }
  strip.show();
}


bool dump_eeprom_genes(void ) {
  uint8_t NumGenes = EEPROM.read(eeprom_NumGenes);
  Serial.print(F("EEPROMNumGenes = "));
  Serial.println(NumGenes);
  Serial.println(F("EEPROMGenes:"));
  if (NumGenes < MaxGenes) {
    for (uint8_t i = 0; i < NumGenes; i++) {
      Serial.print(i, HEX);
      Serial.print(F(" : "));
      Serial.println(all_genes[i], HEX);
    }
  }
  uint16_t eeprom_checksum = ( (uint16_t)EEPROM.read(eeprom_CRC16) << 8)
                             | EEPROM.read(eeprom_CRC16 + 1);

  if ( eeprom_checksum != CRC16(all_genes, NumGenes - 1)) {
    Serial.print(F("BAD CRC:"));
    Serial.println( eeprom_checksum, HEX);
    return (false);
  }
  else {
    Serial.print(F("CRC OK:"));
    Serial.println( eeprom_checksum, HEX);
    return (true);
  }
}

uint16_t geneof(uint8_t effect, uint8_t colorset) {
  return ((uint16_t)effect << 8) | colorset;
}


int copy_EEPROM_to_genes() {
  int NumGenes = EEPROM.read(eeprom_NumGenes);
  if (NumGenes < MaxGenes) {
    for (uint16_t i = 0; i < NumGenes; i++) {
      uint16_t addr = (i << 1) + eeprom_genes_start;
      all_genes[i] = geneof(EEPROM.read( addr ), EEPROM.read( addr + 1));
    }
  }
  else {
    NumGenes  = 0;
  }
  return NumGenes;
}


void copy_genes_to_EEPROM(uint16_t all_genes[], uint8_t NumGenes ) {
  if (EEPROM.read(0) != NumGenes) {
    EEPROM.write(0, NumGenes);
  }
  for (uint16_t i = 0; i < NumGenes; i++) {
    uint16_t addr = (i << 1) + eeprom_genes_start;
    if ( EEPROM.read( addr ) !=  (all_genes[i] >> 8) & 0xFF ) {
      EEPROM.write( addr ,  (all_genes[i] >> 8) & 0xFF);
    }
    addr ++;
    if ( EEPROM.read( addr ) !=  all_genes[i] & 0xFF) {
      EEPROM.write( addr , all_genes[i] & 0xFF);
    }
  }
  uint16_t crc16  = CRC16(all_genes, NumGenes - 1);
  EEPROM.write(eeprom_CRC16, crc16 >> 8);
  EEPROM.write(eeprom_CRC16 + 1, crc16);
  Serial.print(F("Write EEPROM CRC16:"));
  Serial.println(crc16, HEX);
}


void fill_genes(uint16_t all_genes[], uint8_t numToCopy, uint8_t colorsetnum) {
  for (uint8_t i = 0; i < numToCopy; i++) {
    all_genes[i] = geneof(i, colorsetnum);
  }
}

void corrupt_gene0() {
  EEPROM.write( eeprom_genes_start , ~EEPROM.read(eeprom_genes_start)); //corrupt genes -- should reset genetable on next boot
  copy_EEPROM_to_genes();
  NeoEffect_BufferedFlash(GREEN, 1000);
}

void do_effect(uint8_t current_effect, uint8_t colorsetnum) {
  switch (current_effect) {
    case 0: NeoEffect_loading (colorsetnum, colorset, 200 ); break;
    case 1: NeoEffect_spider (colorsetnum, colorset, 100, 30 ); break;
    case 2: NeoEffect_spider2(colorsetnum, colorset, 10, 30 );  break;
    case 3: NeoEffect_cylon  (colorsetnum, colorset, 200, 30 );  break;
    case 4: NeoEffect_cylon2 (colorsetnum, colorset, 200, 30 );  break;
    case 5: NeoEffect_zigzag (colorsetnum, colorset, 350, 30 );  break;
    case 6: NeoEffect_infinity(colorsetnum, colorset, 100, 30 );  break;
    case 7: NeoEffect_portal (colorsetnum, colorset, 200, 30 ); break;
    case 8: NeoEffect_portal2 (colorsetnum, colorset, 150, 30 ); break;
    case 9: NeoEffect_smiley (colorsetnum, colorset, 300, 30 ); break;
    case 10: NeoEffect_poolofradiance (colorsetnum, colorset, 100, 150); break;
    case 11: NeoEffect_eyeblink (colorsetnum, colorset, 200, 30); break;
    case 12: NeoEffect_pacmania (colorsetnum, colorset, 40, 30); break;
    case 13: NeoEffect_pacgame  (colorsetnum, colorset, 500, 30); break;
    case 14: NeoEffect_stephen (colorsetnum, colorset, 70 ); break;
  }
}

//TODO:
//rainbow color
//explosion effect
//smiley fix - done
//ir - done
//fix brightnesses

////////////////////////////////////////////////////////////////////////////////
uint8_t colorsetnum = 6;
uint32_t next_TX_millis = 0;

void loop()
{
  if (NumGenes > 0) {
    if (millis() > next_TX_millis) {
      send_IRTXgenetics(all_genes[0]); //gene 0 is the "base" gene  //we could reduce data to save power here
      next_TX_millis = millis() + 1000 + random(1000);
    }
    do_effect( (all_genes[current_gene] >> 8), all_genes[current_gene] & 0xFF);

  }
  else {
    neo.setcolor(0, random(0xFFFFFF)); //random color on led0 == no genes set
  }
  uint8_t rx = check_IRRX();
  if (rx) {
    uint16_t RXgene = ((uint16_t)RXframe[2] << 8) | RXframe[3];
    if (rx == IRTXcommand_genetics) {
      for (uint8_t i = 0; i < NumGenes; i++) { //search for duplicate genes
        if (all_genes[i] == RXgene) {
          RXframe_full = false;
          Serial.print(F("Already have gene:"));
          Serial.println(RXgene, HEX);
          break;

        }
      }
      if (RXframe_full) {
        NeoEffect_BufferedFlash(BRIGHT_WHITE, 1000);
      }
    }
    else if (rx == IRTXcommand_setgenetics) {
      NeoEffect_BufferedFlash(RED, 1000);
      NumGenes = 1;
      all_genes[0] = RXgene;
      Serial.print(F("Base gene set to:"));
      Serial.println(RXgene, HEX);
      copy_genes_to_EEPROM(all_genes, NumGenes);
      current_gene = 0;
    }
  }
  neo.wait(10, strip); //dummy wait to set debuncedButtonState

  if (debouncedButtonHeld) {
    if (debouncedButtonHeld < 1e6 && !RXframe_full) {
      current_gene = (current_gene + 1) % NumGenes;
      Serial.print(F("Current ShowGene = "));
      Serial.print(current_gene);
      Serial.print(F(" : "));
      Serial.println(all_genes[current_gene], HEX);
      if (!NumGenes) {
        fill_genes(all_genes, MaxEffects, colorsetnum);
        NumGenes = MaxEffects;
      }
    }
    // when button held for < 1 second and we have a RXframe waiting then adopt that frame
    else if (debouncedButtonHeld < 1e6 && RXframe_full && RXframe[1] == IRTXcommand_genetics) {
      all_genes[NumGenes] = geneof(RXframe[2], RXframe[3]);
      current_gene = NumGenes; //switch to newly acquired gene
      Serial.print(F("New gene learned = "));
      Serial.println(all_genes[NumGenes], HEX);
      NumGenes ++;
      copy_genes_to_EEPROM( all_genes, NumGenes);
      if (NumGenes > MaxGenes) NumGenes = MaxGenes;
      RXframe_full = false;
    }
    // if button held for 10 to 30 seconds then set master mode.
    else if (debouncedButtonHeld >= 10e6 && debouncedButtonHeld < 30e6) {
      master_mode_loop();
    }
    else if (debouncedButtonHeld >= 30e6 && debouncedButtonHeld < 120e6) {
      corrupt_gene0();
    }
    debouncedButtonHeld = 0; //clear
  }
  //added serialport command check to do_effect
  check_serial_cmd();
}

bool master_active = true;
uint8_t current_effect = 0;
uint32_t next_flash = 0;
void master_mode_loop() {
  Serial.println(F("master mode activated"));
  debouncedButtonHeld = 0; //clear
  while (master_active) {
    do_effect( current_effect, colorsetnum);
    check_IRRX();
    if (next_flash < millis()) {
      NeoEffect_BufferedFlash(BRIGHT_WHITE, 100); //constantly flash to indicate master mode
      next_flash = millis() + 1000;
    }
    if (debouncedButtonHeld) {
      if (debouncedButtonHeld < 1e6) {
        current_effect = (current_effect + 1) % MaxEffects;
        colorsetnum = random(MaxColorsets);
        Serial.print(F("Effect = "));
        Serial.println(current_effect);
      }
      // when button held for 1 to 5 seconds then send new gene
      else if (debouncedButtonHeld >= 1e6 && debouncedButtonHeld < 5e6) {
        uint16_t gene_out = ((uint16_t)current_effect << 8) | (uint16_t)colorsetnum;
        debouncedButtonHeld = 0;
        while (!debouncedButtonHeld) { //keep sending set gene until button is pressed
          send_IRTXsetgenetics( gene_out );
          Serial.print(F("Setgene to:"));
          Serial.println( gene_out, HEX);
          NeoEffect_BufferedFlash(RED, 500); //flash RED to indicate setGene
        }
      }
      // if button held for 10 to 30 seconds then exit master mode.
      else if (debouncedButtonHeld >= 10e6 && debouncedButtonHeld < 30e6) {
        master_active = false;
      }
      else {
        Serial.print(F("button held?="));
        Serial.println(debouncedButtonHeld);
      }
      debouncedButtonHeld = 0; //clear
    }
  }
}

