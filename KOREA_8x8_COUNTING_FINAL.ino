//for 8x8 matrix of neopixels
//Korea tour version
//timings reflect new tracks from Leo
//written by David Crittenden 7/2018
//updated after meeting with musicians
//BUTTON LAYOUT
/*
  row 1: All around you WITH COUNT IN / Always Away NO INTRO / Everything is Beautiful Under the Sun NO INTRO / Sing WITH COUNT IN
  row 2: Inferno NO INTRO / Beautiful NO INTRO / All around you NO INTRO / Sing NO INTO
  row 3: CRISTIANO DANCE MUSIC / HEART FRAME FADE / Circle Droplet / Multi - Color Shock Wave
  row 4: FUCK TRUMP / Rainbow Checkers / Red-White Heart / CLEAR ALL
*/

//counts to 8 with the bpm
//works with matrix definitions only, not strip definitions!
//Functional with dma zero library!
//https://forum.arduino.cc/index.php?topic=476630.0 to explain how to resolve conflicts with the TC3

//radio transmitter to trigger neopixels
//uses the zero dma libraries https://learn.adafruit.com/dma-driven-neopixels/overview
//code derived from https://learn.adafruit.com/remote-effects-trigger/overview-1?view=all
//see also https://learn.adafruit.com/adafruit-feather-32u4-radio-with-rfm69hcw-module/using-the-rfm69-radio
//Ada_remoteFXTrigger_RX_NeoPixel
//Remote Effects Trigger Box Receiver
//by John Park
//for Adafruit Industries
//MIT License
//Modified by David Crittenden 11/2017 - 1/2018

#include <Adafruit_GFX.h>
#include <avr/pgmspace.h> //for the bitmap in program memory
#include <Adafruit_NeoPixel_ZeroDMA.h>
#include <Adafruit_NeoMatrix_ZeroDMA.h>//https://learn.adafruit.com/dma-driven-neopixels/overview
#include <SPI.h>
#include <RH_RF69.h>
#include <Wire.h>

#define LED 13

/********** NeoPixel Setup *************/
#define PIN            11

Adafruit_NeoMatrix_ZeroDMA matrix(8, 8, PIN,
                                  NEO_MATRIX_BOTTOM     + NEO_MATRIX_LEFT +
                                  NEO_MATRIX_COLUMNS + NEO_MATRIX_PROGRESSIVE,
                                  NEO_GRB            + NEO_KHZ800);

/************ Radio Setup ***************/
// Change to 434.0 or other frequency, must match RX's freq!
#define RF69_FREQ 915.0

//#if defined(ARDUINO_SAMD_FEATHER_M0) // Feather M0 w/Radio
#define RFM69_CS      8
#define RFM69_INT     3
#define RFM69_RST     4
//#endif

// Singleton instance of the radio driver
RH_RF69 rf69(RFM69_CS, RFM69_INT);

bool oldState = HIGH;
int showType = 0;

////////////////////////////variables used in different functions
//radio variables
uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];
uint8_t len = sizeof(buf);

//used for mode cycling
uint8_t mode = 0;
uint8_t currentMode;
uint8_t previousMode = 0;

//used for timing
int pass = 0;
int previousPass;
int count = 1;
int previousCount;
int segment = 0;
int previousSegment;
int i = 0;//ugh....
int part;//counter for loops within functions
bool ready = true;

int bpm;
unsigned long oneBeat;//calculates the number of milliseconds of one beat
unsigned long oneBar;//calculates the length of one bar in milliseconds
unsigned long currentMillis;
unsigned long startMillis;
unsigned long reStartMillis;
unsigned long previousRestartMillis;
unsigned long previousMillis;
unsigned long elapsedMillis;
unsigned long previousElapsedMillis;

//used for color cycling
uint8_t R;
uint8_t G;
uint8_t B;
uint8_t RA;
uint8_t GA;
uint8_t BA;
uint8_t red;
uint8_t green;
uint8_t blue;

int x = matrix.width();
int previousX;
int y = matrix.height();
int previousY;

//used for multiple random locations
byte XA = random(x);
byte YA = random(y);
byte XB = random(x);
byte YB = random(y);
byte XC = random(x);
byte YC = random(y);

