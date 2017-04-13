#include <stdlib.h>

#include <avr/pgmspace.h>

#include <EEPROM.h>
#include <avr/sleep.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <avr/power.h> // Comment out this line for non-AVR boards (Arduino Due, etc.)
#include "IRSerial-2014.h"

#define NeoPIN 10
#define NeoLEDs 10 //number of addressable LEDs


byte black[3]  = { 0, 0, 0 };
byte white[3]  = { 100, 100, 100 };
byte dimWhite[3] = { 30, 30, 30 };

int pink[3] = { 100, 0, 50 };
byte red[3]    = { 100, 0, 0 };
int orange[3] = { 100, 50, 0};
int yellow[3] = {100,100, 0 };

int greenyellow[3] = { 50, 100, 0 };
int green[3]  = { 0,  100, 0 };
int lime[3] =   { 0, 100, 50 };
int greenblue[3] = { 0, 100, 100};

int teal[3] =   {  0, 50, 100 };
int blue[3]   = {  0,  0, 100 };
int purple[3] = { 50,  0, 100 };
int darkpurple[3] ={100, 0, 100 };

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
    init_g[LEDnum] = (Color >> 16) & 0xFF;
    init_r[LEDnum] = (Color >> 8) & 0xFF;
    init_b[LEDnum] = Color & 0xFF;
  }

  boolean fadeto(uint8_t LEDnum, uint32_t Color, uint16_t duration) {
    LEDnum %= NeoLEDs;
    event_type[LEDnum] = event_fadeto;
    end_g[LEDnum] = (Color >> 16) & 0xFF;
    end_r[LEDnum] = (Color >> 8) & 0xFF;
    end_b[LEDnum] = Color & 0xFF;
    init_r[LEDnum] = current_r[LEDnum];
    init_g[LEDnum] = current_g[LEDnum];
    init_b[LEDnum] = current_b[LEDnum];
    
    diff_g[LEDnum] = end_g[LEDnum] - init_g[LEDnum];
    diff_r[LEDnum] = end_r[LEDnum] - init_r[LEDnum];
    diff_b[LEDnum] = end_b[LEDnum] - init_b[LEDnum];
    /*Serial.print("fadeto: [");
    Serial.print(LEDnum);
    Serial.print("] ");
    Serial.print(init_r[LEDnum]);
    Serial.print(" -> ");
    Serial.println(end_r[LEDnum]);*/
    event_starttime[LEDnum] = millis();  
    event_duration[LEDnum] = duration;
  }

  boolean wait(int waitfor, Adafruit_NeoPixel &strip) {
    waitfor = waitfor>0?waitfor:0;
    uint32_t returnTime = millis() + waitfor;
    uint32_t out_color;
    for (int i = 0; i< NeoLEDs; i++) {
      //Serial.println(event_type[i] == event_setcolor);
      switch (event_type[i]) {
        case event_setcolor :
          event_type[i] = 0;
          out_color = (((uint32_t) init_g[i])<<16) + (((uint16_t) init_r[i])<<8) + init_b[i];
          strip.setPixelColor(i, out_color) ;
          break;
        
        case event_fadeto :
          event_type[i] = -event_fadeto;
          //out_color = (((uint32_t) init_g[i])<<16) + (((uint16_t) init_r[i])<<8) + init_b[i];
          //strip.setPixelColor(i, out_color) ; // this debug should not be necessary
          break;
      }
    }
    strip.show();
    unsigned long waitstart = millis();

    while (millis() < returnTime) {
       for (int i = 0; i< NeoLEDs; i++) {
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
              strip.setPixelColor(i, strip.Color(current_g[i],current_r[i],current_b[i]));
              /*
              Serial.print(i);
              Serial.print(": ");
              Serial.print(current_r[i],HEX);
              Serial.print(", ");
              Serial.print(current_g[i],HEX);
              Serial.print(", ");
              Serial.println(current_b[i],HEX);
              delay(10); //slow down serial output
              */
              break;              
          }
       }
       strip.show();
    }
  }
};




//-------------------------------------------------------------------------------------------------

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NeoLEDs, NeoPIN, NEO_GRB + NEO_KHZ800);

