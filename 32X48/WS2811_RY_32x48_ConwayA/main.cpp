#include "mbed.h"
#include "matrix.h"
Serial pc(USBTX, USBRX);
DigitalOut dat(p5);
DigitalOut bLed(LED1);

//*******************************************************************************    
// DEFINE WS2811 Strip Parameters
#define numLEDs 1536 
#define BrightMax 64
#define BrightMin 0

#define ROWS_LEDs 32  // LED_LAYOUT assumed 0 if ROWS_LEDs > 8
#define COLS_LEDs 48  // all of the following params need to be adjusted for screen size


uint8_t RandRed;
uint8_t RandGrn;
uint8_t RandBlu;

uint8_t pixels[numLEDs*3];
Timer guardtime;
uint32_t bogocal;
//*******************************************************************************
//Byte val 2PI Cosine Wave, offset by 1 PI 
//supports fast trig calcs and smooth LED fading/pulsing.
uint8_t cos_wave[256] =  
{0,0,0,0,1,1,1,2,2,3,4,5,6,6,8,9,10,11,12,14,15,17,18,20,22,23,25,27,29,31,33,35,38,40,42,
45,47,49,52,54,57,60,62,65,68,71,73,76,79,82,85,88,91,94,97,100,103,106,109,113,116,119,
122,125,128,131,135,138,141,144,147,150,153,156,159,162,165,168,171,174,177,180,183,186,
189,191,194,197,199,202,204,207,209,212,214,216,218,221,223,225,227,229,231,232,234,236,
238,239,241,242,243,245,246,247,248,249,250,251,252,252,253,253,254,254,255,255,255,255,
255,255,255,255,254,254,253,253,252,252,251,250,249,248,247,246,245,243,242,241,239,238,
236,234,232,231,229,227,225,223,221,218,216,214,212,209,207,204,202,199,197,194,191,189,
186,183,180,177,174,171,168,165,162,159,156,153,150,147,144,141,138,135,131,128,125,122,
119,116,113,109,106,103,100,97,94,91,88,85,82,79,76,73,71,68,65,62,60,57,54,52,49,47,45,
42,40,38,35,33,31,29,27,25,23,22,20,18,17,15,14,12,11,10,9,8,6,6,5,4,3,2,2,1,1,1,0,0,0,0
};


//Gamma Correction Curve
uint8_t exp_gamma[256] =
{0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,3,3,3,3,3,
4,4,4,4,4,5,5,5,5,5,6,6,6,7,7,7,7,8,8,8,9,9,9,10,10,10,11,11,12,12,12,13,13,14,14,14,15,15,
16,16,17,17,18,18,19,19,20,20,21,21,22,23,23,24,24,25,26,26,27,28,28,29,30,30,31,32,32,33,
34,35,35,36,37,38,39,39,40,41,42,43,44,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,
61,62,63,64,65,66,67,68,70,71,72,73,74,75,77,78,79,80,82,83,84,85,87,89,91,92,93,95,96,98,
99,100,101,102,105,106,108,109,111,112,114,115,117,118,120,121,123,125,126,128,130,131,133,
135,136,138,140,142,143,145,147,149,151,152,154,156,158,160,162,164,165,167,169,171,173,175,
177,179,181,183,185,187,190,192,194,196,198,200,202,204,207,209,211,213,216,218,220,222,225,
227,229,232,234,236,239,241,244,246,249,251,253,254,255
};
//*******************************************************************************
void setup () {
    /*if ((pixels = (uint8_t *)malloc(numLEDs * 3))) {
        memset(pixels, 0x00, numLEDs * 3); // Init to RGB 'off' state
    }*/
    
    for (int j=0; j<numLEDs*3; j++) {
        pixels[j]=0x00;
    }
    
    // calibrate delay loops for NRZ 
    int i;
    guardtime.start();
    for (i=0; i<1000; i++)
        /* do nothing */;
    i=guardtime.read_us();
    printf("ws2811:  1000 iters took %d usec.\n", i);
    bogocal = (1000 / (i*10)); // iterations per bitcell (417 nsec)
    bogocal = 1;
    printf("ws2811:  calibrating to %d bogojiffies.\n", bogocal);
}
//*******************************************************************************
inline void celldelay(void) {
    for (int i = 0; i<bogocal; i++)
        /* do nothing */ ;
}
//*******************************************************************************
void writebit(bool bit) {
    // first cell is always 1
    dat = 1;
    celldelay();
    if (bit) {
        celldelay();
    } else {
        dat=0;
        celldelay();
    }
    // last cell is always 0
    dat=0;
    celldelay();
}
//*******************************************************************************
void write(uint8_t byte) {
    __disable_irq();
    for (int i=0; i<8; i++) {
        if (byte & 0x80)
            writebit(1);
        else
            writebit(0);
        byte <<= 1;
    }
    __enable_irq();
}