//used for randomizing locations
int X;
int Y;
uint8_t j = 0;//color starts at 0 used in rainbow and wheel functions

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup()
{
  delay(500);
  Serial.begin(115200);

  /*while (!Serial)
    {
    delay(1);  // wait until serial console is open, remove if not tethered to computer
    }*/

  matrix.begin();
  matrix.setBrightness(30);
  matrix.show();

  randomSeed(analogRead(0));
  pinMode(LED, OUTPUT);
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);

  Serial.println("Feather RFM69 RX/TX Test!");
  // manual reset
  digitalWrite(RFM69_RST, HIGH);
  delay(10);
  digitalWrite(RFM69_RST, LOW);
  delay(10);
  if (!rf69.init())
  {
    Serial.println("RFM69 radio init failed");
    while (1);
  }
  Serial.println("RFM69 radio init OK!");
  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM (for low power module)
  // No encryption
  if (!rf69.setFrequency(RF69_FREQ)) Serial.println("setFrequency failed");

  // If you are using a high power RF69 eg RFM69HW, you *must* set a Tx power with the
  // ishighpowermodule flag set like this:
  rf69.setTxPower(14, true);//set to 20 on transmitter

  // The encryption key has to be the same as the one in the server
  uint8_t key[] =
  {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08
  };
  rf69.setEncryptionKey(key);

  pinMode(LED, OUTPUT);//sets the onboard led
  Serial.print("RFM69 radio @");  Serial.print((int)RF69_FREQ);  Serial.println(" MHz");

  //splash screen
  matrix.fillScreen(matrix.Color(0, 255, 0));
  matrix.setTextSize(1);//1 is standard
  matrix.setTextColor(matrix.Color(255, 255, 255));
  matrix.setCursor(1, 0);
  matrix.print("0");
  matrix.show();
  delay(2000);
  matrix.fillScreen(0);
  matrix.show();

  zeroOut();
  timeCheck();
}