#define PURPLE  0
#define YELLOW  1
#define BLUE  2
#define WHITE  4

uint8_t ColorSets[][4] = {
  {232, 100, 255},   // purple
  {200, 200, 20},   // yellow
  {30, 200, 200},   // blue
  {64, 64, 64}   //white
};
#define FAVCOLORS sizeof(ColorSets) / 3

#define BUTTON_PIN 5
int button = BUTTON_PIN;
int photopot = A1;
int colorModeMax = 5;
int colorMode = 0;

//for Backlight_patch(BACKLIGHT_X,HI/LO)
#define BACKLIGHT_1 1
#define BACKLIGHT_2 2
#define BACKLIGHT_3 3
#define BACKLIGHT_4 4


/*
   Pinout:
   0, 1: FTDI serial header
   2, 4, 7: USB D+, D-, Pullup (respectively)
   8, 9: IR Rx, Tx (respectively)
   3: Morse LED
   5, 6, 10, 11: Backlight LEDs
   12: Button
   16: Display board UP button
   23: Display board RIGHT button
   24: Display board CENTER (ENTER) button
   25: Display Board DOWN button
   26: Display Board LEFT button
   27: SDA
   28: SCL
*/
#define MAKE_ME_A_CONTEST_RUNNER 0
// IR Parameters
#define IR_RX 8 //was 8
#define IR_TX 9 //was 9
#define IR_BAUD 300
IRSerial irSerial(IR_RX, IR_TX, false, true);

#define SERIAL_BAUD 115200

// Main morse LED
#define LED_PIN 3



//BEGIN SERIAL EPIC VARS -darknetstuff
//bool to see if we have started the SerialEpic
#define MAX_SERIAL_EPIC_TIME_MS 30000
#define MAX_SERIAL_ANSWER_LENGTH 10
//END SERIAL EPIC VARS

// BEGIN STRINGS
//string that starts serial epic
static const char* START_SERIAL_EPIC_STRING = "JOSHUA";
//static const char* SERIAL_PORT_ANSWER = "42";
#define SERIAL_PORT_QUESTION F("What is the answer to all things?")
//length of serial string
#define MIN_SERIAL_LEN  6
#define SEND_TO_DAEMON F("Send this to daemon: ")
#define DisplayEpicText F("Who started the \nDC Dark Net?")
static const char* DisplayEpicAnswer = "SMITTY";
#define iKnowNot F("I know not what you speak of.")
#define SPECTER_1337 F("Find specter.  Bring techno.")
#define sendTheCodes F("https://dcdark.net/  Send the following codes:\n")
static const char* const PROGMEM LINE1 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789!*#@%";
#define LINE1_LENGTH 42
#define EMPTY_STR F("")
#define DKN F("DKN-")
// END REPEATED STRINGS

//BEGIN GLOBAL VARS:
//We are getting tight on space so I'm packing variables as tightly as I can
struct _PackedVars {
  /*  uint32_t InSerialEpic : 1;
    uint32_t AwaitingSerialAnswer: 1;
    uint32_t DisplayAnswer:1;
    uint32_t LEDMode : 4; //what start up mode are we need (animation)
    uint32_t Silent : 1;
    uint32_t LINE1Loc : 6;
    uint32_t AnswerPos : 4;
    uint32_t hasSilk1 : 1;
    uint32_t hasSilk2 : 1;
    uint32_t hasSilk3 : 1;
    uint32_t isContestRunner : 1;
  */
  uint32_t AwaitingSerialAnswer: 1;
  uint32_t DisplayAnswer: 1;
  uint32_t LEDMode : 4; //what start up mode are we need (animation)
  uint32_t swapKeyAndGUID : 1;
  uint32_t modeInit : 1;
} PackedVars;

uint16_t KEY[4];
char GUID[9] = "201005xx";
unsigned long nextBeacon;
#define OLED_RESET 4
//DarkNetDisplay Display(OLED_RESET);
//END GLOBAL VARS


