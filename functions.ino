void updateLCD() {
  /*
     This function will do the folowing:
     1. The first time through this function write the headers in the LCD
     2. Update the LCD every interval[1] elapsed time
     3. Update each changed field
     4.

  */
  // 1. Write the headers to the LCD
  if (writeHeader) {
    lcd.clear();
    lcd.setCursor(0, 0);  // Coloumn, Row
    lcd.print("C");
    lcd.setCursor(8, 0);  // Coloumn, Row
    lcd.print("M");
    lcd.setCursor(12, 1);
    lcd.print("000%");
    writeHeader = false;
  }
  //  2. Update only on elapsed interval[1] time
  currentMillis[1] = millis();
  if (currentMillis[1] - previousMillis[1] >= interval[1]) {
    previousMillis[1] = currentMillis[1];

    // Current weight Field col= 1-7 row=0
    //TODO update so it only updates if greater than X% changed to make it not seem so jumpy
    if (previousAverage != average) {
      lcd.setCursor(1, 0);   // Coloumn, Row
      lcd.print("       ");  // erase previous value to get rid of any character that won't be over written
      lcd.setCursor(1, 0);   // Coloumn, Row
      lcd.print(currentPounds);
      previousAverage = currentPounds;
    }

    // MAX weight Field col= 9-15 row=0
    // only update the MAX reading while in logging state
    if (previousMaxReading != maxReading && state == 1) {
      lcd.setCursor(9, 0);   // Coloumn, Row
      lcd.print("       ");  // erase previous value to get rid of any character that won't be over written
      lcd.setCursor(9, 0);   // Coloumn, Row
      lcd.print(maxReading);
      previousMaxReading = maxReading;
    };


    // Status Message Field col= 0-7 row=1
    if (previousStatusMessage != statusMessage) {
      lcd.setCursor(0, 1);            // Coloumn, Row
      lcd.print("        ");  // erase previous value to get rid of any character that won't be over written
      lcd.setCursor(0, 1);            // Coloumn, Row
      //      statusMessage = statusMessage + " " + String(photocellReadingB) + "%";
      lcd.print(statusMessage);
      previousStatusMessage = statusMessage;
    }

    // Update connected load cell
    if (previousLoadCellCapacity != connectedLoadCell.capacity) {
      lcd.setCursor(8, 1);            // Coloumn, Row
      lcd.print("   ");  // erase previous value to get rid of any character that won't be over written
      lcd.setCursor(8, 1);            // Coloumn, Row
      lcd.print(connectedLoadCell.capacity);
      previousLoadCellCapacity = connectedLoadCell.capacity;
    }




    // PhotoResitor Percent Field col= 12-14 row=1
    if (previousPhoteResPercent != photocellReadingB) {
      //3 digits to be printed
      String photoResStatus = "";
      if (photocellReadingB == 100) {
        photoResStatus = photocellReadingB;
      }
      // 1 digit to be printed - put 2 spaces beforephotocellReadingB to make 3 digits;
      else if (photocellReadingB < 10) {

        photoResStatus = "  " + String(photocellReadingB);
      }
      else {
        //2 digits to be printed - put 1 space beforephotocellReadingB to make 3 digits;
        photoResStatus = " " + String(photocellReadingB);
      }

      lcd.setCursor(12, 1);            // Coloumn, Row
      lcd.print(photoResStatus);
      previousPhoteResPercent = photocellReadingB;
    }
  }
}

