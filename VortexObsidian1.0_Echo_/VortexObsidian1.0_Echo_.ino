
#include <FastLED.h>
#include <FlashStorage.h>
#include "Modes.h"
#include "Buttons.h"

#include <Adafruit_DotStar.h>

#include <IRLibSendBase.h>
#include <IRLibDecodeBase.h>
#include <IRLib_P01_NEC.h>
#include <IRLibCombo.h>

#include <IRLibRecv.h>


#define NUM_LEDS 28
#define DATA_PIN 4
#define CLOCK_PIN 3

#define totalModes 14 // How many modes the vortex cycles through
#define totalPatterns 22 // How many possible patterns there are

//Objects
//---------------------------------------------------------

CRGB leds[NUM_LEDS];
CRGB boardlight[1];
Modes mode[totalModes];
Buttons button[2];

IRdecode myDecoder;
IRrecv myReceiver(2);
IRsend mySender;

typedef struct Orbit {
  bool dataIsStored;
  uint8_t sHue[totalModes][8];
  uint8_t sSat[totalModes][8];
  uint8_t sVal[totalModes][8];
  uint8_t sNumColors[totalModes];
  uint8_t sPatternNum[totalModes];
  uint8_t brightness;
  uint8_t demoSpeed;
};
FlashStorage(saveData, Orbit);

//Variable
//---------------------------------------------------------

bool sharing = true, restore = false;
bool dataStored = 0;
bool on, on2, on3;
int m = 0;
byte menu;
byte stage = 0;
byte frame = 0;
byte qBand;
int gap;
int patNum;
byte dot = 0;
byte k = 0;
int targetSlot;
byte currentSlot;
int targetZone;
byte colorZone;
int targetHue;
byte selectedHue;
int targetSat;
byte selectedSat;
int targetVal;
byte selectedVal;
bool buttonState, buttonState2, lastButtonState, lastButtonState2 = 0;
unsigned long mainClock, prevTime, duration, prevTime2;
int data1[8];
int data2[8];
int data3[8];
bool received1, received2, received3;
int rep = 0;

unsigned long prevTime3, prevTime4;
unsigned long pressTime, prevPressTime, holdTime, prevHoldTime;

const byte numChars = 128;
char receivedChars[numChars];
char tempChars[numChars];

boolean newData = false;

int dataNumber = 0;

int menuSection, brightVal = 2, prevBrightness = 20;

// Demo mode keeps rolling randomized colors until user confirms or cancels
int demoSpeed = 0;
int newDemoSpeed = 0;
bool demoMode = false;
unsigned long demoTime;

int brightness;

//Main body
//---------------------------------------------------------

void setup() {
  Serial.begin(9600);
  FastLED.addLeds<DOTSTAR, DATA_PIN, CLOCK_PIN, BGR>(leds, NUM_LEDS);
  FastLED.setBrightness(20);
  button[0].createButton(19);
  button[1].createButton(20);
  setDefaults();
  loadSave();
  prevPressTime = 0;
  prevTime = 0;
  duration = 0;
  mode[m].menuNum = 0;
  randomSeed(analogRead(0));

  Adafruit_DotStar strip = Adafruit_DotStar(1, 7, 8, DOTSTAR_BGR);
  strip.begin();
  strip.show();
}

void loop() {
  menu = mode[m].menuNum;
  if (menu == 0) playMode();
  if (menu == 1) menuRing(0); //Button 1: Start Randomizer //Choose Colors //Choose Pattern //Share&Receive Mode
  if (menu == 2) menuRing(1); //Button 2: Global Brightness //Demo Speed //Restore Defaults
  if (menu == 3) colorSet();
  if (menu == 4) patternSelect();
  if (menu == 5) modeSharing();
  if (menu == 6) chooseBrightness();
  if (menu == 7) chooseDemoSpeed();
  if (menu == 8) restoreDefaults();
  if (menu == 9) confirmBlink();

  checkButton();
  checkSerial();
  FastLED.setBrightness(brightness);
  FastLED.show();
  //Serial.println(m);
}

int hue, sat, val;

void playMode() {
  mainClock = millis();

  if (demoMode) runDemo();

  patterns(mode[m].patternNum);
  catchMode();
}

//Patterns
//---------------------------------------------------------
// To add new patterns add a switch to this method and update totalPatterns.
// In your pattern code, getColor() will set the hue, sat, val variables for this iteration.
// Then call setLeds() to set those values on to the leds you want.
// The LEDs are arranged from the center out and around the other side then back to center.
// So 0,1,2 is one arm, 3 is the edge, then 4,5,6 [o] 7,8,9 on the other side then 10 on the edge. Etc...
// this code will repeat forever, incrementing "mainClock" with each iteration.