//BEGIN MODE_DEFS
#define MODE_COUNT 9
#define MODE_MORSE_CODE_EPIC     0
#define MODE_SNORING             1
#define MODE_SERIAL_EPIC         2
#define MODE_DISPLAY_BOARD_EPIC  3
#define MODE_JUST_A_COOL_DISPLAY   4
#define MODE_SILK_SCREEN           5
#define MODE_UBER_BADGE_SYNC       6
#define MODE_BACKLIGHT             7
#define MODE_SHUTDOWN              8
//END MODE DEFS


// BEGIN EEPROM count location
#define MSG_COUNT_ADDR 1022
#define RESET_STATE_ADDR 1020
#define GUID_ADDR 1012
#define KEY_ADDR 1004

// Maximum number of messages
#define MAX_NUM_MSGS 60
#define GUID_SIZE 8
#define MORSE_CODE_ENCODED_MSG 8
#define TOTAL_STORAGE_SIZE_MSG (GUID_SIZE+MORSE_CODE_ENCODED_MSG)
#define MAX_MSG_ADDR                     (TOTAL_STORAGE_SIZE_MSG*MAX_NUM_MSGS) //960
#define START_EPIC_RUN_TIME_STORAGE       MAX_MSG_ADDR+20 //cushion 980
#define UBER_CRYTPO_CONTEST_RUNNER_ADDR   START_EPIC_RUN_TIME_STORAGE //first 8 bytes uber contest runner KEY then GUID
#define MAX_EPIC_RUN_TIME_STORAGE         START_EPIC_RUN_TIME_STORAGE+16

// END EEPROM COUNT LOCATION

#define ENDLINE F("\n")

//experimental
uint32_t PUF_hash()
{
  uint8_t const * p;
  uint8_t i;
  uint32_t hash = 0;
  uint8_t hash0 = 0;
  uint8_t hash1 = 0;
  uint8_t hash2 = 0;
  uint8_t hash3 = 0;

  p = (const uint8_t *)(8192 - 256);
  i = 0;
  do
  {
    hash0 ^= *p;
    p ++;
    hash1 ^= *p;
    p ++;
    hash2 ^= *p;
    p ++;
    hash3 ^= *p;
    p ++;
    i ++;
  }
  while ( i != 0 );

  return ((hash3 << 24) + (hash2 << 16) + (hash1 << 8) + hash0);
} uint8_t hash0 = 0;



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


void readInGUIDAndKey() {
  for (char ndx = 0; ndx < 8; ndx++) {
    GUID[ndx] = EEPROM.read(GUID_ADDR + ndx);
  }
  GUID[8] = 0;
  // Yes, this is big endian. I don't want to have to byte-swap
  // when building the EEPROM file from text strings.
  KEY[0] = (EEPROM.read(KEY_ADDR + 0) << 8) + EEPROM.read(KEY_ADDR + 1);
  KEY[1] = (EEPROM.read(KEY_ADDR + 2) << 8) + EEPROM.read(KEY_ADDR + 3);
  KEY[2] = (EEPROM.read(KEY_ADDR + 4) << 8) + EEPROM.read(KEY_ADDR + 5);
  KEY[3] = (EEPROM.read(KEY_ADDR + 6) << 8) + EEPROM.read(KEY_ADDR + 7);
}

/* This is the workhorse function.  Whatever you do elsewhere,
   When you're not working you need to call this so it looks
   for IR data and acts on it if received.  The Rx ISR has a
   64 byte buffer, so you need to call this at least once every
   (1 sec/300 symbols * 10 symbos/1 byte * 64 byte buffer = )
   2.13 seconds before you risk overflowing the buffer, but I'd
   make sure you call it more often than that.  Basically,
   whenever you are pausing, use this instead of delay().

   Like delay(), but looks for IR serial data while waiting.
   Delays for pauseFor milliseconds.  Tight koops around looking
   for serial data.  Also triggers a USB dump if the button is
   pressed.

   IMPORTANT: If either a full Serial frame is received, or the
   button is pressed, then delayAndReadIR() can delay for much
   longer than pauseFor milliseconds as it either handles the USB,
   or handles a received IR signal.
*/

