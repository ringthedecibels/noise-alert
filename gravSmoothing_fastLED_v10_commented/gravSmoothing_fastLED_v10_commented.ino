/*  gravSmoothing_fastLED_v10_commented
 *  
 *  This system polls analog data (0-1023) of the environmental sound level using the Gravity Sound Level Meter,
 *  smooths the analog data over a 1 second period,
 *  converts the smoothed analog data into a decibel value (dBA),
 *  stores the decibel value in a 15 element array,
 *  updates an average dBA value of the 15-element array every 1 second,
 *  and uses the average sound level value to update a two-LED strip (WS2811/Neopixel) warning display every 5 seconds.
 *  
 *  Input: Gravity Sound Level Meter
 *  Output: LED strip (WS2811)
 *  Requires: FastLED library
 *  
 *  Ring the Decibels, SEED 2019, Rice University
 *  Last Modified: July 18, 2019
 */

#include <FastLED.h>

// initialize first LED strip variables
#define LED_PIN_1     5
#define NUM_LEDS_1    10
#define BRIGHTNESS_1  64
#define LED_TYPE_1    WS2811
#define COLOR_ORDER_1 GRB

// initialize second LED strip variables
#define LED_PIN_2     6
#define NUM_LEDS_2    10
#define BRIGHTNESS_2  64
#define LED_TYPE_2    WS2811
#define COLOR_ORDER_2 GRB

// initialize color arrays for both LED strips
CRGB leds1[NUM_LEDS_1];
CRGB leds2[NUM_LEDS_2];

// setup to control LED blinking
CRGB ledsSaved[NUM_LEDS_1];       // initialize color array to save a color pattern (for blinking)
int blinkIndex = 0;               // initialize index to time blinking
int blinkIndicator = 0;           // initialize blink indicator to determine when to blink

int micInputPin = A0;             // initialize microphone input pin

double VREF = 5.0;                // initialize input voltage

// initialize timers
unsigned long dbaMillis = 0;
unsigned long printMillis = 0;

// initialize timing windows
const int dataTime = 50;          // record analog data for 50ms = 20Hz
const int blinkTime = 500;        // blink every 500ms
const int dbaTime = 1000;         // calculate dba average over 1s
const int printTime = 5000;       // print dba average every 5s
const int smoothingTime = 15000;  // smooth over 15 seconds

// initialize maximum signal array values to record analog data from sound level meter
const int signalMaxArraySize = floor(dbaTime / dataTime);
double signalMaxArray[signalMaxArraySize];
int signalMaxIndex = 0;
double signalMaxTotal = 0;        // running total
double signalMaxAverage = 0;      // running average

// initialize dba array values to record the decibel value for every second
const int dbaArraySize = floor(smoothingTime / 1000);
double dbaArray[dbaArraySize];
int dbaIndex = 0;
double dbaTotal = 0;              // running total
double dbaAverage = 0;            // running average

void setup() {
  
  delay(3000); // power-up safety delay
  
  // initialize LED strips
  FastLED.addLeds<LED_TYPE_1, LED_PIN_1, COLOR_ORDER_1>(leds1, NUM_LEDS_1);
  FastLED.addLeds<LED_TYPE_2, LED_PIN_2, COLOR_ORDER_2>(leds2, NUM_LEDS_2);
  
  // initialize serial monitor
  Serial.begin(9600);
  Serial.println("Started");  
}

/* mainColors()
 * 
 * This functions determines the gradient pattern for the first LED strip.
 * The gradient level is divided into 10 sections based on the number of LEDs on the strip.
 * Each gradient level corresponds to a decibel level threshold.
 * The function uses the dbaAverage value to populate the leds1 array and then displays the array on the LED strip.
 * Additionally, for decibel levels greater than 76 dBA, the function sets the blinkIndicator to 1.
 * For decibel levels less than or equal to 76 dBA, the blinkIndicator is set to 0.
 * 
 * Input: dbaAverage
 * Output: first LED strip gradient pattern; blinkIndicator 
 */
 