void patterns(int pat) {

  int totalColors = mode[m].numColors;
  int currentColor = mode[m].currentColor;
  int next = mode[m].nextColor;

  switch (pat) {
    case 1: { // All tracer
        getColor(0);
        setLeds(0, 27);
        if (on) {
          getColor(currentColor);
          if (totalColors == 1) val = 0;
          setLeds(0, 27);
          duration = 2;
        }
        if (!on) duration = 10;
        if (mainClock - prevTime > duration) {
          if (!on)nextColor(1);
          on = !on;
          prevTime = mainClock;
        }
        break;
      }

    case 2: { // SparkleTrace
        getColor(0);
        setLeds(0, 27);
        if (on) {
          getColor(currentColor);
          if (totalColors == 1) val = 0;
          setLed(random(0, 7));
          setLed(random(0, 7));
          setLed(random(7, 14));
          setLed(random(7, 14));
          setLed(random(14, 21));
          setLed(random(14, 21));
          setLed(random(21, 28));
          setLed(random(21, 28));
        }
        if (!on) nextColor (1);
        on = !on;
        break;
      }

    case 3: { // Vortex
        getColor(currentColor);
        if (mainClock - prevTime > 5) {
          clearAll();
          for (int side = 0; side < 4; side++) {
            if (frame <= 3) {
              setLed(3 + (7 * side) + frame);
              setLed(3 + (7 * side) - frame);
            }
            if (frame >= 4) {
              setLed(3 + (7 * side) + (6 - frame));
              setLed(3 + (7 * side) - (6 - frame));
            }
          }
          nextColor (0);
          frame++;
          if (frame > 6) frame = 0;
          prevTime = mainClock;
        }
        break;
      }

    case 4: { // Dot Zip
        getColor(currentColor);
        if (mainClock - prevTime > 1) {
          clearAll();
          qBand++;
          if (qBand > 6) qBand = 0;
          setLed(qBand);
          setLed(qBand + 7);
          setLed(qBand + 14);
          setLed(qBand + 21);
          nextColor (0);
          prevTime = mainClock;
        }
        break;
      }

    case 5: { // Cross strobe
        getColor(currentColor);
        if (mainClock - prevTime > 3) {
          clearAll();
          for (int s = 0; s < 7; s++) {
            if (on) {
              setLed(s);
              setLed(s + 14);
            }
            if (!on) {
              setLed(s + 7);
              setLed(s + 21);
            }
            nextColor (0);
          }

          on = !on;
          prevTime = mainClock;
        }
        break;
      }

    case 6: { // Impact
        if (mainClock - prevTime > 1) on = !on, prevTime = mainClock;
        getColor(0);
        if (on) {
          clearAll();
          setLeds(0, 2);
          setLeds(11, 17);
          setLeds(25, 27);
        }
        getColor(1);
        if (totalColors == 1) val = 0;
        if (!on) {
          clearAll();
          setLeds(4, 9);
          setLeds(18, 23);
        }
        getColor(currentColor);
        if (totalColors <= 2) val = 0;
        setLed(3), setLed(10), setLed(17), setLed(24);
        nextColor(2);
        break;
      }

    case 7: { // Blend
        getColor(currentColor);
        int color1 = mode[m].hue[currentColor];
        int color2 = mode[m].hue[next];
        if (color1 > color2 && color1 - color2 < (255 - color1) + color2)gap--;
        if (color1 > color2 && color1 - color2 > (255 - color1) + color2)gap++;
        if (color1 < color2 && color2 - color1 < (255 - color2) + color1)gap++;
        if (color1 < color2 && color2 - color1 > (255 - color2) + color1)gap--;
        if (color1 + gap >= 255) gap -= 255;
        if (color1 + gap < 0) gap += 255;
        int finalHue = color1 + gap;
        if (finalHue == color2) gap = 0, nextColor(0);
        for (int a = 0; a < 28; a++) leds[a].setHSV(finalHue, sat, val);
        break;
      }

    case 8: { // Chroma Crush
        getColor(currentColor);
        setLeds(0, 27);
        nextColor(0);

        break;
      }

    case 9: { // Dops Crush
        if (on) {
          clearAll();
          if (mainClock - prevTime > 7) {
            on = !on;
            prevTime = mainClock;
          }
        }
        if (!on) {
          getColor(currentColor);
          setLeds(0, 27);
          if (currentColor == totalColors - 1) on = !on;
          nextColor(0);
        }

        break;
      }

    case 10: { // Meteor
        for (int a = 0; a < NUM_LEDS; a++)leds[a].fadeToBlackBy(30);
        if (mainClock - prevTime > 4) {
          getColor(currentColor);
          setLed(random(0, 7));
          setLed(random(0, 7));
          setLed(random(7, 14));
          setLed(random(7, 14));
          setLed(random(14, 21));
          setLed(random(14, 21));
          setLed(random(21, 28));
          setLed(random(21, 28));
          nextColor (0);
          prevTime = mainClock;
        }

        break;
      }

    case 11: { //Carnival Chroma
        if (mainClock - prevTime > 20) {
          for (int i = 0; i < NUM_LEDS; i++) {
            if (i % 2 == 0) {
              getColor(currentColor);
              setLed(i);
            }
            if (i % 2 == 1) {
              getColor(currentColor + 1);
              if (currentColor + 1 == totalColors) {
                getColor(0);
              }
              setLed(i);
            }
          }
          nextColor(0);
          prevTime = mainClock;
        }

        break;
      }

    case 12: { // Vortex Wipe
        getColor(currentColor);
        if (mainClock - prevTime > 4) {
          for (int side = 0; side < 4; side++) {
            if (frame <= 3) {
              setLed(3 + (7 * side) + frame);
              setLed(3 + (7 * side) - frame);
            }
            if (frame >= 4) {
              clearLight(3 + (7 * side) + (frame - 4));
              clearLight(3 + (7 * side) - (frame - 4));
            }
          }
          if (frame == 7) nextColor (0);
          frame++;
          if (frame > 7) {
            frame = 0;
          }
          prevTime = mainClock;
        }

        break;
      }

    case 13: { //Warp Worm
        getColor(0);
        setLeds(0, 27);
        getColor(currentColor);
        for (int i = 0; i < 25; i++) {
          int chunk = i + k;
          if (chunk > 27) chunk -= 28;
          setLed(chunk);
        }
        if (mainClock - prevTime > 1) {
          k++;
          prevTime = mainClock;
          if (k > 27) k = 0, nextColor(1);
        }
        break;
      }

    case 14: { // MegaDops
        if (on) {
          getColor(currentColor);
          setLeds(0, 27);
          duration = 1;
        }
        if (!on) {
          clearAll();
          duration = 3;
        }
        if (mainClock - prevTime > duration) {
          if (!on)nextColor(0);
          on = !on;
          prevTime = mainClock;
        }

        break;
      }

    case 15: { // Warp Wipe
        if (mainClock - prevTime > 5) {
          getColor(currentColor);
          setLed(dot);
          dot++;
          if (dot >= NUM_LEDS) {
            dot = 0;
            nextColor(0);
          }
          prevTime = mainClock;
        }

        break;
      }

    case 16: { // Double Warp
        getColor(0);
        setLeds(0, 27);
        getColor(currentColor);
        for (int i = 0; i < 4; i++) {
          int chunk = i + k;
          if (chunk > 27) chunk -= 28;
          setLed(chunk);
          int otherChunk = i + k + 7;
          if (otherChunk > 27) otherChunk -= 28;
          setLed(otherChunk);
          int otherChunk1 = i + k + 14;
          if (otherChunk1 > 27) otherChunk1 -= 28;
          setLed(otherChunk1);
          int otherChunk2 = i + k + 21;
          if (otherChunk2 > 27) otherChunk2 -= 28;
          setLed(otherChunk2);
        }
        if (mainClock - prevTime > 1) {
          if (currentColor == totalColors - 1) k++;
          prevTime = mainClock;
          if (k > 27) k = 0;
          nextColor(1);
        }

        break;
      }

    case 17: { //Warp Fade
        if (mainClock - prevTime2 > 1) {
          for (int a = 0; a < NUM_LEDS; a++)leds[a].fadeToBlackBy(20);
          prevTime2 = mainClock;
        }
        if (mainClock - prevTime > 1) {
          getColor(currentColor);
          setLed(dot);
          dot++;
          if (dot >= NUM_LEDS) {
            dot = 0;
            nextColor(0);
          }
          prevTime = mainClock;
        }

        break;
      }

    case 18: { // Double fade
        if (mainClock - prevTime2 > 1) {
          for (int a = 0; a < NUM_LEDS; a++)leds[a].fadeToBlackBy(16);
          prevTime2 = mainClock;
        }
        if (mainClock - prevTime > 4) {
          getColor(currentColor);
          setLed(dot);
          dot++;
          int dot2 = dot + 13;
          if (dot2 > 27) dot2 -= 28;
          setLed(dot2);
          if (dot >= NUM_LEDS) {
            dot = 0;
            nextColor(0);
          }
          prevTime = mainClock;
        }

        break;
      }

    case 19: { ///Chroma Sizzle
        int pos;
        if (!on) {
          if (rep == 0) pos = 3;//1
          if (rep == 1) pos = 2;//0
          getColor(mode[m].currentColor);
          for (int side = 0; side < 4; side++) {
            setLed(3 + (7 * side) + pos);
            setLed(3 + (7 * side) - pos);
            setLed(3 + (7 * side) + (pos - 2));
            setLed(3 + (7 * side) - (pos - 2));
          }
          nextColor(0);
        }
        if (on) {
          if (rep == 0) pos = 3;
          if (rep == 1) pos = 2;
          for (int side = 0; side < 4; side++) {
            clearLight(3 + (7 * side) + pos);
            clearLight(3 + (7 * side) - pos);
            clearLight(3 + (7 * side) + (pos - 2));
            clearLight(3 + (7 * side) - (pos - 2));
          }
        }
        on = !on;

        getColor(0);
        if (rep == 1) pos = 3;
        if (rep == 0) pos = 2;
        for (int side = 0; side < 4; side++) {
          setLed(3 + (7 * side) + pos);
          setLed(3 + (7 * side) - pos);
          setLed(3 + (7 * side) + (pos - 2));
          setLed(3 + (7 * side) - (pos - 2));
        }
        if (on2) {
          getColor(mode[m].currentColor1);
          if (mode[m].numColors == 1) val = 0;
          if (rep == 1) pos = 3;
          if (rep == 0) pos = 2;
          duration = 100;
          for (int side = 0; side < 4; side++) {
            setLed(3 + (7 * side) + pos);
            setLed(3 + (7 * side) - pos);
            setLed(3 + (7 * side) + (pos - 2));
            setLed(3 + (7 * side) - (pos - 2));
          }
        }
        if (!on2) duration = 100;
        if (mainClock - prevTime2 > duration) {
          if (!on2)nextColor1(1);
          on2 = !on2;
          prevTime2 = mainClock;
        }
        if (mainClock - prevTime4 > 500) {
          rep++;
          if (rep > 1) rep = 0;
          prevTime4 = mainClock;
        }

        break;
      }

    case 20: { // Backstrobe
        int pos;
        if (!on3) {
          getColor(mode[m].currentColor);
          if (rep == 0) pos = 3;
          if (rep == 1) pos = 2;
          for (int side = 0; side < 4; side++) {
            setLed(3 + (7 * side) + pos);
            setLed(3 + (7 * side) - pos);
            setLed(3 + (7 * side) + (pos - 2));
            setLed(3 + (7 * side) - (pos - 2));
          }
        }
        if (on3) {
          if (rep == 0) pos = 3;
          if (rep == 1) pos = 2;
          for (int side = 0; side < 4; side++) {
            clearLight(3 + (7 * side) + pos);
            clearLight(3 + (7 * side) - pos);
            clearLight(3 + (7 * side) + (pos - 2));
            clearLight(3 + (7 * side) - (pos - 2));
          }
        }
        if (mainClock - prevTime3 > 20) {
          on3 = !on3;
          if (!on3) nextColor(0);
          prevTime3 = mainClock;
        }

        if (mainClock - prevTime > 1) {
          getColor(mode[m].currentColor1);
          if (rep == 0) pos = 2;
          if (rep == 1) pos = 3;
          for (int side = 0; side < 4; side++) {
            setLed(3 + (7 * side) + pos);
            setLed(3 + (7 * side) - pos);
            setLed(3 + (7 * side) + (pos - 2));
            setLed(3 + (7 * side) - (pos - 2));
          }
          nextColor1(0);
          prevTime = mainClock;
        }
        if (mainClock - prevTime4 > 500) {
          rep++;
          if (rep > 1) rep = 0;
          prevTime4 = mainClock;
        }

        break;
      }
    case 21: { // Dash dops (first color dash, rest of the colors dops)
        if (on) {
          getColor(currentColor);
          setLeds(0, 27);
          if (currentColor == 0) duration = 20;
          else duration = 1;
        }
        if (!on) {
          clearAll();
          duration = 2;
        }
        if (mainClock - prevTime > duration) {
          if (!on)nextColor(0);
          on = !on;
          prevTime = mainClock;
        }
        break;
      }

    default: { // All Ribbon - executed if pat == 0 or out-of-range
        if (mainClock - prevTime > 10) {
          getColor(currentColor);
          setLeds(0, 27);
          nextColor(0);
          prevTime = mainClock;
        }
      }
  }
}