// Sends our GUID out over the IR.  Call this "often"
// when it's convenient in your animation. It'll handle
// dealing with the timing, whether it's been long enough
// to send another beacon.
void beaconGUID(int timegap) {
  if (millis() >= nextBeacon) {
    /*if (1 == PackedVars.swapKeyAndGUID) {
      readContestRunnerKeyAndGUID();
      }*/
    // Add a little randomness so devices don't get sync'd up.
    nextBeacon = millis() + timegap;
    uint8_t r=random(64);
    uint8_t g=random(64);
    uint8_t b=random(64);
    char str[9];
    sprintf(str,"%02x",r);
    sprintf(str+2,"%02x",g);
    sprintf(str+4,"%02x",b);
    sprintf(str+6,"xx");
    strip.setPixelColor(0, strip.Color(r,g,b));
    strip.show();


    irSerial.print(F("0x"));
    irSerial.println(str);
    Serial.print("\n");
    Serial.print("Beacon:");
    Serial.println(str);
  }
}
// TEA (known to be broken) with all word sizes cut in half (64 bit key, 32 bit blocks)
// Yes, I'm inviting people to hack this if they want. :-)
// TODO: Since we're giving up on backward compatability in 2014, should we increase this size?
void encrypt(uint16_t *v) {
  uint16_t v0 = v[0], v1 = v[1], sum = 0, i;     // set up
  uint16_t delta = 0x9e37;                       // a key schedule constant
  for (i = 0; i < 32; i++) {                     // basic cycle start
    sum += delta;
    v0 += ((v1 << 4) + KEY[0]) ^ (v1 + sum) ^ ((v1 >> 5) + KEY[1]);
    v1 += ((v0 << 4) + KEY[2]) ^ (v0 + sum) ^ ((v0 >> 5) + KEY[3]);
  }                                              /* end cycle */
  v[0] = v0; v[1] = v1;
}

uint16_t getNumMsgs() {
  return (EEPROM.read(MSG_COUNT_ADDR) << 8) + EEPROM.read(MSG_COUNT_ADDR + 1);
}

void writeNumMsgs(uint16_t numMsgs) {
  EEPROM.write(MSG_COUNT_ADDR, numMsgs / 256);
  EEPROM.write(MSG_COUNT_ADDR + 1, numMsgs % 256);
}

bool isNumMsgsValid(uint16_t numMsgs) {
  return numMsgs < MAX_NUM_MSGS;
}