void mainColors() {   
  
    Serial.println("Main Colors");

    // highest threshold = 82 dBA
    // gradient pattern: green, yellow, orange, red
    // set blinkIndicator = 1
    if (dbaAverage > 82) {
      for(int z = 0; z <= floor(0.2*NUM_LEDS_1); z++) 
        leds1[z] = CRGB::Green;
       
      for(int z = (floor(0.2*NUM_LEDS_1)+1); z <= floor(0.5*NUM_LEDS_1); z++)
        leds1[z] = CRGB::Yellow;  
        
      for(int z = (floor(0.5*NUM_LEDS_1)+1); z <= floor(0.7*NUM_LEDS_1); z++)
        leds1[z] = CRGB::Orange;
        
      for(int z = (floor(0.7*NUM_LEDS_1)+1); z < NUM_LEDS_1; z++)
        leds1[z] = CRGB::Red;
        
      blinkIndicator = 1;
    }

    // second threshold = 76 dBA
    // gradient pattern: green, yellow, orange, red
    // set blinkIndicator = 1
    else if (dbaAverage > 76) {
      for(int z = 0; z <= floor(0.2*NUM_LEDS_1); z++) 
        leds1[z] = CRGB::Green;
      
      for(int z = (floor(0.2*NUM_LEDS_1)+1); z <= floor(0.5*NUM_LEDS_1); z++)
        leds1[z] = CRGB::Yellow;
        
      for(int z = (floor(0.5*NUM_LEDS_1)+1); z <= floor(0.7*NUM_LEDS_1); z++)
        leds1[z] = CRGB::Orange;
        
      for(int z = (floor(0.7*NUM_LEDS_1)+1); z < (NUM_LEDS_1-floor(0.1*NUM_LEDS_1)); z++)
        leds1[z] = CRGB::Red;
        
      blinkIndicator = 1;
    }

    // highest threshold = 70 dBA
    // gradient pattern: green, yellow, orange
    // set blinkIndicator = 0
    else if (dbaAverage > 70) {
      for(int z = 0; z <= floor(0.2*NUM_LEDS_1); z++) 
        leds1[z] = CRGB::Green;
      
      for(int z = (floor(0.2*NUM_LEDS_1)+1); z <= floor(0.5*NUM_LEDS_1); z++)
        leds1[z] = CRGB::Yellow;
        
      for(int z = (floor(0.5*NUM_LEDS_1)+1); z <= floor(0.7*NUM_LEDS_1); z++)
        leds1[z] = CRGB::Orange;

      blinkIndicator = 0;
    }

    // highest threshold = 64 dBA
    // gradient pattern: green, yellow, orange
    // set blinkIndicator = 0
    else if (dbaAverage > 64) {
      for(int z = 0; z <= floor(0.2*NUM_LEDS_1); z++) 
        leds1[z] = CRGB::Green;
      
      for(int z = (floor(0.2*NUM_LEDS_1)+1); z <= floor(0.5*NUM_LEDS_1); z++)
        leds1[z] = CRGB::Yellow;
        
      for(int z = (floor(0.5*NUM_LEDS_1)+1); z <= (NUM_LEDS_1-floor(0.4*NUM_LEDS_1)); z++)
        leds1[z] = CRGB::Orange;
      
      blinkIndicator = 0;  
    }

    // highest threshold = 58 dBA
    // gradient pattern: green, yellow
    // set blinkIndicator = 0
    else if (dbaAverage > 58) {
      for(int z = 0; z <= floor(0.2*NUM_LEDS_1); z++) 
        leds1[z] = CRGB::Green;
      
      for(int z = (floor(0.2*NUM_LEDS_1)+1); z <= floor(0.5*NUM_LEDS_1); z++)
        leds1[z] = CRGB::Yellow;

      blinkIndicator = 0;  
    }

    // highest threshold = 52 dBA
    // gradient pattern: green, yellow
    // set blinkIndicator = 0
    else if (dbaAverage > 52) {
      for(int z = 0; z <= floor(0.2*NUM_LEDS_1); z++) 
        leds1[z] = CRGB::Green;
      
      for(int z = (floor(0.2*NUM_LEDS_1)+1); z <= (NUM_LEDS_1-floor(0.6*NUM_LEDS_1)); z++)
        leds1[z] = CRGB::Yellow;

      blinkIndicator = 0;
    }

    // highest threshold = 46 dBA
    // gradient pattern: green, yellow
    // set blinkIndicator = 0
    else if (dbaAverage > 46) {
      for(int z = 0; z <= floor(0.2*NUM_LEDS_1); z++) 
        leds1[z] = CRGB::Green;
      
      for(int z = (floor(0.2*NUM_LEDS_1)+1); z <= (NUM_LEDS_1-floor(0.7*NUM_LEDS_1)); z++)
        leds1[z] = CRGB::Yellow;

      blinkIndicator = 0;        
    }

    // highest threshold = 40 dBA
    // gradient pattern: green
    // set blinkIndicator = 0
    else if (dbaAverage > 40) {
      for(int z = 0; z <= floor(0.2*NUM_LEDS_1); z++) 
        leds1[z] = CRGB::Green;
      
      blinkIndicator = 0;
    }

    // highest threshold = 34 dBA
    // gradient pattern: green
    // set blinkIndicator = 0
    else if (dbaAverage > 34) {
      for(int z = 0; z <= (NUM_LEDS_1-floor(0.9*NUM_LEDS_1)); z++) 
        leds1[z] = CRGB::Green;
      
      blinkIndicator = 0;
    }

    // lowest threshold = < 34 dBA
    // gradient pattern: green
    // set blinkIndicator = 0
    else {
      leds1[0] = CRGB::Green;
      
      blinkIndicator = 0;  
    }

    // display the color array for the first LED strip
    FastLED[0].showLeds(BRIGHTNESS_1);
  }

