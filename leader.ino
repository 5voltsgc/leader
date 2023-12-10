/*LEADER
   Jeff Watts  9/30/2023
   This program reads a load cell and logs that to a SD card at a 60Hz rate
   The photo resitor helps to determine if the logger is inside a pipe during a pull test

    Version 11/2/2023 - Added the 20K Load cell calibration and zero offset

    
 * */
#include <math.h>
#include <LiquidCrystal.h>
#include <TimeLib.h>
#include <Wire.h>
#include "SparkFun_Qwiic_Scale_NAU7802_Arduino_Library.h"  // Click here to get the library: http://librarymanager/All#SparkFun_NAU7802
#include <SD.h>
#include <SPI.h>

NAU7802 myScale;  //Create instance of the NAU7802 class

//Setup the Buttons using the Bounce2 library for debounce control
#include <ezButton.h>
ezButton LOG_BUTTON(10);  // create ezButton object that attach to pin ;


// Define Global Varibles
bool newdataLCD = true;  // Used to update LCD only on interval elapsed time
char filename[50];       // Used to hold file name
int state = 0;           // 0 = idle, 1 = logging

long maxReading = 0;                 // used to keep track of max reading for update LCD
long currentReading = 0;             // used to keep track ofcurrent scale reading for update LCD
long currentPounds = 0;
long previousMaxReading = 0;         // Used to see if has changed since last write to LCD
long previousAverage = 0;            // Used to see if has changed since last write to LCD
String statusMessage = " ";          // used to infor user of any messages 16 charcter or less at this time
String previousStatusMessage = " ";  // Used to see if has changed since last write to LCD
int previousPhoteResPercent = 0;     // Used to see if has changed since last write to LCD

String previousLoadCellCapacity = "";
bool writeHeader = true;             // used to write headers to LCD the first time
const int numReadings = 64;          // used for smoothing the current readings
int readings[numReadings];           // the readings from the analog input
int readIndex = 0;                   // the index of the current reading
int total = 0;                       // the running total
int average = 0;                     // the average

int DB9_pin2 = 21;                   // used for identifing the connected load cell
int DB9_pin6 = 22;                   // When these pins are tied to ground, it indicates a LOW
int DB9_pin7 = 23;



//https://www.tutorialspoint.com/structs-in-arduino-program
struct LoadCell {
  String capacity;
  long zeroOffset;
  long calibrationFactor;
};

LoadCell connectedLoadCell = {"?K", 0, 0};

const int chipSelect = BUILTIN_SDCARD;

const int loggingLEDPin = 9;
const int photocellPinA = 0;    // the cell and 10K pulldown are connected to a0
const int photocellPinB = 1;    // the cell and 10K pulldown are connected to a1
int photocellReadingA;     // the analog reading from the sensor divider
int photocellReadingB;     // the analog reading from the sensor divider



// Start LCD
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//Delay Functions currentMillis, previousMillis, interval as arrays
const byte NUM_DELAYS = 3;                               // Number of delay timers used: 0 for Samples per second, 1 LCD update, 2 Blinking lED for logging,
unsigned long previousMillis[NUM_DELAYS] = { 0, 0, 0 };  // used for delay without delay functions
unsigned long currentMillis[NUM_DELAYS] = { 0, 0, 0 };   // used for delay without delay functions
unsigned int interval[NUM_DELAYS] = { 12, 550, 500};        // 0 ForSamples Per Second, 1 for LCD update to Current, 2 for Blinking LED


/* setup load cell array
  Background: The IC merely outputs the raw data from a load cell. For example, the
  output may be 25776 and change to 43122 when a cup of tea is set on the scale.
  These values are unitless - they are not grams or ounces. Instead, it is a
  linear relationship that must be calculated. Remeber y = mx + b?
  If 25776 is the 'zero' or tare state, and 43122 when I put 15.2oz of tea on the
  scale, then what is a reading of 57683 in oz?

  (43122 - 25776) = 17346/15.2 = 1141.2 per oz
  (57683 - 25776) = 31907/1141.2 = 27.96oz is on the scale

  https://arduino.stackexchange.com/questions/80236/initializing-array-of-structs
*/

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  pinMode(loggingLEDPin, OUTPUT);
  Wire.begin();
  if (myScale.begin() == false) {
    Serial.println("Scale not detected. Please check wiring. Freezing...");
    while (1)
      ;
  }
  Serial.println("Scale detected!");
  myScale.setSampleRate(NAU7802_SPS_80);  //Increase to max sample rate
  // myScale.setCalibrationFactor(long  unknown value);
  myScale.setZeroOffset(7850);
  myScale.calibrateAFE();  //Re-cal analog front end when we change gain, sample rate, or channel

  // set the Time library to use Teensy 3.0's RTC to keep time - Serial Terminal
  // Format is a capital letter T followed by unix epoch time - as in "T1689100936"  July 11, 2023
  setSyncProvider(getTeensy3Time);

  LOG_BUTTON.setDebounceTime(50);  // set debounce time to 50 milliseconds

  // initialize all the readings to 0 for the smoothing function
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;
  }
  //  setup the automatic identifing of the connected load cell with internal pull-up resistor
  pinMode(DB9_pin2, INPUT_PULLUP);
  pinMode(DB9_pin6, INPUT_PULLUP);
  pinMode(DB9_pin7, INPUT_PULLUP);
}

void loop() {
  update_buttons();
  logging();
  updateLCD();
  readScale();
  readPhotoResistor();
  connected_load_cell();
}