// Randomize colors and pattern every so often
void runDemo() {
  int demoInterval = 0;
  if (demoSpeed == 0) demoInterval = 600;
  if (demoSpeed == 1) demoInterval = 6000;
  if (demoSpeed == 2) demoInterval = 10000;
  if (demoSpeed == 3) demoInterval = 16000;

  if (mainClock - demoTime > demoInterval) {
    rollColors();
    demoTime = mainClock;
  }
}

//Led controlls for running patterns
//-----------------------------------------------------

void getColor(int target) {
  hue = mode[m].hue[target];
  sat = mode[m].sat[target];
  val = mode[m].val[target];
}

void setLed(int target) {
  leds[target].setHSV(hue, sat, val);
}

void setLeds(int first, int last) {
  for (int a = first; a <= last; a++) setLed(a);
}

void nextColor(int start) {
  mode[m].currentColor++;
  if (mode[m].currentColor >= mode[m].numColors) mode[m].currentColor = start;
  mode[m].nextColor = mode[m].currentColor + 1;
  if (mode[m].nextColor >= mode[m].numColors) mode[m].nextColor = start;
}

void nextColor1(int start) {
  mode[m].currentColor1++;
  if (mode[m].currentColor1 >= mode[m].numColors) mode[m].currentColor1 = start;
  mode[m].nextColor1 = mode[m].currentColor1 + 1;
  if (mode[m].nextColor1 >= mode[m].numColors) mode[m].nextColor1 = start;
}

void clearAll() {
  for (int a = 0; a < 28; a++) leds[a].setHSV(0, 0, 0);
}

void clearLight(int lightNum) {
  leds[lightNum].setHSV(0, 0, 0);
}

void blinkTarget(unsigned long blinkTime) {
  mainClock = millis();
  if (mainClock - prevTime > blinkTime) {
    on = !on;
    prevTime = mainClock;
  }
}

// Randomizer
//---------------------------------------------------------

void rollColors() {
  rollPattern();
  int type = random(0, 10);
  //true random, monochrome, complimentary, analogous, triadic, split complimentary, tetradic
  if (type == 0) { // true random
    mode[m].numColors = random(1, 8);
    for (int r = 0; r < 8; r ++) {
      mode[m].hue[r] = random(0, 16) * 16;
      mode[m].sat[r] = random(0, 4) * 85;
      mode[m].val[r] = random(1, 4) * 85;
    }
  }
  if (type == 1) { // monochrome
    mode[m].numColors = 4;
    int tempHue = random(0, 16) * 16;
    for (int r = 0; r < 4; r++) {
      mode[m].hue[r] = tempHue;
      mode[m].sat[r] = r * 85;
      mode[m].val[r] = random(1, 4) * 85;
    }
  }
  if (type == 2) { // complimentary
    mode[m].numColors = 2;
    int tempHue = random(0, 16) * 16;
    int compHue = tempHue + 128;
    if (compHue >= 255) compHue -= 256;
    mode[m].hue[0] = tempHue;
    mode[m].hue[1] = compHue;
    for (int r = 0; r < 2; r++) {
      mode[m].sat[r] = 255;
      mode[m].val[r] = random(1, 4) * 85;
    }
  }
  if (type == 3) { // analogous
    mode[m].numColors = 3;
    int tempHue = random(0, 16) * 16;
    int analHue1 = tempHue - 16;
    if (analHue1 < 0) analHue1 += 256;
    int analHue2 = tempHue + 16;
    if (analHue2 > 255) analHue2 -= 256;
    mode[m].hue[0] = tempHue;
    mode[m].hue[1] = analHue1;
    mode[m].hue[2] = analHue2;
    for (int r = 0; r < 3; r++) {
      mode[m].sat[r] = 255;
      mode[m].val[r] = random(1, 4) * 85;
    }
  }
  if (type == 4) { // triadic
    mode[m].numColors = 3;
    int tempHue = random(0, 16) * 16;
    int triadHue1 = tempHue + 80;
    int triadHue2 = tempHue - 80;
    if (triadHue1 > 255) triadHue1 -= 256;
    if (triadHue2 < 0) triadHue2 += 256;
    mode[m].hue[0] = tempHue;
    mode[m].hue[1] = triadHue1;
    mode[m].hue[2] = triadHue2;
    for (int r = 0; r < 3; r++) {
      mode[m].sat[r] = 255;
      mode[m].val[r] = random(1, 4) * 85;
    }
  }
  if (type == 5) { // split complimentary
    mode[m].numColors = 3;
    int tempHue = random(0, 16) * 16;
    int splitCompHue1 = tempHue + 112;
    int splitCompHue2 = tempHue - 112;
    if (splitCompHue1 > 255) splitCompHue1 += 256;
    if (splitCompHue2 < 0) splitCompHue2 += 256;
    mode[m].hue[0] = tempHue;
    mode[m].hue[1] = splitCompHue1;
    mode[m].hue[2] = splitCompHue2;
    for (int r = 0; r < 3; r++) {
      mode[m].sat[r] = 255;
      mode[m].val[r] = random(1, 4) * 85;
    }
  }
  if (type == 6) { // tetradic
    mode[m].numColors = 4;
    int tempHue = random(0, 16) * 16;
    int tetradHue1 = tempHue + 48;
    int tetradHue2 = tempHue + 128;
    int tetradHue3 = tempHue + 208;
    if (tetradHue1 > 255) tetradHue1 -= 256;
    if (tetradHue2 > 255) tetradHue2 -= 256;
    if (tetradHue3 > 255) tetradHue3 -= 256;
    mode[m].hue[0] = tempHue;
    mode[m].hue[1] = tetradHue1;
    mode[m].hue[2] = tetradHue2;
    mode[m].hue[3] = tetradHue3;
    for (int r = 0; r < 4; r++) {
      mode[m].sat[r] = 255;
      mode[m].val[r] = random(1, 4) * 85;
    }
  }
  if (type == 7) { // square
    mode[m].numColors = 4;
    int tempHue = random(0, 16) * 16;
    int tetradHue1 = tempHue + 64;
    int tetradHue2 = tempHue + 128;
    int tetradHue3 = tempHue + 192;
    if (tetradHue1 > 255) tetradHue1 -= 256;
    if (tetradHue2 > 255) tetradHue2 -= 256;
    if (tetradHue3 > 255) tetradHue2 -= 256;
    for (int r = 0; r < 4; r++) {
      mode[m].sat[r] = 255;
      mode[m].val[r] = random(1, 4) * 85;
    }
  }
  if (type == 8) { // full rainbow
    mode[m].numColors = 8;
    int shift = random(0, 7);
    for (int r = 0; r < 8; r++) {
      int rshift = r + shift;
      if (rshift >= 8) rshift -= 8;
      mode[m].hue[rshift] = r * 32;
      mode[m].sat[rshift] = 255;
      mode[m].val[rshift] = random(1, 4) * 85;
    }
    int reroll = random(0, 3);
    if (reroll == 2) rollColors();
  }
  if (type == 9) { // Solid
    mode[m].numColors = 1;
    mode[m].hue[0] = random(0, 16) * 16;
    mode[m].sat[0] = random(0, 4) * 85;
    mode[m].val[0] = random(1, 4) * 85;
    int reroll = random(0, 2);
    if (reroll == 1) rollColors();
  }
  if (mode[m].patternNum == 6 && mode[m].numColors < 3) rollColors();

  int blank = random(0, 4); // randomly chooses to add blanks to colorset
  if (blank == 0) {
    int blankType = 0;
    if (mode[m].numColors == 8) mode[m].val[0] = random(1, 2) * 85;// Dim first color
    if (mode[m].numColors >= 1 && mode[m].numColors <= 7) blankType = 1;
    if (mode[m].numColors >= 2 && mode[m].numColors <= 6) blankType = random (1, 3);
    if (mode[m].numColors >= 3 && mode[m].numColors <= 5) blankType = random (1, 3);
    if (mode[m].patternNum == 6) blankType = 0;

    if (blankType == 1) { // Blank at beginning
      for (int c = 0; c < mode[m].numColors; c++) {
        mode[m].hue[mode[m].numColors - c] = mode[m].hue[mode[m].numColors - (1 + c)];
        mode[m].sat[mode[m].numColors - c] = mode[m].sat[mode[m].numColors - (1 + c)];
        mode[m].val[mode[m].numColors - c] = mode[m].val[mode[m].numColors - (1 + c)];
      }
      mode[m].val[0] = 0;
      mode[m].numColors += 1;
    }
    if (blankType == 2) { // Blank at middle and beginning
      for (int c = 0; c < (mode[m].numColors / 2 + 1 ); c++) {
        mode[m].hue[mode[m].numColors - c] = mode[m].hue[mode[m].numColors - (1 + c)];
        mode[m].sat[mode[m].numColors - c] = mode[m].sat[mode[m].numColors - (1 + c)];
        mode[m].val[mode[m].numColors - c] = mode[m].val[mode[m].numColors - (1 + c)];
      }
      mode[m].val[mode[m].numColors / 2] = 0;
      mode[m].numColors += 1;
      for (int c = 0; c < mode[m].numColors; c++) {
        mode[m].hue[mode[m].numColors - c] = mode[m].hue[mode[m].numColors - (1 + c)];
        mode[m].sat[mode[m].numColors - c] = mode[m].sat[mode[m].numColors - (1 + c)];
        mode[m].val[mode[m].numColors - c] = mode[m].val[mode[m].numColors - (1 + c)];
      }
      mode[m].val[0] = 0;
      mode[m].numColors += 1;
    }
  }
}