uint16_t numPixels(void) {
    return numLEDs;
}

//*******************************************************************************
void show(void) {
    uint16_t i, nl3 = numLEDs * 3; // 3 bytes per LED
    while (guardtime.read_us() < 50)
        /* spin */;
    for (i=0; i<nl3; i++ ) {
        write(pixels[i]);
    }
    guardtime.reset();
}
//*******************************************************************************
void blank(void) {
    for (int i=0; i<numLEDs*3; i++) {
        pixels[i]=0x00;
    }
    show();
}
//*******************************************************************************
void blankDelay(int n) {
    for (int i=0; i<numLEDs*3; i++) {
        pixels[i]=0x00;
    }
    show();
    wait_ms(n);
}
//*******************************************************************************
uint32_t total_luminance(void) {
    uint32_t running_total;
    running_total = 0;
    for (int i=0; i<numLEDs*3; i++)
        running_total += pixels[i];
    return running_total;
}
//*******************************************************************************
// Convert R,G,B to combined 32-bit color
uint32_t Color(uint8_t r, uint8_t g, uint8_t b) {
    // Take the lowest 7 bits of each value and append them end to end
    // We have the top bit set high (its a 'parity-like' bit in the protocol
    // and must be set!)
    return ((uint32_t)g << 16) | ((uint32_t)r << 8) | (uint32_t)b;
}
//*******************************************************************************
// store the rgb component in our array
void setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b) {
    if (n >= numLEDs) return; // '>=' because arrays are 0-indexed
    pixels[n*3  ] = g;
    pixels[n*3+1] = r;
    pixels[n*3+2] = b;
    //pc.printf("setPixelColor-4 inputs\n");   
}
//*******************************************************************************
void setPixelR(uint16_t n, uint8_t r) {
    if (n >= numLEDs) return; // '>=' because arrays are 0-indexed
    pixels[n*3+1] = r;
}
//*******************************************************************************
void setPixelG(uint16_t n, uint8_t g) {
    if (n >= numLEDs) return; // '>=' because arrays are 0-indexed
    pixels[n*3] = g;
}
//*******************************************************************************
void setPixelB(uint16_t n, uint8_t b) {
    if (n >= numLEDs) return; // '>=' because arrays are 0-indexed
    pixels[n*3+2] = b;
}
//*******************************************************************************
void setPixelColor(uint16_t n, uint32_t c) {
    if (n >= numLEDs) return; // '>=' because arrays are 0-indexed
    pixels[n*3  ] = (c >> 16);
    pixels[n*3+1] = (c >>  8);
    pixels[n*3+2] =  c;
    //pc.printf("setPixelColor-2 inputs\n");   
}
//*******************************************************************************
// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<numPixels(); i++) {
      setPixelColor(Matrix[i], c);      
      show();
      wait_ms(wait);
  }
}
//*******************************************************************************
// Fill the dots one after the other with a color
void colorAll(uint32_t c, uint16_t wait) {
  for(uint16_t i=0; i<numPixels(); i++) {
      setPixelColor(Matrix[i], c);
  }
  show();
  wait_ms(wait);
}

