void updateLCD() {

  // This function will update any changed data to the LCD
  // Found that it takes about 200 microsceonds to set cursor and write one letter
  // The I2C LCD took 48ms to write 16 characters
  // The idea is to create varibles called prevoius and current
  // if they are different then set the cursor and update the LCD
  // this example will use the 1602 and not the 2004 screen as planned
  //
  // * the first time in this function write the headers
  // * write curent data
  // * store previous data to compare against new so that only changed data is updated
  if (writeHeader) {
    lcd.clear();
    lcd.setCursor(0, 0);  // Coloumn, Row
    lcd.print("C");
    lcd.setCursor(8, 0);  // Coloumn, Row
    lcd.print("M");
    writeHeader = false;
  }
  if (newdataLCD) {
    // only update the MAX reading while in logging state
    if (previousMaxReading != maxReading && state == 1) {
      lcd.setCursor(9, 0);   // Coloumn, Row
      lcd.print("       ");  // erase previous value to get rid of any character that won't be over written
      lcd.setCursor(9, 0);   // Coloumn, Row
      lcd.print(maxReading);
      previousMaxReading = maxReading;
    };

    if (previousStatusMessage != statusMessage) {
      lcd.setCursor(0, 1);            // Coloumn, Row
      lcd.print("                ");  // erase previous value to get rid of any character that won't be over written
      lcd.setCursor(0, 1);            // Coloumn, Row
      lcd.print(statusMessage);
      previousStatusMessage = statusMessage;
    }
    // only update the current after interval[1]
    currentMillis[1] = millis();
    if (currentMillis[1] - previousMillis[1] >= interval[1]) {
      // read scale
      previousMillis[1] = currentMillis[1];
      if (previousAverage != average) {
        lcd.setCursor(1, 0);   // Coloumn, Row
        lcd.print("       ");  // erase previous value to get rid of any character that won't be over written
        lcd.setCursor(1, 0);   // Coloumn, Row
        lcd.print(average);
        previousAverage = average;
      }
    }

    newdataLCD = false;
    // Serial.println(filename);
  }
}
void update_buttons() {
  START_BUTTON.loop();  // MUST call the loop() function first
  STOP_BUTTON.loop();   // MUST call the loop() function first
  int startBtnState = START_BUTTON.getState();
  int stopBtnState = STOP_BUTTON.getState();
  if (startBtnState) {
    state = 1;
    newdataLCD = true;
    digitalWrite(13, HIGH);
    statusMessage = "LOGGING";
  }
  if (stopBtnState) {
    state = 0;
    digitalWrite(13, LOW);
    statusMessage = "IDLE";
    newdataLCD = true;
  }
}

void logging() {
  // This function opens a file on the SD card, reads the scale value and writes to the file
  // until the stop button is pressed or the file SD card fills up.
  // This is is a blocking function so the main loop is not acted upon while in the logging state
  // * open file
  // * After interval time read scale sensor
  // * write to SD
  // * Update the MAX
  //

  if (state == 1) {
    update_filename();  // get a new filename based on time
    maxReading = 0;     // reset maxReading every time you enter this logging function
    // open file on SD card
    String dataString = "";
    if (!SD.begin(chipSelect)) {
      // did not find the SD card don't go into logging and alert to LCD
      statusMessage = "SD not found";
      state= 0;
    }

    File dataFile = SD.open(filename, FILE_WRITE);
    dataFile.println("Milliseconds, Current");
    while (state == 1) {
      update_buttons();
      updateLCD();  // this function will update the LCD with current info
      currentMillis[0] = millis();
      if (currentMillis[0] - previousMillis[0] >= interval[0]) {
        // read scale
        previousMillis[0] = currentMillis[0];
        readScale();
        String dataString = String(millis()) + ", " + String(currentReading);
        dataFile.println(dataString);
        if (maxReading < currentReading) {
          maxReading = currentReading;  // update max reading
        }
        newdataLCD = true;
      }
    }
    dataFile.close();
  }
}


void update_filename() {
  // This function grabs the current time creates a tile name that is unique with a csv extension.
  // possibly could add which loadcell was selected as we currently have 3

  // https://github.com/PaulStoffregen/Time/blob/master/examples/TimeTeensy3/TimeTeensy3.ino  // Only use the TimeTeensy3.ino example for the teensy 4 RTC the other don't work
  // https://bigdanzblog.wordpress.com/2015/01/05/using-the-teensy-3-1-real-time-clock-rtc/ how to adjust for acuracy
  // https://github.com/PaulStoffregen/Time useage of other functions
  // char *desc = "200lb";
  time_t t = now();   // grab the current time and assign to t
  int h = hour(t);    // returns the hour for the given time t
  int m = minute(t);  // returns the minute for the given time t
  int s = second(t);  // returns the second for the given time t
  int d = day(t);     // the day for the given time t
  int mo = month(t);  // the month for the given time t
  int y = year(t);    // the year for the given time t
  // assign these values to the dynamic filename
  snprintf(filename, 50, "%d-%d-%d-%d-%d-%d.csv", y, mo, d, h, m, s);  // create filname based on current time
}

time_t getTeensy3Time() {
  return Teensy3Clock.get();
}



/*  code to process time sync messages from the serial port   */
#define TIME_HEADER "T"  // Header tag for serial time sync message

unsigned long processSyncMessage() {
  unsigned long pctime = 0L;
  const unsigned long DEFAULT_TIME = 1357041600;  // Jan 1 2013

  if (Serial.find(TIME_HEADER)) {
    pctime = Serial.parseInt();
    return pctime;
    if (pctime < DEFAULT_TIME) {  // check the value is a valid time (greater than Jan 1 2013)
      pctime = 0L;                // return 0 to indicate that the time is not valid
    }
  }
  return pctime;
}

void readScale() {
  // This function will read the scale, and get a smoothed reading as well

  if (myScale.available() == true) {
    currentReading = myScale.getReading();
    // TODO convert this unitless number to pounds
    // TODO create an error message for when the scale is not connected

    // } else {
    // statusMessage = "scale not found";
  }

  // subtract the last reading:
  total = total - readings[readIndex];
  // read from the sensor:
  readings[readIndex] = currentReading;
  // add the reading to the total:
  total = total + readings[readIndex];
  // advance to the next position in the array:
  readIndex = readIndex + 1;

  // if we're at the end of the array...
  if (readIndex >= numReadings) {
    // ...wrap around to the beginning:
    readIndex = 0;
  }
  // calculate the average:
  average = total / numReadings;
  newdataLCD = true;
}