void loop()
{
  buttonCheck();

  switch (mode)
  {
    case 0:
      matrix.fillScreen(0);
      matrix.show();
      buttonCheck();
      break;

    case 1://ALL AROUND YOU - WITH 8 BEAT COUNT IN - 120 BPM 4:12
      bpm = 120;
      oneBeat = (60000 / bpm);
      oneBar = oneBeat * 8;

      timeCheck();
      currentMode = mode;

      //ADDED BEATS OF SILENCE FOR COUNT IN
      if (ready == true)
      {
        segment = 0;
        ready = false;
      }
      while (elapsedMillis < (oneBar) && segment == 0)//COUNT IN
      {
        buttonCheck();
        if (currentMode != mode) break;
        R = 0;
        G = 255;
        B = 255;
        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar) && segment == 0) segment = 1, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar) && segment == 1)//INTRO
      {
        R = 255;
        G = 0;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar) && segment == 1) segment = 2, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 6) && segment == 2)//VERSE A1
      {
        R = 255;
        G = 255;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 6) && segment == 2) segment = 3, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 6) && segment == 3)//VERSE A2
      {
        R = 0;
        G = 255;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 6) && segment == 3) segment = 4, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 6) && segment == 4)//FIRST CHORUS
      {
        R = 255;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 6) && segment == 4) segment = 5, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 6) && segment == 5)//VERSE B1
      {
        R = 255;
        G = 255;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 6) && segment == 5) segment = 6, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 6) && segment == 6)//VERSE B2
      {
        R = 0;
        G = 255;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 6) && segment == 6) segment = 7, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 8) && segment == 7)//SECOND CHORUS
      {
        R = 255;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 8) && segment == 7) segment = 8, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 8)//MORF
      {
        R = 255;
        G = 255;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 8) segment = 9, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 9)//THIRD CHORUS
      {
        R = 255;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 9) segment = 10, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 6) && segment == 10)//GUITAR SOLO PART 1
      {
        R = 255;
        G = 0;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 6) && segment == 10) segment = 11, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 11)//GUITAR SOLO PART 2
      {
        R = 0;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 11) segment = 12, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 6) && segment == 12)//FINAL CHORUS
      {
        R = 255;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 6) && segment == 12) segment = 13, count = 1;

      matrix.fillScreen(0);
      matrix.show();
      break;

    case 2://ALWAYS AWAY - WITHOUT COUNT IN - 120 BPM
      bpm = 120;
      oneBeat = (60000 / bpm);
      oneBar = oneBeat * 8;
      timeCheck();
      currentMode = mode;

      while (elapsedMillis < (oneBar * 4) && segment == 1)//INTRO
      {
        R = 255;
        G = 255;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 1) segment = 2, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 2)//CHORUS C1
      {
        R = 255;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 2) segment = 3, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 3)//VERSE A1
      {
        R = 255;
        G = 0;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 3) segment = 4, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 4)//CHORUS C2
      {
        R = 255;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 4) segment = 5, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 5)//MORPH
      {
        R = 255;
        G = 255;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 5) segment = 6, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 6)//INTRO REPRISE
      {
        R = 255;
        G = 255;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 6) segment = 7, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 7)//CHORUS C3
      {
        R = 255;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 7) segment = 8, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 8)//MORPH REPRISE
      {
        R = 255;
        G = 255;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 8) segment = 9, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 9)//VERSE A2
      {
        R = 255;
        G = 0;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 9) segment = 10, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 10)//CHORUS C4
      {
        R = 255;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 10) segment = 11, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 11)//CHORUS C5
      {
        R = 127;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 11) segment = 12, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 7) && segment == 12)//SOLO
      {
        R = 0;
        G = 255;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 7) && segment == 12) segment = 13, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 1) && segment == 13)//INTRO REPRISE
      {
        R = 255;
        G = 255;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 1) && segment == 13) segment = 14, count = 1;
      //HARD END

      matrix.fillScreen(0);
      matrix.show();
      break;

    case 3://EVERYTHING IS BEAUTIFUL - WITHOUT COUNT IN - 129 BPM
      bpm = 129;
      oneBeat = (60000 / bpm);
      oneBar = oneBeat * 8;
      timeCheck();
      currentMode = mode;

      while (elapsedMillis < (oneBar) && segment == 1)//INTRO
      {
        R = 255;
        G = 255;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar) && segment == 1) segment = 2, count = 1;

      segmentZero(true);
      while (elapsedMillis < ((oneBar * 8) - (oneBeat * 4)) && segment == 2)//VERSE A1
      {
        R = 255;
        G = 0;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= ((oneBar * 8) - (oneBeat * 4)) && segment == 2) segment = 3, count = 5;

      segmentZero(true);
      while (elapsedMillis < ((oneBar * 8) + (oneBeat * 4)) && segment == 3)//CHORUS C1
      {
        R = 255;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= ((oneBar * 8) + (oneBeat * 4)) && segment == 3) segment = 4, count = 1;

      segmentZero(true);
      while (elapsedMillis < ((oneBar * 8) - (oneBeat * 4)) && segment == 4)//VERSE A2
      {
        R = 255;
        G = 0;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= ((oneBar * 8) - (oneBeat * 4)) && segment == 4) segment = 5, count = 5;

      segmentZero(true);
      while (elapsedMillis < ((oneBar * 12) + (oneBeat * 4)) && segment == 5)//CHORUS C2
      {
        R = 255;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= ((oneBar * 12) + (oneBeat * 4)) && segment == 5) segment = 6, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 8) && segment == 6)//BREAK
      {
        R = 255;
        G = 255;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 8) && segment == 6) segment = 7, count = 1;

      segmentZero(true);
      while (elapsedMillis < ((oneBar * 4) - (oneBeat * 4)) && segment == 7)//INTERLUDE
      {
        R = 0;
        G = 255;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= ((oneBar * 4) - (oneBeat * 4)) && segment == 7) segment = 8, count = 5;

      segmentZero(true);
      while (elapsedMillis < ((oneBar * 10) + (oneBeat * 4)) && segment == 8)//CHORUS C3
      {
        R = 255;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= ((oneBar * 10) + (oneBeat * 4)) && segment == 8) segment = 9, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 3) && segment == 9)//INTRO REPRISE
      {
        R = 255;
        G = 255;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 3) && segment == 9) segment = 10, count = 1;


      matrix.fillScreen(0);
      matrix.show();
      break;

    case 4://SING - WITH 8 BEAT COUNT IN - 100 BPM
      bpm = 100;
      oneBeat = (60000 / bpm);
      oneBar = oneBeat * 8;
      timeCheck();
      currentMode = mode;

      //ADDED BEATS OF SILENCE FOR COUNT IN
      if (ready == true)
      {
        segment = 0;
        ready = false;
      }
      while (elapsedMillis < (oneBar) && segment == 0)//COUNT IN
      {
        buttonCheck();
        if (currentMode != mode) break;
        R = 0;
        G = 255;
        B = 255;
        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar) && segment == 0) segment = 1, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 2) && segment == 1)//INTRO
      {
        R = 255;
        G = 255;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 2) && segment == 1) segment = 2, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 2)//VERSE A1
      {
        R = 255;
        G = 0;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 2) segment = 3, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 3)//VERSE B1
      {
        R = 0;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 3) segment = 4, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 2) && segment == 4)//BRIDGE
      {
        R = 255;
        G = 255;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 2) && segment == 4) segment = 5, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 2) && segment == 5)//CHORUS C1
      {
        R = 255;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 2) && segment == 5) segment = 6, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 6)//INTERLUDE
      {
        R = 0;
        G = 255;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 6) segment = 7, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 7)//VERSE A2
      {
        R = 255;
        G = 0;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 7) segment = 8, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 8)//B1 REPRISE
      {
        R = 0;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 8) segment = 9, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 9)//CHORUS C2
      {
        R = 255;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 9) segment = 10, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 10)//INTERLUDE
      {
        R = 0;
        G = 255;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 10) segment = 11, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 11)//CHORUS C3
      {
        R = 255;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 11) segment = 12, count = 1;

      matrix.fillScreen(0);
      matrix.show();
      break;

    case 5://INFERNO - NO INTRO - 134-135 BPM *varies to match the file 4:11
      bpm = 134;
      oneBeat = (60000 / bpm);
      oneBar = oneBeat * 8;
      timeCheck();
      currentMode = mode;
      //NO COUNT IN
      while (elapsedMillis < (oneBar * 2) && segment == 1)//INTRO
      {
        R = 255;
        G = 255;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 2) && segment == 1) segment = 2, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 2)//VERSE A1a
      {
        R = 255;
        G = 0;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 2) segment = 3, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 3)//VERSE A1b
      {
        R = 0;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 3) segment = 4, count = 1;

      segmentZero(true);
      //adjust for timing here
      bpm = 135;
      oneBeat = (60000 / bpm);
      oneBar = oneBeat * 8;
      while (elapsedMillis < (oneBar * 4) && segment == 4)//VERSE B1a
      {
        R = 255;
        G = 255;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 4) segment = 5, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 5)//VERSE B1b
      {
        R = 0;
        G = 255;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 5) segment = 6, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 6)//CHORUS C1a
      {
        R = 255;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 6) segment = 7, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 7)//CHORUS C1b
      {
        R = 0;
        G = 255;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 7) segment = 8, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 8)//MORPH1
      {
        R = 255;
        G = 10;
        B = 50;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 8) segment = 9, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 9)//VERSE A2a
      {
        R = 255;
        G = 0;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 9) segment = 10, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 10)//VERSE A2b
      {
        R = 0;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 10) segment = 11, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 11)//VERSE B2a
      {
        R = 255;
        G = 255;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 11) segment = 12, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 12)//VERSE B2b
      {
        R = 0;
        G = 255;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 12) segment = 13, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 13)//CHORUS C2a
      {
        R = 255;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 13) segment = 14, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 14)//CHORUS C2b
      {
        R = 0;
        G = 255;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 14) segment = 15, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 15)//MORPH2
      {
        R = 255;
        G = 10;
        B = 50;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 15) segment = 16, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 16)//CHORUS C3a
      {
        R = 255;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 16) segment = 17, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 17)//CHORUSC3b
      {
        R = 0;
        G = 255;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 17) segment = 18, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 18)//MORPH3
      {
        R = 255;
        G = 10;
        B = 50;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 18) segment = 19, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBeat * 4) && segment == 19)//SOFT END
      {
        R = 255;
        G = 255;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBeat * 4) && segment == 19) segment = 20, count = 1;
      matrix.fillScreen(0);
      matrix.show();
      break;

    case 6://BEAUTIFUL - NO COUNT IN - 108 BPM
      bpm = 108;
      oneBeat = (60000 / bpm);
      oneBar = oneBeat * 8;
      timeCheck();
      currentMode = mode;

      while (elapsedMillis < (oneBar) && segment == 1)//INTRO
      {
        R = 255;
        G = 255;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar) && segment == 1) segment = 2, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 2)//VERSE A1
      {
        R = 255;
        G = 0;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 2) segment = 3, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 3)//VERSE A2
      {
        R = 0;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 3) segment = 4, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 4)//CHORUS C1
      {
        R = 255;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 4) segment = 5, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 5)//VERSE A3
      {
        R = 255;
        G = 0;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 5) segment = 6, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 6)//VERSE A4
      {
        R = 0;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 6) segment = 7, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 7)//CHORUS C2
      {
        R = 255;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 7) segment = 8, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 8)//C2 REPRISE
      {
        R = 0;
        G = 255;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 8) segment = 9, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 8) && segment == 9)//MORPH
      {
        R = 255;
        G = 255;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 8) && segment == 9) segment = 10, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 10)//CHORUS C3
      {
        R = 255;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 10) segment = 11, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 11)//C3 REPRISE
      {
        R = 0;
        G = 255;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 11) segment = 12, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 12)//SOLO
      {
        R = 255;
        G = 255;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 12) segment = 13, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 3) && segment == 13)//CHORUS C4
      {
        R = 255;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 3) && segment == 13) segment = 0, count = 1;
      //HARD END

      matrix.fillScreen(0);
      matrix.show();
      break;

    case 7://ALL AROUND YOU - NO COUNT IN - 120 BPM 4:12
      bpm = 120;
      oneBeat = (60000 / bpm);
      oneBar = oneBeat * 8;
      timeCheck();
      currentMode = mode;
      //NO COUNT IN
      while (elapsedMillis < (oneBar) && segment == 1)//INTRO
      {
        R = 255;
        G = 0;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar) && segment == 1) segment = 2, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 6) && segment == 2)//VERSE A1
      {
        R = 255;
        G = 255;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 6) && segment == 2) segment = 3, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 6) && segment == 3)//VERSE A2
      {
        R = 0;
        G = 255;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 6) && segment == 3) segment = 4, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 6) && segment == 4)//FIRST CHORUS
      {
        R = 255;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 6) && segment == 4) segment = 5, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 6) && segment == 5)//VERSE B1
      {
        R = 255;
        G = 255;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 6) && segment == 5) segment = 6, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 6) && segment == 6)//VERSE B2
      {
        R = 0;
        G = 255;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 6) && segment == 6) segment = 7, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 8) && segment == 7)//SECOND CHORUS
      {
        R = 255;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 8) && segment == 7) segment = 8, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 8)//MORF
      {
        R = 255;
        G = 255;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 8) segment = 9, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 9)//THIRD CHORUS
      {
        R = 255;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 9) segment = 10, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 6) && segment == 10)//GUITAR SOLO PART 1
      {
        R = 255;
        G = 0;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 6) && segment == 10) segment = 11, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 11)//GUITAR SOLO PART 2
      {
        R = 0;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 11) segment = 12, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 6) && segment == 12)//FINAL CHORUS
      {
        R = 255;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 6) && segment == 12) segment = 13, count = 1;

      matrix.fillScreen(0);
      matrix.show();
      break;

    case 8://SING - WITHOUT COUNT IN - 100 BPM
      bpm = 100;
      oneBeat = (60000 / bpm);
      oneBar = oneBeat * 8;
      timeCheck();
      currentMode = mode;

      while (elapsedMillis < (oneBar * 2) && segment == 1)//INTRO
      {
        R = 255;
        G = 255;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 2) && segment == 1) segment = 2, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 2)//VERSE A1
      {
        R = 255;
        G = 0;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 2) segment = 3, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 3)//VERSE B1
      {
        R = 0;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 3) segment = 4, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 2) && segment == 4)//BRIDGE
      {
        R = 255;
        G = 255;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 2) && segment == 4) segment = 5, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 2) && segment == 5)//CHORUS C1
      {
        R = 255;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 2) && segment == 5) segment = 6, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 6)//INTERLUDE
      {
        R = 0;
        G = 255;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 6) segment = 7, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 7)//VERSE A2
      {
        R = 255;
        G = 0;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 7) segment = 8, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 8)//B1 REPRISE
      {
        R = 0;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 8) segment = 9, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 9)//CHORUS C2
      {
        R = 255;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 9) segment = 10, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 10)//INTERLUDE
      {
        R = 0;
        G = 255;
        B = 0;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 10) segment = 11, count = 1;

      segmentZero(true);
      while (elapsedMillis < (oneBar * 4) && segment == 11)//CHORUS C3
      {
        R = 255;
        G = 0;
        B = 255;

        buttonCheck();
        if (currentMode != mode) break;

        //do this
        eightCount(oneBeat);
        timeCheck();
      }
      if (elapsedMillis >= (oneBar * 4) && segment == 11) segment = 12, count = 1;

      matrix.fillScreen(0);
      matrix.show();
      break;
      
    case 16://bottom right black out
      matrix.fillScreen(0);
      matrix.show();
      buttonCheck();
      break;

    default://black out
      matrix.fillScreen(0);
      matrix.show();
      buttonCheck();
      break;
  }
}