void rollPattern() {
  mode[m].patternNum = random(0, totalPatterns);
}

//Menus and settings
//---------------------------------------------------------

//void openColors() {
//  mainClock = millis();
//  if (mainClock - prevTime > 75) {
//    clearAll();
//    for (int side = 0; side < 4; side++) {
//      if (frame >= 0 && frame <= 3) {
//        leds[3 + (7 * side) + frame].setHSV(0, 0, 170);
//        leds[3 + (7 * side) - frame].setHSV(0, 0, 170);
//      }
//      if (frame >= 4 && frame <= 6) {
//        leds[3 + (7 * side) + (6 - frame)].setHSV(0, 0, 170);
//        leds[3 + (7 * side) - (6 - frame)].setHSV(0, 0, 170);
//      }
//    }
//    frame++;
//    if (frame > 6) frame = 0;
//    prevTime = mainClock;
//  }
//}

void colorSet() {
  if (stage == 0) {
    int setSize = mode[m].numColors;
    clearAll();
    for (int colorSlot = 0; colorSlot < 8; colorSlot++) {
      int side = colorSlot / 2;
      leds[colorSlot + (5 * side)].setHSV(0, 0, 1);
      leds[((2 + (3 * side)) * 3) - colorSlot].setHSV(0, 0, 1);
    }
    for (int colorNum = 0; colorNum < setSize; colorNum++) {
      getColor(colorNum);
      int side = colorNum / 2;
      leds[(5 * side) + colorNum].setHSV(hue, sat, val);
      leds[((2 + (3 * side)) * 3) - colorNum].setHSV(hue, sat, val);
    }
    if (targetSlot < setSize) {
      if (on) {
        int side = targetSlot / 2;
        int blinkVal = 0;
        if (mode[m].val[targetSlot] == 0)blinkVal = 5;
        leds[targetSlot + (5 * side)].setHSV(0, 0, blinkVal);
        leds[((2 + (3 * side)) * 3) - targetSlot].setHSV(0, 0, blinkVal);
      }
      blinkTarget(300);
    }
    if (targetSlot == setSize) {
      if (on) {
        int side = (setSize - 1) / 2;
        leds[(5 * side) + (setSize - 1)].setHSV(0, 0, 1);
        leds[((2 + (3 * side)) * 3) - (setSize - 1)].setHSV(0, 0, 1);
        for (int side = 0; side < 4; side++) {
          leds[2 + (7 * side)].setHSV(0, 0, 1);
          leds[4 + (7 * side)].setHSV(0, 0, 1);
        }
      }
      blinkTarget(60);
    }
    if (targetSlot > setSize) {
      if (on) {
        int targLed = targetSlot - 1;
        int side = targLed / 2;
        leds[targLed + (5 * side)].setHSV(0, 0, 0);
        leds[((2 + (3 * side)) * 3) - targLed].setHSV(0, 0, 0);
      }
      blinkTarget(300);
    }
  }
  if (stage == 1) colorWheel(0);
  if (stage == 2) colorWheel(1);
  if (stage == 3) colorWheel(2);
  if (stage == 4) colorWheel(3);
}

void colorWheel(int layer) {
  int hue = 0, sat = 255, val = 170;
  if (layer == 0) {
    for (int color = 0; color < 16; color++) {
      int side = color / 4;
      hue = color * 16;
      if (color >= 0 + 4 * side && color <= 3 + side * 4) {
        leds[(side * 3) + color].setHSV(hue, sat, val);
        leds[((side * 11) + 6) - color].setHSV(hue, sat, val);
      }
    }
  }
  if (layer == 1) {
    for (int shade = 0; shade < 4; shade++) {
      hue = (shade * 16) + (64 * colorZone);
      for (int band = 0; band < 7; band++)leds[band + (7 * shade)].setHSV(hue, sat, val);
    }
  }
  if (layer == 2) {
    for (int fade = 0; fade < 4; fade++) {
      sat = 255 - (85 * fade);
      for (int band = 0; band < 7; band++)leds[band + (7 * fade)].setHSV(selectedHue, sat, val);
    }
  }
  if (layer == 3) {
    for (int bright = 0; bright < 4; bright ++) {
      val = 255 - (85 * bright);
      if (bright == 2) val = 120;
      for (int band = 0; band < 7; band++)leds[band + (7 * bright)].setHSV(selectedHue, selectedSat, val);
    }
  }
  if (on) {
    if (layer == 0)for (int band = 0; band < 7; band++)leds[band + (7 * targetZone)].setHSV(0, 0, 0);
    if (layer == 1)for (int band = 0; band < 7; band++)leds[band + (7 * targetHue)].setHSV(0, 0, 0);
    if (layer == 2)for (int band = 0; band < 7; band++)leds[band + (7 * targetSat)].setHSV(0, 0, 0);
    if (layer == 3)for (int band = 0; band < 7; band++)leds[band + (7 * targetVal)].setHSV(0, 0, 0);
    if (layer == 3 && targetVal == 3) for (int band = 0; band < 7; band++)leds[band + (7 * targetVal)].setHSV(0, 0, 1);
  }
  blinkTarget(300);
}