//*******************************************************************************
// Fill the dots one after the other with a color
void RandOne(uint8_t MaxRand, uint16_t wait) {
  for(uint16_t i=0; i<numPixels(); i++) {
        setPixelColor(Matrix[rand() % numLEDs-1], rand() % MaxRand,rand() % MaxRand,rand() % MaxRand);
        show();
        wait_ms(wait);
  }

}
//*******************************************************************************
// Fill the dots one after the other with a color
void RandAll(uint8_t MaxRand, uint16_t wait) {
  for(uint16_t i=0; i<numPixels(); i++) {
        setPixelColor(Matrix[i], rand() % MaxRand,rand() % MaxRand,rand() % MaxRand);
  }
  show();
  wait_ms(wait);
}
//*******************************************************************************
// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(int WheelPos) {
  if(WheelPos < 85) {
   return Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}
//*******************************************************************************
void rainbow(uint8_t rwait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<numPixels(); i++) {
      setPixelColor(i, Wheel((i+j) & 255));
    }
    show();
    wait_ms(rwait);
  }
}
//*******************************************************************************
// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t rwait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< numPixels(); i++) {
      setPixelColor(i, Wheel(((i * 256 / numPixels()) + j) & 255));
    }
    show();
    wait_ms(rwait);
  }
}
//*******************************************************************************
void Ring(int RingArray[], uint8_t r, uint8_t g, uint8_t b, uint8_t RingWait) {
   
    for (int i=1;i<RingArray[0];++i) {
        setPixelColor(RingArray[i], r, g, b);
        //pc.printf("RingArray[%d,%d]\n",RingArray[i],RingArray[0]);
    }
    show();
    wait_ms(RingWait);
}
//*******************************************************************************
void RingFadeUp(int RingArray[], uint8_t r, uint8_t g, uint8_t b, uint8_t FadeSteps, uint8_t FadeTime, uint16_t RingWait) {
    uint8_t rBump, gBump, bBump;
    uint8_t rStep, gStep, bStep;
    
    rStep = r/FadeSteps; if(rStep == 0) {rStep = 1;}
    gStep = g/FadeSteps; if(gStep == 0) {gStep = 1;}
    bStep = b/FadeSteps; if(bStep == 0) {bStep = 1;}
        
    rBump = 0;
    gBump = 0;
    bBump = 0;
    
    for (int j=0;j<FadeSteps;++j) {
        rBump += rStep; if(rBump > BrightMax) {rBump = 0;} //Account for Int rollover
        gBump += gStep; if(gBump > BrightMax) {gBump = 0;} //Account for Int rollover
        bBump += bStep; if(bBump > BrightMax) {bBump = 0;} //Account for Int rollover
        for (int i=1;i<RingArray[0];++i) {
            setPixelColor(RingArray[i], rBump, gBump, bBump);
            pc.printf("UP [%d,%d %d]\n",rBump, gBump, bBump);
        }
        show();
    wait_ms(FadeTime);    
    };
    wait_ms(RingWait);
}
//*******************************************************************************
void RingFadeDn(int RingArray[], uint8_t r, uint8_t g, uint8_t b, uint8_t FadeSteps, uint8_t FadeTime, uint16_t RingWait) {
    uint8_t rBump, gBump, bBump;
    uint8_t rStep, gStep, bStep;
    
    rStep = r/FadeSteps; if(rStep == 0) {rStep = 1;}
    gStep = g/FadeSteps; if(gStep == 0) {gStep = 1;}
    bStep = b/FadeSteps; if(bStep == 0) {bStep = 1;}
    
    rBump = r;
    gBump = g;
    bBump = b;
    
    for (int j=0;j<FadeSteps;++j) {
        rBump -= rStep; if(rBump > BrightMax) {rBump = 0;} //Account for Int rollover
        gBump -= gStep; if(gBump > BrightMax) {gBump = 0;} //Account for Int rollover
        bBump -= bStep; if(bBump > BrightMax) {bBump = 0;} //Account for Int rollover
        for (int i=1;i<RingArray[0];++i) {
            setPixelColor(RingArray[i], rBump, gBump, bBump);
            pc.printf("DN [%d,%d %d]\n",rBump, gBump, bBump);
        }
        show();
    wait_ms(FadeTime);    
    };
    wait_ms(RingWait);
}
//*******************************************************************************
uint8_t ColorRand(void) {
    return (rand()%(BrightMax-BrightMin)+BrightMin);
}
//*******************************************************************************
inline uint8_t fastCosineCalc( uint16_t preWrapVal)
{
  uint8_t wrapVal = (preWrapVal % 255);
  if (wrapVal<=0){
  wrapVal=255+wrapVal;
  }
  return cos_wave[wrapVal]; 
}
//*******************************************************************************
void Plasma()
{
    unsigned long frameCount=25500;  // arbitrary seed to calculate the three time displacement variables t,t2,t3
    while(1) {
        frameCount++ ; //42 35 38
        uint16_t t = fastCosineCalc((128 * frameCount)/100);  //time displacement - fiddle with these til it looks good...
        uint16_t t2 = fastCosineCalc((128 * frameCount)/100); 
        uint16_t t3 = fastCosineCalc((128 * frameCount)/100);
        
        for (uint8_t y = 0; y < ROWS_LEDs; y++) {
          int left2Right, pixelIndex;
          if (((y % (ROWS_LEDs/8)) & 1) == 0) {
            left2Right = 1;
            pixelIndex = y * COLS_LEDs;
          } else {
            left2Right = -1;
            pixelIndex = (y + 1) * COLS_LEDs - 1;
          }
          for (uint8_t x = 0; x < COLS_LEDs ; x++) {
            //Calculate 3 seperate plasma waves, one for each color channel
            uint8_t r = fastCosineCalc(((x << 3) + (t >> 1) + fastCosineCalc((t2 + (y << 3)))));
            uint8_t g = fastCosineCalc(((y << 3) + t + fastCosineCalc(((t3 >> 2) + (x << 3)))));
            uint8_t b = fastCosineCalc(((y << 3) + t2 + fastCosineCalc((t + x + (g >> 2)))));
            //uncomment the following to enable gamma correction
            r=exp_gamma[r];  
            g=exp_gamma[g];
            b=exp_gamma[b];
            setPixelColor(Matrix[pixelIndex],r,g,b);
            //pc.printf("Plasma [%d,%d,%d %d]\n",pixelIndex,r,g,b);
            pixelIndex += left2Right;
          }
        }
        show();
    }
}