// Writes a pairing code to EEPROM
int writeEEPROM(unsigned char *guid, uint8_t *msg) {
  char msgStr[9];
  intToStr(msg, msgStr);

  uint16_t numMsgs = getNumMsgs();
  if (!isNumMsgsValid(numMsgs)) {
    Serial.print(F("numMsgs not valid. Setting to 0. Ignore if this is your first pairing."));
    Serial.println(numMsgs);
    // Assume this is the first read of this chip and initialize
    numMsgs = 0;
  }

  int msgAddr;
  unsigned char ndx;
  for (msgAddr = 0; msgAddr < numMsgs * TOTAL_STORAGE_SIZE_MSG; msgAddr += TOTAL_STORAGE_SIZE_MSG) {
    for (ndx = 0; ndx < 8; ndx++) {
      if (guid[ndx] != EEPROM.read(msgAddr + ndx))
        break;
    }
    if (ndx == 8) {
      // Found a match.  Rewrite in case it was wrong before
      break;
    }
  }

  if (ndx != 8) {
    // FIXME This doens't properly stop at 60. It wraps around.
    if (numMsgs > 0 && !isNumMsgsValid(numMsgs)) {
      // The DB is full.  Don't actually create a new entry.
      for (ndx = 0; ndx < 16; ndx++) {
        // Randy Quaid is my hero.
        EEPROM.write(msgAddr + ndx, "SHITTERS FULL..."[ndx]);
      }
      return msgAddr;
    }
    else {
      numMsgs++;
    }
  }

  for (ndx = 0; ndx < 8; ndx++) {
    EEPROM.write(msgAddr + ndx, guid[ndx]);
    EEPROM.write(msgAddr + 8 + ndx, msgStr[ndx]);
  }
  writeNumMsgs(numMsgs);
  return msgAddr;
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


/*
  int weAreAlice() {
  backlight_patched(BACKLIGHT_1, HIGH);
  // Read the 0y from Bob and process it.
  uint8_t aliceEnc[4] = {0, 0, 0, 0};
  uint8_t bob[4] = {0, 0, 0, 0};
  unsigned char bobGUID[8];
  if (!readWordFromBuf(aliceEnc)) {
    Serial.println(F("Error reading 0y from rxBuf."));
    return -1;
  }
  backlight_patched(BACKLIGHT_2, HIGH);

  // Right on the heels is 0a, Bob's GUID
  if (!readWordFromIR() || rxBufNdx(-11) != 'a') {
    Serial.println(F("Error reading 0a"));
    return -1;
  }
  backlight_patched(BACKLIGHT_3, HIGH);

  if (!readWordFromBuf(bob, bobGUID)) {
    Serial.println(F("Error reading 0a from rxBuf"));
    return -1;
  }

  uint8_t bobEnc[4];
  // Nasty pointer math to convert to the right format for encryption.
  // TODO this could almost certainly be done cleaner.
   (uint32_t*)bobEnc = *(uint32_t*)bob;
  encrypt((uint16_t*)bobEnc);
  delay(100);  // Give Bob some time to recover from his send
  irSerial.print(F("0b"));
  writeWord(bobEnc);
  backlight_patched(BACKLIGHT_4, HIGH);

  // Alright!  We've got everything we need!  Build a message
   (uint32_t*)bobEnc ^= *(uint32_t*)aliceEnc;
  int retVal =  writeEEPROM(bobGUID, bobEnc);

  if (1 == PackedVars.swapKeyAndGUID) {
    PackedVars.swapKeyAndGUID = 0;
    readInGUIDAndKey();
  }
  return retVal;
  }

  int weAreBob() {
  // Read the 0x from Alice and process it.
  backlight_patched(BACKLIGHT_4, HIGH);
  uint8_t alice[4] = {0, 0, 0, 0};
  unsigned char aliceGUID[8];
  if (!readWordFromBuf(alice, aliceGUID)) {
    Serial.println(F("Error reading 0x from rxBuf."));
    return -1;
  }
  backlight_patched(BACKLIGHT_3, HIGH);

  uint8_t aliceEnc[4];
   (uint32_t*)aliceEnc = *(uint32_t*)alice;
  encrypt((uint16_t*)aliceEnc);

  // Transmit response, plus my GUID
  delay(100);  // Sleep a little bit to let the other side clear out their buffer. ??
  irSerial.print(F("0y"));
  writeWord(aliceEnc);
  delay(100);  // Give the other side a bit of time to process.
  irSerial.print(F("0a"));
  irSerial.println(GUID);

  // Look for a response from Alice
  if (!readWordFromIR() || rxBufNdx(-11) != 'b') {
    Serial.println(F("Error reading 0b"));
    return -1;
  }
  backlight_patched(BACKLIGHT_2, HIGH);

  uint8_t bobEnc[4] = {0, 0, 0, 0};
  if (!readWordFromBuf(bobEnc)) {
    Serial.println(F("Error reading 0b from rxBuf."));
    return -1;
  }
  backlight_patched(BACKLIGHT_1, HIGH);

  // Alright!  We've got everything we need!  Build a message
   (uint32_t*)bobEnc ^= *(uint32_t*)aliceEnc;
  return writeEEPROM(aliceGUID, bobEnc);
  }
*/
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
    uint8_t r =  char2tob(rxBufNdx(-10),rxBufNdx(-9));
    uint8_t g =  char2tob(rxBufNdx(- 8),rxBufNdx(-7));
    uint8_t b =  char2tob(rxBufNdx(- 6),rxBufNdx(-5));

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
   return((convertchartobyte(hexH)<<4) + convertchartobyte(hexL));
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
    return(cur);
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


void setup() {
  uint32_t  PUFhash_result = PUF_hash();
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
  delay(200);  // Reset "debounce"

  // Setup the IR buffers and timers.
  nextBeacon = millis();
  clearRxBuf();

  PackedVars.swapKeyAndGUID = 0;
  PackedVars.modeInit = 0;

  // BLINKY SHINY!
  PackedVars.LEDMode = EEPROM.read(RESET_STATE_ADDR) % MODE_COUNT;
  EEPROM.write(RESET_STATE_ADDR, (PackedVars.LEDMode + 1) % MODE_COUNT);
  digitalWrite(13, LOW);
  
  //NeoPixel init
  pinMode(button, INPUT);
  pinMode(photopot, INPUT);
  strip.begin();
  strip.setBrightness(255);
  strip.setPixelColor(0, strip.Color(0x3F,0,0));  //NOTE ORDER!: GRB
  
  strip.show(); // Initialize all pixels to 'off'
  Serial.println("Starting kernel loop");
  strip.setPixelColor(0, 0x00000000);
  strip.show();
}

  uint32_t cylon_BGcolor = 0x000000; //((uint32_t)r << 16) | ((uint32_t)g << 8) |
  uint32_t cylon_BGcolor2;
  uint32_t cylon_FGcolor; //RED 25%;
  uint32_t cylon_FGcolor2;
  uint8_t last_cylon = 0;
  uint8_t cylon_current_LEDnum = 4;
  boolean cylon_dir = true;
  //fix negative elspased time
  
  const uint8_t center1 = 4;
  const uint8_t center2 = 5;


  uint32_t magenta = strip.Color(0, 0x3F, 0x3F);//GRB
/*
void regularCylon() {
    //joel this is a debug line a may be ignored irSerial.write_SPECTER(0xF0);
    const  uint32_t cylon_BGcolor = 0x000000; //((uint32_t)r << 16) | ((uint32_t)g << 8) |
    uint32_t cylon_FGcolor; //RED 25%;
    uint8_t last_cylon = 0;
    uint8_t cylon_current_LEDnum = 0;
    boolean cylon_dir = true;
    //fix negative elspased time
    
    cylon_FGcolor = (uint32_t)(0x40<<8); // GRB
    
    neo.fadeto(cylon_current_LEDnum, cylon_FGcolor, 0);  //lednum, Color, FadeTime
    
    neo.fadeto(last_cylon, cylon_BGcolor, 500);
    
    
    neo.wait(100, strip);
    
    
    
    last_cylon = cylon_current_LEDnum;
    if (cylon_dir) {
      cylon_current_LEDnum ++;
      if (cylon_current_LEDnum >= (NeoLEDs - 1)) {
        cylon_current_LEDnum = NeoLEDs - 1;
        cylon_dir = false;
      }
    }
    else  {
      cylon_current_LEDnum --;
      if (cylon_current_LEDnum <=0) {
        cylon_current_LEDnum = 0;
        cylon_dir = true;
      }
    }
  
//was  static uint8_t count = 0;
//was  delayAndReadIRSpecter(1000, ne);
//was  beaconGUID(1000 + random(100)); //GUID gap
  
}
*/
/////////////////////////////////////////////////////////////
void circularCylon()
{

    cylon_FGcolor = magenta; // GRB     
    cylon_FGcolor2 = strip.Color(0x3F,0,0x3F); // GRB 

    cylon_BGcolor = strip.Color(0x3F,0,0x3F);
    cylon_BGcolor2 = strip.Color(0x3F,0x3F,0);
    
    neo.fadeto(cylon_current_LEDnum, cylon_FGcolor, 0);  //lednum, Color, FadeTime
    neo.fadeto(NeoLEDs - cylon_current_LEDnum - 1, cylon_FGcolor2, 0);  //lednum, Color, FadeTime

    neo.fadeto(last_cylon, cylon_BGcolor, 200);
    neo.fadeto(NeoLEDs - last_cylon - 1, cylon_BGcolor2, 200);

    neo.wait(100, strip);
    
    
    //bounce code
    last_cylon = cylon_current_LEDnum;
    if (cylon_dir) {
      cylon_current_LEDnum ++;
      if (cylon_current_LEDnum > (center1)) {
        cylon_current_LEDnum = center1;
        cylon_dir = false;
      }
    }
    else  {
      cylon_current_LEDnum --;
      if (cylon_current_LEDnum <=0) {
        cylon_current_LEDnum = 0;
        cylon_dir = true;
      }
    }

}
/////////////////////////////////////////////////////////////

void loop()
{
  circularCylon();
}



