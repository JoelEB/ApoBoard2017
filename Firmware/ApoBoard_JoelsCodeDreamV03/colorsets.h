#define BRIGHT_WHITE      0xFFFFFF
#define DIM_WHITE         0x555555
#define BLACK             0x000000

/*
   G..R...B
*/
#define GREEN             0xFF0000
#define RED               0x00FF00
#define BLUE              0x0000FF
#define L_GREEN             0x770000
#define L_RED               0x007700
#define L_BLUE              0x000077
#define YELLOW            0xFFFF00
#define ORANGE            0x77FF00
#define TEST              0xFF7700
#define CYAN              0xFF00FF
#define DARK_CYAN         0x8B008B
#define PURPLE            0x0099FF
#define MAGENTA       0x00FFFF
#define AQUA              0xFF7FD4
#define AERO_BLUE         0xFFC9E5
#define BLUE_GREEN        0x980DBA
#define CARIBBEAN_CURRENT 0x6D006F
#define CERULAN           0x7B00A7

#define MaxColorsets 8
PROGMEM const uint32_t colorarray [MaxColorsets] [8] = { //memopt
  {0xFF0000, 0x00FF00, 0x0000FF, 0x00FFFF,     0, 0, 0, 0}, //BG is used as BLACK in several effects!
  {0xFF3377, 0x119933, 0x220044, 0x880044,     0x110022, 0x440022, 0x771133, 0x004411}, //coder colorz
  {0xFF3377, 0x119933, 0x220044, 0x880044,     0x080011, 0x003311, 0x110811, 0x002208}, //coder colorz
  {0x00FF00, 0x00FF00, 0x00FF00, 0x00FF00,     0x000200, 0x000200, 0x000200, 0x000200}, //RED only
  {0x38761D, 0x351C75, 0xE69138, 0xC27BA0,     0x110022, 0x440022, 0x771133, 0x004411},
  {YELLOW, RED, YELLOW, RED,                   RED, YELLOW, RED, YELLOW},
  {TEST, L_RED, YELLOW, ORANGE,                L_BLUE, L_BLUE, L_BLUE, L_BLUE},
  {PURPLE, ORANGE, YELLOW, GREEN,              AQUA, CERULAN, BLUE_GREEN, CARIBBEAN_CURRENT}
};


uint8_t BGcounter = 0; //global for Background color counter
uint8_t FGcounter = 0; //global for Foreground color counter
/*
  uint8_t ColorSets[][4] = {
  {232, 100, 255},   // purple
  {200, 200, 20},   // yellow
  {30, 200, 200},   // blue
  {64, 64, 64}   //white
  };
  #define FAVCOLORS sizeof(ColorSets) / 3
*/


class Colorsets {
  public:

    uint32_t getFG(uint8_t colorsetnum) {
      return pgm_read_dword_near( &( colorarray [colorsetnum] [FGcounter % 4] ) );
      // was return (colorarray[colorsetnum] [FGcounter % 4]);
    }

    uint32_t getFG(uint8_t colorsetnum, uint8_t offset) {
      return pgm_read_dword_near( &( colorarray [colorsetnum] [(FGcounter + offset) % 4] ) );
      // was return (colorarray[colorsetnum] [FGcounter % 4]);
    }

    uint32_t getBG(uint8_t colorsetnum) {
      return  pgm_read_dword_near( &( colorarray [colorsetnum] [BGcounter % 4 + 4] ) );
      //was return (colorarray[colorsetnum] [4 + BGcounter % 4]);
    }

    uint32_t getBG(uint8_t colorsetnum, uint8_t offset) {
      return  pgm_read_dword_near( &( colorarray [colorsetnum] [(BGcounter + offset) % 4 + 4] ) );
      //was return (colorarray[colorsetnum] [4 + BGcounter % 4]);
    }
};


//uint32_t magenta = strip.Color(0, 0x3F, 0x3F);//GRB


int pink[3] = { 100, 0, 50 };


int greenyellow[3] = { 50, 100, 0 };
int green[3]  = { 0,  100, 0 };
int lime[3] =   { 0, 100, 50 };
int greenblue[3] = { 0, 100, 100};
/*


*/