//void openPatterns() {
//  mainClock = millis();
//  if (mainClock - prevTime > 100) {
//    for (int a = 0; a < 28; a++) {
//      leds[a].setHSV(0, 0, 110);
//    }
//    if (on) {
//      clearAll();
//    }
//    on = !on;
//    prevTime = mainClock;
//  }
//}

void patternSelect() {
  mainClock = millis();
  patterns(patNum);
}

void modeSharing() {
  if (sharing) shareMode();
  else if (!sharing) receiveMode();
}

void chooseBrightness() {
  int newBrightness;
  clearAll();
  for (int q = 0; q < 4; q++) {
    if (brightVal == 0) {
      newBrightness = 3;
      leds[7 * (q) + 0].setHSV(0, 0, 255);
      leds[7 * (q) + 6].setHSV(0, 0, 255);
    }
    if (brightVal == 1) {
      newBrightness = 20;
      leds[7 * (q) + 1].setHSV(0, 0, 255);
      leds[7 * (q) + 5].setHSV(0, 0, 255);
    }
    if (brightVal == 2) {
      newBrightness = 35;
      leds[7 * (q) + 2].setHSV(0, 0, 255);
      leds[7 * (q) + 4].setHSV(0, 0, 255);
    }
    if (brightVal == 3) {
      newBrightness = 50;
      leds[7 * (q) + 3].setHSV(0, 0, 255);
    }
  }
  brightness = newBrightness;
}

void chooseDemoSpeed() {
  clearAll();
  if (on) {
    for (int q = 0; q < 4; q++) {
      if (newDemoSpeed == 0) {
        leds[7 * (q) + 0].setHSV(190, 255, 255);
        leds[7 * (q) + 6].setHSV(190, 255, 255);
      }
      if (newDemoSpeed == 1) {
        leds[7 * (q) + 1].setHSV(190, 255, 255);
        leds[7 * (q) + 5].setHSV(190, 255, 255);
      }
      if (newDemoSpeed == 2) {
        leds[7 * (q) + 2].setHSV(190, 255, 255);
        leds[7 * (q) + 4].setHSV(190, 255, 255);
      }
      if (newDemoSpeed == 3) {
        leds[7 * (q) + 3].setHSV(190, 255, 255);
      }
    }
  }
  blinkTarget(100 * newDemoSpeed + 100);
}

void restoreDefaults() {
  if (restore) {
    if (on) {
      hue = 0, sat = 0, val = 0;
      setLeds(0, 27);
    }
    if (!on) {
      hue = 0, sat = 255, val = 175;
      setLeds(0, 27);
    }
    blinkTarget(100);
  }
  if (!restore) {
    if (on) {
      hue = 0, sat = 255, val = 80;
      setLeds(0, 27);
    }
    if (!on) {
      hue = 0, sat = 0, val = 0;
      setLeds(0, 27);
    }
    blinkTarget(200);
  }
}

void confirmBlink() {
  mainClock = millis();
  if (mainClock - prevTime > 50) {
    if (frame == 0) clearAll();
    if (frame == 1) sat = 0, val = 175, setLeds(0, 27);
    if (frame == 2) clearAll();
    if (frame == 3) frame = 0, mode[m].menuNum = 0;
    frame++;
    prevTime = mainClock;
  }
}

void menuRing(int buttonNum) {
  clearAll();
  int menuHue, menuSat;
  if (menuSection == 0) {
    if (buttonNum == 0) menuHue = 0, menuSat = 0;
    if (buttonNum == 1) menuHue = 60, menuSat = 255;
  }
  if (menuSection == 1) {
    if (buttonNum == 0) menuHue = 20, menuSat = 255;
    if (buttonNum == 1) menuHue = 190, menuSat = 255;
  }
  if (menuSection == 2) {
    if (buttonNum == 0) menuHue = 160, menuSat = 255;
    if (buttonNum == 1) menuHue = 0, menuSat = 255;
  }
  if (menuSection == 3) {
    if (buttonNum == 0) menuHue = 120, menuSat = 255;
  }

  for (int q = 0; q < 4; q++) {
    if (button[buttonNum].holdTime > 1000 * menuSection) {
      leds[7 * (q) + 0].setHSV(menuHue, menuSat, 110);
      leds[7 * (q) + 6].setHSV(menuHue, menuSat, 110);
    }
    if (button[buttonNum].holdTime > 1250 + 1000 * menuSection) {
      leds[7 * (q) + 1].setHSV(menuHue, menuSat, 110);
      leds[7 * (q) + 5].setHSV(menuHue, menuSat, 110);
    }
    if (button[buttonNum].holdTime > 1500 + 1000 * menuSection) {
      leds[7 * (q) + 2].setHSV(menuHue, menuSat, 110);
      leds[7 * (q) + 4].setHSV(menuHue, menuSat, 110);
    }
    if (button[buttonNum].holdTime > 1750 + 1000 * menuSection) {
      leds[7 * (q) + 3].setHSV(menuHue, menuSat, 110);
    }
  }
}

//Buttons
// button[0] is outer button
// button[1] is inner button
//---------------------------------------------------------