void eightCount(unsigned long interval)
{
  matrix.setTextSize(1);
  matrix.setTextColor(matrix.Color(R, G, B));
  matrix.fillScreen(0);
  matrix.setCursor(2, 0);
  matrix.print(count);
  matrix.show();
  timeCheck();

  currentMillis = millis();
  if (currentMillis - previousMillis > interval)
  {
    previousMillis = currentMillis;
    count++;
    if (count > 8) count = 1;
  }
}

void timeCheck()
{
  currentMillis = millis();//check the time
  elapsedMillis = currentMillis - reStartMillis;//check if enough time has passed
}

void zeroOut()
{
  x = matrix.width();
  y = matrix.height();
  i = 0;
  part = 0;
  segment = 1;
  pass = 0;
  count = 1;
  ready = true;
  currentMillis = millis();//check the time
  reStartMillis = currentMillis;
  previousMillis = currentMillis;
  elapsedMillis = 0;
}

void segmentZero(bool blackOut)
{
  reStartMillis = millis();
  previousMillis = millis();
  timeCheck();
  currentMode = mode;
  i = 0;
  j = 0;
  x = matrix.width();
  y = matrix.height();
  if (blackOut == true) matrix.fillScreen(0);
}

void previousValues()
{
  //currentMode = mode;
  previousMode = mode;
  previousCount = count;
  previousPass = pass;
  previousSegment = segment;
  previousX = x;
  previousY = y;
  previousRestartMillis = reStartMillis;
  timeCheck();
  previousElapsedMillis = elapsedMillis;
}

void restoreValues()
{
  x = previousX;
  y = previousY;
  mode = previousMode;
  count = previousCount;
  pass = previousPass;
  segment = previousSegment;
  elapsedMillis = previousElapsedMillis;
  reStartMillis = previousRestartMillis;
}

void buttonCheck()
{
  if (rf69.waitAvailableTimeout(2))//originally 100
  {
    // Should be a message for us now
    buf[RH_RF69_MAX_MESSAGE_LEN];
    len = sizeof(buf);

    if (! rf69.recv(buf, &len))
    {
      Serial.println("Receive failed");
      return;
    }

    rf69.printBuffer("Received: ", buf, len);
    buf[len] = 0;

    previousValues();
    mode = buf[0];
    zeroOut();
  }
}

//END OF GRAMMERCY_8x8_COUNTING_PERFORMANCE