void connected_load_cell() {
  // This function identifies the connected load cell based on the following table
  // For example:  the DB9 connector has the following connections:
  // Pin 1 = Load Cell RED/E+
  // PIN 2 = MCU Input D21
  // Pin 3 = Load Cell GREEN/A+
  // Pin 4 = Load Cell White/A-
  // Pin 5 = Ground
  // Pin 6 = MCU Input D22
  // Pin 7 = MCU Input D23
  // Pin 8 = Ground
  // Pin 9 = Load Cell Black/E-
  // To identify a 2k load cell short the pin 7 to ground (either pin 5 or 8)
  // Then use the pinMode(23, INPUT_PULLUP) in setup to activate the internal pull-up resitor

  // +-------+-------+-------+-----------+-----------+-----------+-------------------------+-------------+--------------------+
  // | Level | Pin 5 | Pin 8 | Pin 2/D21 | Pin 6/D22 | Pin 7/D23 |        Selected         | Zero Offset | Calibration Factor |
  // +-------+-------+-------+-----------+-----------+-----------+-------------------------+-------------+--------------------+
  // |     1 | GND   | GND   | HIGH      | HIGH      | HIGH      | No pins tied to ground  |             |                    |
  // |     2 | GND   | GND   | HIGH      | HIGH      | LOW       | 2K LBS                  | 114,854     | 3382               |
  // |     3 | GND   | GND   | HIGH      | LOW       | HIGH      | 10K LBS                 | 69,160      | 655                |
  // |     4 | GND   | GND   | HIGH      | LOW       | LOW       | 20K LBS                 | 10,865      | 340                |
  // |     5 | GND   | GND   | LOW       | HIGH      | HIGH      | Open                    |             |                    |
  // |     6 | GND   | GND   | LOW       | HIGH      | LOW       | Open                    |             |                    |
  // |     7 | GND   | GND   | LOW       | LOW       | HIGH      | Open                    |             |                    |
  // |     8 | GND   | GND   | LOW       | LOW       | LOW       | all pins tied to ground | N/A         | N/A                |
  // +-------+-------+-------+-----------+-----------+-----------+-------------------------+-------------+--------------------+




  // level 1 - no loadcell connected or not wired up to self identify
  if (digitalRead(DB9_pin2) == HIGH  && digitalRead(DB9_pin6) == HIGH && digitalRead(DB9_pin7) == HIGH) {
    connectedLoadCell = {"?K", 1, 1};
    //    Serial.println(connectedLoadCell.capacity);

  }
  //level 2 -  2K load cell connected
  if (digitalRead(DB9_pin2) == HIGH  && digitalRead(DB9_pin6) == HIGH && digitalRead(DB9_pin7) == LOW) {
    connectedLoadCell = {"2K", 114854, 3382};
    //    Serial.println(connectedLoadCell.capacity);
  }

  // Level 3 - 10K load cell
  if (digitalRead(DB9_pin2) == HIGH  && digitalRead(DB9_pin6) == LOW && digitalRead(DB9_pin7) == HIGH) {
    connectedLoadCell = {"10K", 69160, 655};
    //    Serial.println(connectedLoadCell.capacity);
  }
    // Level 4 - 20K load cell
  if (digitalRead(DB9_pin2) == HIGH  && digitalRead(DB9_pin6) == LOW && digitalRead(DB9_pin7) == LOW) {
    connectedLoadCell = {"20K", 10865, 340};
    //    Serial.println(connectedLoadCell.capacity);
  }
}

void update_buttons() {
  LOG_BUTTON.loop();  // MUST call the loop() function first
  int startBtnState = LOG_BUTTON.getState();

  if (startBtnState) {
    state = 1;
    newdataLCD = true;
    digitalWrite(loggingLEDPin, HIGH);
    statusMessage = "LOGGING";
  }
  else {
    state = 0;
    digitalWrite(loggingLEDPin, LOW);
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

  // create a string of the specifics of the load cell
  String loadCellData = String(connectedLoadCell.capacity) + ", " + String(connectedLoadCell.zeroOffset) + ", " + String(connectedLoadCell.calibrationFactor);
  
  if (state == 1) {
    update_filename();  // get a new filename based on time
    maxReading = 0;     // reset maxReading every time you enter this logging function
    // open file on SD card
    String dataString = "";
    if (!SD.begin(chipSelect)) {
      // did not find the SD card don't go into logging and alert to LCD
      statusMessage = "Check SD";
      updateLCD();
      state = 0;
      digitalWrite(loggingLEDPin, LOW);
      
    }

    File dataFile = SD.open(filename, FILE_WRITE);

    dataFile.println("Milliseconds, Current RAW, Pounds,  Photoresistor, Connected Load Cell, Zero Offset, Calibration Factor");


    while (state == 1) {
      update_buttons();
      updateLCD();  // this function will update the LCD with current info
      readPhotoResistor();
      currentMillis[0] = millis();
      if (currentMillis[0] - previousMillis[0] >= interval[0]) {
        // read scale
        previousMillis[0] = currentMillis[0];
        readScale();
        String dataString = String(millis()) + ", " + String(currentReading) + ", " + String(currentPounds) + ", " + String(photocellReadingB) + ", " + loadCellData;
        dataFile.println(dataString);
        if (maxReading < currentPounds) {
          maxReading = currentPounds;  // update max reading
        }
        newdataLCD = true;
      }
    }
    dataFile.close();
  }
}


void update_filename() {
  // This function grabs the current time creates a filename that is unique with a csv extension.
  // possibly could add which loadcell was selected as we currently have 3

  // https://github.com/PaulStoffregen/Time/blob/master/examples/TimeTeensy3/TimeTeensy3.ino  // Only use the TimeTeensy3.ino example for the teensy 4 RTC the other don't work
  // https://bigdanzblog.wordpress.com/2015/01/05/using-the-teensy-3-1-real-time-clock-rtc/ how to adjust for acuracy
  // https://github.com/PaulStoffregen/Time useage of other functions

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
  //  https://www.reddit.com/r/arduino/comments/smmuty/rounding_to_nearest_5_or_10/

  if (myScale.available() == true) {
    currentReading = myScale.getReading();

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
  currentPounds = (average - connectedLoadCell.zeroOffset) / connectedLoadCell.calibrationFactor;
  newdataLCD = true;
}

void readPhotoResistor() {
  // This functin will read the photoresistor and map them to a 0-100% brightness

  int currentReadingA = analogRead(photocellPinA);
  int currentReadingB = analogRead(photocellPinB);
  photocellReadingA = map(currentReadingA, 0, 1023, 0, 100);
  photocellReadingB = map(currentReadingB, 0, 1023, 0, 100);

}