//*******************************************************************************
//http://onehourhacks.blogspot.com/2012/03/conways-game-of-life-on-arduino.html
const int ROWS = 48;
const int COLS = 32;
int boardnum = 0; // number of boards run by the game
int iteration = 0; // current round in the current board
int numberAround(int row, int col);

// The "Alive" cells on the board.
uint32_t alive[ROWS] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

//*******************************************************************************
bool isAlive(int row, int col)
{
return alive[row] & (1<<(col));
}
//*******************************************************************************
void setAlive(int row, int col)
{
alive[row] |= 1 << col;
}
//*******************************************************************************
/*
 * Sets the alive array to all falses.
 */
void blank_alive()
{
for(int i = 0; i < ROWS; ++i)
 alive[i] = 0;
}
//*******************************************************************************
/**
 * Writes output to the console.
 */
 
void do_output()
{
 //blank();
 //pc.printf("Board: %d",boardnum);
 //pc.printf(" Iteration: %d\n",iteration);
    for(int i = 0; i < ROWS; i++)
    {
     for(int j = 0; j < COLS; j++)
     {
      // WIDTH, HEIGHT
      if(isAlive(i,j))
       {
       setPixelColor(Matrix[i+RowIdx[j]],rand() % BrightMax,rand() % BrightMax,rand() % BrightMax);
       //pc.printf("Alive: %d %d\n",i,j);
       }
      else
       setPixelColor(Matrix[i+RowIdx[j]],0,0,0);
     }
    }
}