void checkButton() {
  for (int b = 0; b < 2; b++) {
    button[b].buttonState = digitalRead(button[b].pinNum);
    if (button[b].buttonState == LOW && button[b].lastButtonState == HIGH && (millis() - button[b].pressTime > 200)) {
      button[b].pressTime = millis();
    }
    button[b].holdTime = (millis() - button[b].pressTime);
    if (button[b].holdTime > 50) {
      //---------------------------------------Button Down-----------------------------------------------------
      if (button[b].buttonState == LOW && button[b].holdTime > button[b].prevHoldTime) {
        if (b == 0) {
          if (button[b].holdTime > 1000 && button[b].holdTime <= 2000 && menu == 0 && !demoMode) mode[m].menuNum = 1, menuSection = 0;
          if (button[b].holdTime > 2000 && button[b].holdTime <= 3000 && menuSection == 0) menuSection = 1;
          if (button[b].holdTime > 3000 && button[b].holdTime <= 4000 && menuSection == 1) menuSection = 2;
          if (button[b].holdTime > 4000 && button[b].holdTime <= 5000 && menuSection == 2) menuSection = 3;
          if (button[b].holdTime > 5000 && menuSection == 3) mode[m].menuNum = 5;
        }
        if (b == 1) {
          if (button[b].holdTime > 1000 && button[b].holdTime <= 2000 && menu == 0 && !demoMode) mode[m].menuNum = 2, menuSection = 0;
          if (button[b].holdTime > 2000 && button[b].holdTime <= 3000 && menuSection == 0) menuSection = 1;
          if (button[b].holdTime > 3000 && button[b].holdTime <= 4000 && menuSection == 1) menuSection = 2;
          if (button[b].holdTime > 4000 && menuSection == 2) mode[m].menuNum = 8;
        }
      }//------------------------------------------------------------------------------------------------------
      //========================================Button Up======================================================
      if (button[b].buttonState == HIGH && button[b].lastButtonState == LOW && millis() - button[b].prevPressTime > 150) {
        if (menu == 0 && !demoMode) {
          if (button[b].holdTime <= 300) {
            if (b == 0) m++, frame = 0, gap = 0; //, throwMode();
            if (b == 1) m--, frame = 0, gap = 0;
          }
          if (button[b].holdTime > 300 && Serial) exportSettings();
        }
        // press in demo mode
        if (menu == 0 && demoMode) {
          if (button[b].holdTime <= 3000) {
            if (b == 0) saveAll(), frame = 0, mode[m].currentColor = 0;
            if (b == 1) tempLoad(), frame = 0, mode[m].currentColor = 0;
            mode[m].menuNum = 9;
            demoMode = false;
          }
        }
        if (menu == 1) {
          if (button[0].holdTime > 1000 && button[b].holdTime <= 2000) {
            if (b == 0) demoMode = true, frame = 0, mode[m].menuNum = 0, demoTime = mainClock, tempSave(), rollColors();
          }
          if (button[0].holdTime > 2000 && button[b].holdTime <= 3000) {
            if (b == 0) mode[m].menuNum = 3;
          }
          if (button[0].holdTime > 3000 && button[b].holdTime <= 4000) {
            if (b == 0) mode[m].menuNum = 4, mode[m].currentColor = 0;
          }
          if (button[0].holdTime > 4000) {
            if (b == 0) mode[m].menuNum = 5;
          }
        }
        if (menu == 2) {
          if (button[b].holdTime > 1000 &&  button[b].holdTime <= 2000) {
            if (b == 1) {
              mode[m].menuNum = 6;
              prevBrightness = brightness;
              if (prevBrightness == 3)brightVal = 0;
              if (prevBrightness == 20)brightVal = 1;
              if (prevBrightness == 35)brightVal = 2;
              if (prevBrightness == 50)brightVal = 3;
            }
          }
          if (button[b].holdTime > 2000 && button[b].holdTime <= 3000) {
            if (b == 1) mode[m].menuNum = 7; newDemoSpeed = demoSpeed;
          }
          if (button[b].holdTime > 3000) {
            if (b == 1) mode[m].menuNum = 8;
          }
        }
        if (menu == 3) {
          if (button[b].holdTime <= 300) {
            if (b == 0) {
              if (stage == 0) targetSlot++;
              if (stage == 1) targetZone++;
              if (stage == 2) targetHue++;
              if (stage == 3) targetSat++;
              if (stage == 4) targetVal++;
            }
            if (b == 1) {
              if (stage == 0) targetSlot--;
              if (stage == 1) targetZone--;
              if (stage == 2) targetHue--;
              if (stage == 3) targetSat--;
              if (stage == 4) targetVal--;
            }
          }
          if (button[b].holdTime > 300 && button[b].holdTime < 3000) {
            if (b == 0) {
              if (stage == 0) {
                int setSize = mode[m].numColors;
                if (targetSlot < setSize) stage = 1, currentSlot = targetSlot; //choose slot
                if (targetSlot == setSize && mode[m].numColors > 1)targetSlot--, mode[m].numColors--; // delete slot
                if (targetSlot > setSize)stage = 1, currentSlot = setSize;  //add slot
              }
              else if (stage == 1) stage = 2, colorZone = targetZone;
              else if (stage == 2) stage = 3, selectedHue = (targetHue * 16) + (colorZone * 64);
              else if (stage == 3) stage = 4, selectedSat = 255 - (85 * targetSat);
              else if (stage == 4) {
                selectedVal = 255 - (85 * targetVal);
                if (targetVal == 2) selectedVal = 120;
                mode[m].saveColor(currentSlot, selectedHue, selectedSat, selectedVal);
                stage = 0;
              }
            }
            if (b == 1) {
              if (stage == 0)mode[m].currentColor = 0, saveAll(), mode[m].menuNum = 0;//cancle exit
              if (stage == 1)stage = 0;
              if (stage == 2)stage = 1;
              if (stage == 3)stage = 2;
              if (stage == 4)stage = 3;
            }
          }
        }
        if (menu == 4) {
          if (button[b].holdTime <= 300) {
            if (b == 0)patNum++, frame = 0, mode[m].currentColor = 0;
            if (b == 1)patNum--, frame = 0, mode[m].currentColor = 0;
          }
          if (button[b].holdTime > 300 && button[b].holdTime < 3000) {
            mode[m].menuNum = 9;
            if (b == 0) mode[m].patternNum = patNum, saveAll(), frame = 0;//confirm selection
            if (b == 1) if (menu == 4) frame = 0;//cancle exit
          }
        }
        if (menu == 5) {
          if (button[b].holdTime <= 500) sharing = !sharing;
          if (button[b].holdTime > 500 && button[b].holdTime < 3000) sharing = true, mode[m].menuNum = 9;
        }
        if (menu == 6) {
          if (button[b].holdTime <= 300) {
            if (b == 0) brightVal++;
            if (b == 1) brightVal--;
          }
          if (button[b].holdTime > 300 && button[b].holdTime < 3000) {
            mode[m].menuNum = 9;
            if (b == 0) saveAll();
            if (b == 1) brightness = prevBrightness;
          }
        }
        if (menu == 7) {
          if (button[b].holdTime <= 300) {
            if (b == 0) newDemoSpeed++;
            if (b == 1) newDemoSpeed--;
          }
          if (button[b].holdTime > 300 && button[b].holdTime < 3000) {
            if (b == 0) demoSpeed = newDemoSpeed; demoTime = mainClock; saveAll();
            mode[m].menuNum = 9;
          }
        }
        if (menu == 8) {
          if (button[b].holdTime <= 300)restore = !restore;
          if (button[b].holdTime > 300 && button[b].holdTime < 3000) {
            mode[m].menuNum = 9;
            if (b == 0) {
              if (restore) {
                setDefaults();
                saveAll();
                frame = 0;
                mode[m].currentColor = 0;
              }
            }
          }
        }
        //if (button[b].holdTime < 4000 && menu == 9)mode[m].menuNum = 7;
        button[b].prevPressTime = millis();
      }//======================================================================================================
    }

    //these are the max and minimum values for each variable.
    if (newDemoSpeed > 3) newDemoSpeed = 0;
    if (newDemoSpeed < 0) newDemoSpeed = 3;
    if (brightVal > 3) brightVal = 0;
    if (brightVal < 0) brightVal = 3;
    if (patNum > totalPatterns - 1) patNum = 0;
    if (patNum < 0) patNum = totalPatterns - 1;
    int lastSlot = mode[m].numColors + 1;
    if (mode[m].numColors == 8) lastSlot = mode[m].numColors;
    if (targetSlot > lastSlot) targetSlot = 0;
    if (targetSlot < 0) targetSlot = lastSlot;
    if (targetZone > 3) targetZone = 0;
    if (targetZone < 0) targetZone = 3;
    if (targetHue > 3) targetHue = 0;
    if (targetHue < 0) targetHue = 3;
    if (targetSat > 3) targetSat = 0;
    if (targetSat < 0) targetSat = 3;
    if (targetVal > 3) targetVal = 0;
    if (targetVal < 0) targetVal = 3;
    if (m < 0)m = totalModes - 1;
    if (m > totalModes - 1)m = 0;
    button[b].lastButtonState = button[b].buttonState;
    button[b].prevHoldTime = button[b].holdTime;
  }
}

int tempH[8], tempS[8], tempV[8], tempNumColors, tempPatternNum;

void tempSave() {
  tempPatternNum = mode[m].patternNum;
  tempNumColors = mode[m].numColors;
  for (int e = 0; e < tempNumColors; e++) {
    tempH[e] = mode[m].hue[e];
    tempS[e] = mode[m].sat[e];
    tempV[e] = mode[m].val[e];
  }
}

void tempLoad() {
  mode[m].patternNum = tempPatternNum;
  mode[m].numColors = tempNumColors;
  for (int e = 0; e < tempNumColors; e++) {
    mode[m].hue[e] = tempH[e];
    mode[m].sat[e] = tempS[e];
    mode[m].val[e] = tempV[e];
  }
}


//Saving/Loading
//---------------------------------------------------------