void loop() {

  // reset the LED strips
  FastLED[0].clearLedData();
  FastLED[1].clearLedData();
  
  // every 1 second: calculate average signal
  if ((millis() - dbaMillis) >= dbaTime) {
    dbaMillis = millis();         // update dbaMillis

    unsigned int sample;          // initialize sample value from Gravity Sound Level Meter
    unsigned int signalMax = 0;   // initialize maximum signal value

    // poll analog data from Gravity Sound Level Meter over 50ms
    while ((millis() - dbaMillis) < dataTime) {

      // read input from Gravity Sound Level Meter
      sample = analogRead(micInputPin);
      
      if (sample < 1024) {        // throw out spurious readings
        if (sample > signalMax) {
          signalMax = sample;     // store sample as signalMax if greater than previous
        }
      }

      // update signalMax array, calculate total, calculate average, update index
      signalMaxTotal = signalMaxTotal - signalMaxArray[signalMaxIndex];   // subtract previous value from total
      signalMaxArray[signalMaxIndex] = signalMax;                         // replace previous value with new value in array
      signalMaxTotal = signalMaxTotal + signalMaxArray[signalMaxIndex];   // add new value to total
      signalMaxAverage = signalMaxTotal / signalMaxArraySize;             // calculate average
      signalMaxIndex++;                                                   // increment index
      
      // wrap signalMaxindex when end of signalMaxArray is reached
      if (signalMaxIndex >= signalMaxArraySize) {
        signalMaxIndex = 0;
      }
    }

    // control blinking
    // if mainColors() indicates blinking (red on gradient)
    if (blinkIndicator == 1) {
      
      // save current gradient pattern (full color gradient)
      for(int z = 0; z <= floor(0.2*NUM_LEDS_1); z++) 
        ledsSaved[z] = CRGB::Green;
       
      for(int z = (floor(0.2*NUM_LEDS_1)+1); z <= floor(0.5*NUM_LEDS_1); z++)
        ledsSaved[z] = CRGB::Yellow;  
        
      for(int z = (floor(0.5*NUM_LEDS_1)+1); z <= floor(0.7*NUM_LEDS_1); z++)
        ledsSaved[z] = CRGB::Orange;
        
      for(int z = (floor(0.7*NUM_LEDS_1)+1); z < NUM_LEDS_1; z++)
        ledsSaved[z] = CRGB::Red;

      blinkIndex++;               // increment blinkIndex
      
      // for every even-numbered pass of the 1 second loop when blinkIndicator = 1
      if ((blinkIndex % 2) == 0) {
        // populate first LED strip color array with black (no color)
        for (int i = 0; i < NUM_LEDS_1; i++) {
          leds1[i] = CRGB::Black;
        }
        // populate second LED strip color array with black (no color)
        for (int i = 0; i < NUM_LEDS_2; i++) {
          leds2[i] = CRGB::Black;
        }
        // display the color arrays for the LED strips
        FastLED[0].showLeds(BRIGHTNESS_1);
        FastLED[1].showLeds(BRIGHTNESS_2);
      }
      // for every odd-numbered pass of the 1 second loop when blinkIndicator = 1
      else {
        // populate first LED strip color array with the saved gradient color array
        for (int i = 0; i < NUM_LEDS_1; i++) {
          leds1[i] = ledsSaved[i];
        }
        // populate second LED strip color array with black (no color)
        for (int i = 0; i < NUM_LEDS_2; i++) {
          leds2[i] = CRGB::White;
        }
        // display the color arrays for the LED strips
        FastLED[0].showLeds(BRIGHTNESS_1);
        FastLED[1].showLeds(BRIGHTNESS_2);
      }
    }
    
    // if mainColors() indicates no blinking (no red on gradient, blinkIndicator = 0)
    else {
      // populate second LED strip color array with black (no color)
      for (int i = 0; i < NUM_LEDS_2; i++) {
          leds2[i] = CRGB::Black;
      }
      // display the color array for the second LED strip
      FastLED[1].showLeds(BRIGHTNESS_2);
    }
    

    // convert to voltSec using signalMaxAverage and signalMinAverage
    double voltSec = signalMaxAverage * (VREF / 1024.0);
 
    // convert to dbaSec using voltSec
    double dbaSec = voltSec * 50.0;

    // update dba array, calculate total, calculate average, update index
    dbaTotal = dbaTotal - dbaArray[dbaIndex];   // subtract previous value from total
    dbaArray[dbaIndex] = dbaSec;                // replace previous value with new value in array
    dbaTotal = dbaTotal + dbaArray[dbaIndex];   // add new value to total
    dbaAverage = dbaTotal / dbaArraySize;       // calculate average
    dbaIndex++;                                 // increment index
    
    // wrap dbaIndex when end of dbaArray is reached
    if (dbaIndex >= dbaArraySize) {
      dbaIndex = 0;
    }

    // every 5 seconds
    if ((millis() - printMillis) >= printTime) {
      printMillis = millis();         // update printMillis
      
      Serial.print("DBA Average: ");
      Serial.println(dbaAverage);

      // call mainColors() to use dbaAverage to determine and display LED gradient
      mainColors();
      // Serial.println("End");
    }
  }  
}
