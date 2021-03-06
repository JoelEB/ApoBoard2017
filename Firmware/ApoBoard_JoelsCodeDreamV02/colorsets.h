/*
 * 
 * 
 */

#define BRIGHT_WHITE      0xFFFFFF
#define DIM_WHITE         0x555555

/*
 * G..R...B
 */

#define RED               0x00FF00
#define ORANGE            0x7FFF00
#define YELLOW            0xFFFF00
#define LIME              0xFF7F00
#define GREEN             0xFF0000
#define CYAN              0xFF00FF
#define BLUE              0x0000FF
#define PURPLE            0x007FFF
#define MAGENTA           0x00FFFF  

//Light colors
#define L_RED             0x007F00
#define L_ORANGE          0x3F7F00
#define L_YELLOW          0x7F7F00
#define L_LIME            0x7F3F00
#define L_GREEN           0x7F0000
#define L_CYAN            0x7F007F
#define L_BLUE            0x00007F
#define L_PURPLE          0x003F7F
#define L_MAGENTA         0x007F7F 


uint8_t BGcounter = 0; //global for Background color counter
uint8_t FGcounter = 0; //global for Foreground color counter

#define NumColorSets 14

/*
 * Good RGB Websites
 * http://arcbotics.com/lessons/mixing-colors-with-the-rgb-led-2/
 * http://www.rapidtables.com/web/color/RGB_Color.htm
 */
class Colorsets{
  public:
  uint32_t colorarray [NumColorSets] [8] = {
    /*
    {0x00FF00,0x00FF00,0x00FF00,0x00FF00,     0x000200,0x000200,0x000200,0x000200}, //RED only
    */
    {0,0,0,0,     0,0,0,0}, // MUST BE HERE FOR FADE OFF\
    
    //Complimentary sets
    {RED,BLUE,CYAN,YELLOW,    L_CYAN,L_YELLOW,L_RED,L_BLUE},
    {RED,CYAN,GREEN,MAGENTA,    L_GREEN,L_MAGENTA,L_RED,L_CYAN},
    {YELLOW,GREEN,BLUE,MAGENTA,    L_BLUE,L_MAGENTA,L_YELLOW,L_GREEN},

    //fire and ice
    {RED, 0x65FF00, ORANGE, YELLOW,           0x8F00FF, CYAN, 0x0800FF, BLUE},
    {0x8F00FF, CYAN, 0x0800FF, BLUE,          RED, 0x65FF00, ORANGE, YELLOW},

    //rainbow
    {RED,YELLOW,CYAN,PURPLE,     ORANGE,GREEN,BLUE,MAGENTA},
    //reverse rainbow
    {ORANGE,GREEN,BLUE,MAGENTA,   RED,YELLOW,CYAN,PURPLE},
    //Spring 
    {LIME,CYAN,PURPLE,MAGENTA,    L_PURPLE,L_MAGENTA,L_LIME,L_CYAN},
    //Princess 
    {0x0077FF,PURPLE,0x009C7C,MAGENTA,    L_MAGENTA,0x002020,L_PURPLE,0x002020},
    //Broncos
    {ORANGE,0x55EE00,0x33CC00,0x22AA00,    BLUE,0x0000B0,0x000090,0x660066},
    
    {0xFF3377,0x119933,0x220044,0x880044,     0x110022,0x440022,0x771133,0x004411}, //coder colorz
    {0xFF3377,0x119933,0x220044,0x880044,     0x080011,0x003311,0x110811,0x002208}, //coder colorz
    {0x38761D,0x351C75,0xE69138,0xC27BA0,     0x110022,0x440022,0x771133,0x004411}

    
  };
  
  uint32_t getFG(uint8_t colorsetnum, int8_t FGdelta) {
    FGcounter += FGdelta;
    return (colorarray[colorsetnum] [FGcounter % 4]);
  }
  uint32_t getBG(uint8_t colorsetnum, int8_t BGdelta) {
    BGcounter += BGdelta;
    return (colorarray[colorsetnum] [4 + BGcounter % 4]);
  }
};

void dim(uint32_t color)
{
    
}