void loadSave() {
  Orbit myOrbit;
  myOrbit = saveData.read();
  if (myOrbit.dataIsStored == true) {
    for (int modes = 0; modes < totalModes; modes ++) {
      mode[modes].patternNum = myOrbit.sPatternNum[modes];
      mode[modes].numColors = myOrbit.sNumColors[modes];
      for (int c = 0; c < mode[modes].numColors; c++) {
        mode[modes].hue[c] = myOrbit.sHue[modes][c];
        mode[modes].sat[c] = myOrbit.sSat[modes][c];
        mode[modes].val[c] = myOrbit.sVal[modes][c];
      }
      brightness = myOrbit.brightness;
      demoSpeed = myOrbit.demoSpeed;
    }
  }
}
void saveAll() {
  Orbit myOrbit;
  for (int modes = 0; modes < totalModes; modes ++) {
    myOrbit.sPatternNum[modes] = mode[modes].patternNum;
    myOrbit.sNumColors[modes] = mode[modes].numColors;
    for (int c = 0; c < mode[modes].numColors; c++) {
      myOrbit.sHue[modes][c] = mode[modes].hue[c];
      myOrbit.sSat[modes][c] = mode[modes].sat[c];
      myOrbit.sVal[modes][c] = mode[modes].val[c];
    }
    myOrbit.brightness = brightness;
    myOrbit.demoSpeed = demoSpeed;
  }
  myOrbit.dataIsStored = true;
  saveData.write(myOrbit);
}

//IR commuinication
//---------------------------------------------------------
void shareMode() {
  //confirmBlink();
  if (on) {
    for (int d = 0; d < 28; d++) leds[d].setHSV(128, 255, 100);
  }
  if (!on) {
    for (int d = 0; d < 28; d++) leds[d].setHSV(0, 0, 0);
  }
  mainClock = millis();
  if (mainClock - prevTime > 20) {
    on = !on;
    prevTime = mainClock;
  }
  if (mainClock - prevTime2 > 1000) {
    unsigned long shareBit;
    unsigned long sb1, sb2;
    for (int s = 0; s < 8; s++) {
      Serial.print(mode[m].hue[s]);
      Serial.print(" ");
    }
    Serial.println();
    shareBit = (((unsigned long)mode[m].hue[0] / 16 ) * (unsigned long)0x10000000) +
               (((unsigned long)mode[m].hue[1] / 16 ) * (unsigned long)0x1000000) +
               (((unsigned long)mode[m].hue[2] / 16 ) * (unsigned long)0x100000) +
               (((unsigned long)mode[m].hue[3] / 16 ) * (unsigned long)0x10000) +
               (((unsigned long)mode[m].hue[4] / 16 ) * (unsigned long)0x1000) +
               (((unsigned long)mode[m].hue[5] / 16 ) * (unsigned long)0x100) +
               (((unsigned long)mode[m].hue[6] / 16 ) * (unsigned long)0x10) +
               (unsigned long)1;
    /*mode[m].hue[0]*/
    //Serial.println(shareBit);
    Serial.println(shareBit, HEX);

    sb1 = shareBit;
    int sendSat[8];
    int sendVal[8];
    for (int n = 0; n < 8; n++) {
      if (mode[m].sat[n] >= 0 && mode[m].sat[n] < 85) sendSat[n] = 0;
      if (mode[m].sat[n] >= 85 && mode[m].sat[n] < 170) sendSat[n] = 1;
      if (mode[m].sat[n] >= 170 && mode[m].sat[n] < 255)sendSat[n] = 2;
      if (mode[m].sat[n] == 255)sendSat[n] = 3;

      if (mode[m].val[n] >= 0 && mode[m].val[n] < 85) sendVal[n] = 0;
      if (mode[m].val[n] >= 85 && mode[m].val[n] < 170) sendVal[n] = 1;
      if (mode[m].val[n] >= 170 && mode[m].val[n] < 255)sendVal[n] = 2;
      if (mode[m].val[n] == 255)sendVal[n] = 3;
      //Serial.print(sendVal[n]);
      //Serial.print(" ");
    }
    //Serial.println();
    shareBit = ((unsigned long)mode[m].hue[7] * (unsigned long)0x1000000) +
               ((unsigned long)((sendSat[0] * 0x4) + sendSat[1]) * (unsigned long)0x1000000) +
               ((unsigned long)((sendSat[2] * 0x4) + sendSat[3]) * (unsigned long)0x100000) +
               ((unsigned long)((sendSat[4] * 0x4) + sendSat[5]) * (unsigned long)0x10000) +
               ((unsigned long)((sendSat[6] * 0x4) + sendSat[7]) * (unsigned long)0x1000) +
               ((unsigned long)((sendVal[0] * 0x4) + sendVal[1]) * (unsigned long)0x100) +
               ((unsigned long)((sendVal[2] * 0x4) + sendVal[3]) * (unsigned long)0x10) +
               (unsigned long)2;
    Serial.println(shareBit, HEX);
    sb2 = shareBit;

    shareBit = ((unsigned long)((sendVal[4] * 0x4) + sendVal[5]) * (unsigned long)0x10000000) +
               ((unsigned long)((sendVal[6] * 0x4) + sendVal[7]) * (unsigned long)0x1000000) +
               ((unsigned long)mode[m].patternNum * (unsigned long)0x10000) +
               ((unsigned long)mode[m].numColors * (unsigned long)0x1000) +
               (unsigned long)3;
    Serial.println(shareBit, HEX);
    mySender.send(NEC, sb1, 0);
    mySender.send(NEC, sb2, 0);
    mySender.send(NEC, shareBit, 0);
    prevTime2 = mainClock;
  }
}

void receiveMode() {
  if (on) {
    for (int d = 0; d < 28; d++) leds[d].setHSV(128, 255, 100);
  }
  if (!on) {
    for (int d = 0; d < 28; d++) leds[d].setHSV(0, 0, 0);
  }
  blinkTarget(750);
  myReceiver.enableIRIn();
  if (myReceiver.getResults()) {
    myDecoder.decode();           //Decode it
    if (myDecoder.value != 0) {
      //myDecoder.dumpResults(false);  //Now print results. Use false for less detail
      unsigned long value = myDecoder.value;
      if (hexValue(0, value) == 1) {
        for (int f = 0; f < 8; f++) {
          data1[f] = hexValue(f, value);
          received1 = true;
        }
        Serial.println("data1");
      }
      if (hexValue(0, value) == 2) {
        for (int f = 0; f < 8; f++) {
          data2[f] = hexValue(f, value);
          received2 = true;
        }
        Serial.println("data2");
      }
      if (hexValue(0, value) == 3) {
        for (int f = 0; f < 8; f++) {
          data3[f] = hexValue(f, value);
          received3 = true;
        }
        Serial.println("data3");
      }
      //for (int f = 0; f < 8; f++) {
      //  Serial.print(hexValue(f, value), HEX);
      //  Serial.print(" ");
      //}
      //Serial.println();
      //Serial.println(value);
      Serial.println(value, HEX);
      myReceiver.enableIRIn();      //Restart receiver
    }
  }
  if (received1 && received2 && received3) {
    Serial.println("data received");
    mode[m].hue[0] = data1[7] * 16;
    mode[m].hue[1] = data1[6] * 16;
    mode[m].hue[2] = data1[5] * 16;
    mode[m].hue[3] = data1[4] * 16;
    mode[m].hue[4] = data1[3] * 16;
    mode[m].hue[5] = data1[2] * 16;
    mode[m].hue[6] = data1[1] * 16;
    mode[m].hue[7] = data2[7] * 16;
    mode[m].sat[0] = (data2[6] / 4) * 85;
    mode[m].sat[1] = (data2[6] % 4) * 85;
    mode[m].sat[2] = (data2[5] / 4) * 85;
    mode[m].sat[3] = (data2[5] % 4) * 85;
    mode[m].sat[4] = (data2[4] / 4) * 85;
    mode[m].sat[5] = (data2[4] % 4) * 85;
    mode[m].sat[6] = (data2[3] / 4) * 85;
    mode[m].sat[7] = (data2[3] % 4) * 85;
    mode[m].val[0] = (data2[2] / 4) * 85;
    mode[m].val[1] = (data2[2] % 4) * 85;
    mode[m].val[2] = (data2[1] / 4) * 85;
    mode[m].val[3] = (data2[1] % 4) * 85;
    mode[m].val[4] = (data3[7] / 4) * 85;
    mode[m].val[5] = (data3[7] % 4) * 85;
    mode[m].val[6] = (data3[6] / 4) * 85;
    mode[m].val[7] = (data3[6] % 4) * 85;
    mode[m].patternNum = (data3[5] * 16) + data3[4];
    mode[m].numColors = data3[3];
    received1 = false;
    received2 = false;
    received3 = false;
    mode[m].currentColor = 0;
    mode[m].menuNum = 0;
    saveAll();
  }
}