/**
 * Randomly fills the grid with alive cells after blanking.
 */
void random_fill()
{
blank_alive();

// Fill up 10-30% of the cells
int numToFill = (ROWS * COLS) * (rand() % 30+10) / 100 ;

for(int r = 0; r < numToFill; r ++)
{
 int row = rand() % ROWS;
 int col = rand() % COLS;
 
 setAlive(row,col);
}
}

/**
 * Returns the index of the row below the current one.
 */
int rowBelow(int row)
{
return (row + 1 < ROWS) ? row + 1 : 0;
}

/**
 * Returns the index of the row above the given one
 */
int rowAbove(int row)
{
return (row > 0) ? row - 1 : ROWS - 1;
}

/** Returns the index of the col to the right of this one */
int colRight(int col)
{
return (col + 1 < COLS) ? col + 1 : 0;
}

/** Returns the index of the col to the left of this one */
int colLeft(int col)
{
return (col > 0) ? col - 1 : COLS -1;
}

/** true if the cell to the left is alive*/
bool left(int row, int col)
{
col = colLeft(col);
return isAlive(row,col);
}

/** true if the cell to the right is alive*/
bool right(int row, int col)
{
col = colRight(col);
return isAlive(row,col);
}

/** true if the cell above is alive*/
bool above(int row, int col)
{
row = rowAbove(row);
return isAlive(row,col);
}

/** true if the cell below is alive*/
bool below(int row, int col)
{
row = rowBelow(row);
return isAlive(row,col);
}

/** true if the cell NE is alive*/
bool aboveright(int row, int col)
{
row = rowAbove(row);
col = colRight(col);
return isAlive(row,col);
}

/** true if the cell SE is alive*/
bool belowright(int row, int col)
{
row = rowBelow(row);
col = colRight(col);
return isAlive(row,col);
}

/** true if the cell NW is alive*/
bool aboveleft(int row, int col)
{
row = rowAbove(row);
col = colLeft(col);
return isAlive(row,col);
}

/** true if the cell SW is alive*/
bool belowleft(int row, int col)
{
row = rowBelow(row);
col = colLeft(col);
return isAlive(row,col);
}

/**Returns the number of living cells sorrounding this one.*/
int numberAround(int row, int col)
{
int around = 0;
if(left(row,col))
 around++;

if(right(row,col))
 around++;

if(above(row,col))
 around++;

if(below(row,col))
 around++;

if(aboveright(row,col))
 around++;

if(aboveleft(row,col))
 around++;

if(belowright(row,col))
 around++;

if(belowleft(row,col))
 around++;

return around;
}

/**
 * Moves all of the cells
 */
void move()
{
uint32_t nextRows[ROWS] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0};

for(int i = 0; i < ROWS; i++)
{
 for(int j = 0; j < COLS; j++)
 {
  int na = numberAround(i,j);
  if((na == 2 && isAlive(i,j)) || na == 3)
   nextRows[i] |= 1 << j;
 }
}
for(int i = 0; i < ROWS; i++)
 alive[i] = nextRows[i];
}

//*******************************************************************************
int main() {
    pc.baud(38400);
    setup();
    while (1) {
        pc.printf("Top of the Loop\n");
        bLed = !bLed;
        blankDelay(0);
  
        boardnum++;
        random_fill();
        
        for(iteration = 0;iteration < 500; iteration++)
        {
         do_output();
         show();
         move(); 
         wait_ms(0);
        }
    }
}