unsigned long hexValue(int place, unsigned long number) {
  for (int p = 0; p < place; p++) number /= 0x10;
  return number % 0x10;
}

void throwMode() {
  unsigned long shareBit;
  shareBit = (m * (unsigned long)0x10000000) +
             (0 * (unsigned long)0x1000000) +
             (0 * (unsigned long)0x100000) +
             (0 * (unsigned long)0x10000) +
             (0 * (unsigned long)0x1000) +
             (0 * (unsigned long)0x100) +
             (0 * (unsigned long)0x10) +
             (unsigned long)4;
  mySender.send(NEC, shareBit, 0);
}

void catchMode() {
  myReceiver.enableIRIn();
  if (myReceiver.getResults()) {
    myDecoder.decode();           //Decode it
    if (myDecoder.value != 0) {
      //myDecoder.dumpResults(false);  //Now print results. Use false for less detail
      unsigned long value = myDecoder.value;
      if (hexValue(0, value) == 4) {
        m = hexValue(7, value);
        Serial.println("Mode Change Caught");
      }
      myReceiver.enableIRIn();      //Restart receiver
    }
  }
}

//Serial Mode Transfer (usb)
//---------------------------------------------------------

void exportSettings() {
  Serial.println("Each line below contains 1 mode, copy and paste them to the line above to upload it!");
  for (int mo = 0; mo < totalModes; mo++) {
    Serial.print("<");
    Serial.print(mo);
    Serial.print(", ");
    Serial.print(mode[mo].patternNum);
    Serial.print(", ");
    Serial.print(mode[mo].numColors);
    Serial.print(", ");
    for (int co = 0; co < 8; co++) {
      Serial.print(mode[mo].hue[co]);
      Serial.print(", ");
      Serial.print(mode[mo].sat[co]);
      Serial.print(", ");
      Serial.print(mode[mo].val[co]);
      if (co != 7) Serial.print(", ");
    }
    Serial.println(">");
  }
}

void checkSerial() {
  recvWithStartEndMarkers();
  importData();
}

void recvWithStartEndMarkers() {
  static boolean recvInProgress = false;
  static byte ndx = 0;
  char startMarker = '<';
  char endMarker = '>';
  char rc;

  if (Serial.available() > 0 && newData == false) {
    rc = Serial.read();
    if (recvInProgress == true) {
      if (rc != endMarker) {
        receivedChars[ndx] = rc;
        ndx++;
        if (ndx >= numChars) {
          ndx = numChars - 1;
        }
      }
      else {
        receivedChars[ndx] = '\0'; // terminate the string
        recvInProgress = false;
        ndx = 0;
        newData = true;
      }
    }

    else if (rc == startMarker) {
      recvInProgress = true;
    }
  }
}

void importData() {
  bool dataIsValid = false;
  char * strtokIndx; // this is used by strtok() as an index
  if (newData == true) {
    newData = false;
    if (!dataIsValid) {
      strcpy(tempChars, receivedChars);
      strtokIndx = strtok(tempChars, ",");
      if (atoi(strtokIndx) >= totalModes) {
        Serial.println("Invalid input. Mode number: too high");
        return;
      }
      strtokIndx = strtok(NULL, ",");
      if (atoi(strtokIndx) >= totalPatterns) {
        Serial.println("Invalid input. Pattern number: too high");
        return;
      }
      strtokIndx = strtok(NULL, ",");
      if (atoi(strtokIndx) < 1) {
        Serial.println("Invalid input. Number of colors: too low");
        return;
      }
      if (atoi(strtokIndx) > 8) {
        Serial.println("Invalid input. Number of colors: too high");
        return;
      }
      for (int col = 0; col < 8; col++) {
        strtokIndx = strtok(NULL, ",");
        if (atoi(strtokIndx) > 255) {
          Serial.println("Invalid input. Hue " + (String)col + ": too high");
          return;
        }
        strtokIndx = strtok(NULL, ",");
        if (atoi(strtokIndx) > 255) {
          Serial.println("Invalid input. Saturation " + (String)col + ": too high");
          return;
        }
        strtokIndx = strtok(NULL, ",");
        if (atoi(strtokIndx) > 255) {
          Serial.println("Invalid input. Brightness " + (String)col + ": too high");
          return;
        }
      }
      dataIsValid = true;
    }
    if (dataIsValid) {
      importMode(receivedChars);
      Serial.println("Data recieved");
      Serial.println(receivedChars);
      saveAll();
      dataIsValid = false;
    }
  }
}

// Comma separated list of 27 numbers (3 settings + 3 * 8 colors):
// Mode Num, Pattern Num, Num Colors, Color1 H, Color1 S, Color1 V, Color2 H..... Color8 V
void importMode(char input[]) {
  char* strtokIndx;
  strcpy(tempChars, input);
  strtokIndx = strtok(tempChars, ",");
  int mNum = atoi(strtokIndx);
  strtokIndx = strtok(NULL, ",");
  mode[mNum].patternNum = atoi(strtokIndx);
  strtokIndx = strtok(NULL, ",");
  mode[mNum].numColors = atoi(strtokIndx);
  for (int col = 0; col < 8; col++) {
    strtokIndx = strtok(NULL, ",");
    mode[mNum].hue[col] = atoi(strtokIndx);
    strtokIndx = strtok(NULL, ",");
    mode[mNum].sat[col] = atoi(strtokIndx);
    strtokIndx = strtok(NULL, ",");
    mode[mNum].val[col] = atoi(strtokIndx);
  }
}

//Default Modes
//---------------------------------------------------------

void setDefaults() {
  brightness = 20;
  demoSpeed = 2;
  importMode("0, 13, 8, 0, 255, 170, 32, 255, 170, 64, 255, 170, 96, 255, 170, 128, 255, 170, 160, 255, 170, 192, 255, 170, 224, 255, 170");
  importMode("1, 9, 3, 48, 255, 170, 128, 255, 85, 224, 255, 85, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0");
  importMode("2, 19, 2, 96, 255, 170, 224, 255, 85, 64, 255, 120, 160, 255, 120, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0");
  importMode("3, 16, 3, 16, 255, 255, 128, 255, 85, 160, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0");
  importMode("4, 12, 4, 96, 255, 85, 128, 255, 255, 160, 255, 170, 192, 255, 85, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0");
  importMode("5, 5, 4, 0, 255, 120, 160, 255, 170, 64, 255, 170, 96, 255, 170, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0");
  importMode("6, 21, 7, 32, 255, 85, 80, 85, 85, 32, 0, 255, 48, 170, 255, 96, 0, 255, 80, 0, 85, 208, 0, 255, 0, 0, 0");
  importMode("7, 20, 4, 16, 255, 85, 96, 255, 170, 192, 255, 255, 0, 255, 170, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0");
  importMode("8, 2, 3, 192, 255, 85, 48, 255, 255, 80, 255, 85, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0");
  importMode("9, 10, 3, 80, 255, 85, 160, 255, 255, 0, 255, 170, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0");
  importMode("10, 2, 5, 240, 170, 0, 240, 255, 85, 240, 255, 255, 128, 255, 170, 128, 255, 255, 160, 255, 85, 192, 255, 170, 224, 255, 85");
  importMode("11, 19, 3, 144, 0, 0, 144, 0, 255, 96, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0");
  importMode("12, 1, 4, 112, 255, 85, 160, 255, 170, 240, 255, 255, 64, 255, 170, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0");
  importMode("13, 8, 5, 16, 255, 0, 144, 255, 0, 16, 255, 85, 144, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0");
}

