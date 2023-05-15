//#include <Wire.h>
//#include "LiquidCrystal_I2C.h"
//#include <LiquidCrystal_I2C.h>
#include <LiquidCrystal.h>
#include "BigNums2x2.h"
#include "TimeLib.h"
#include <iarduino_RTC.h>
#include <EncButton2.h>
//EncButton2<EB_ENCBTN> enc(INPUT_PULLUP, 2, 3, 4);  // энкодер с кнопкой//blue,red,white
EncButton2<EB_ENCBTN> enc(INPUT_PULLUP, 10, 11, 12);  // энкодер с кнопкой//blue,red,white

#define TIME_HEADER       'T'   // Header tag for serial time sync message

#define HOUR              'H'
#define MIN               'I'
#define SEC               'S'
#define DAY               'D'
#define MONTH             'M'
#define YEAR              'Y'

#define BRIGHTNESS_HEADER 'B'
#define CONTRAST_HEADER   'C'
unsigned long currentTime;
unsigned long syncTime;
unsigned long backlightTimeout;
unsigned long unixTime;

iarduino_RTC rtc(RTC_DS3231);
const int rs = 8, en = 9, d4 = 4, d5 = 5, d6 = 6, d7 = 7;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
#define LCD_CONTRAST_PIN 13
#define LCD_BACKLIGHT_PIN 12

int8_t brightness_Value = 40;
int8_t contrast_Value = 40;

BigNums2x2 bigNum(TREK);
//byte arrow[8] = {0x04, 0x02, 0x09, 0x02, 0x04, 0x00, 0x00, 0x00};
//byte dd[8] = {0x0E, 0x0A, 0x0E, 0x00, 0x0E, 0x0A, 0x0E, 0x00};
byte s3[8] = {0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00};
byte s2[8] = {0x00, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00};
byte s1[8] = {0x00, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00};
byte s0[8] = {0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00};

boolean backLT_state = 1;
boolean editMode = 0;
boolean editH = 0;
boolean editI = 0;
boolean editS = 0;
boolean editD = 0;
boolean editM = 0;
boolean editY = 0;
boolean editBrightness = 0;
boolean editContrast = 0;
boolean customFont = 0;
uint8_t cFontNum = 0;

char FontStr[5] = {'\0'};
const char *FontList[NUMFONTS] = {"NASA", "TRON", "TREK", "SERF"};

//int8_t h_Value = hour();
//int8_t i_Value = minute();
//int8_t s_Value = second();
//int8_t d_Value = day();
//int8_t m_Value = month();
//int16_t y_Value = year() - 2000;
int8_t h_Value;
int8_t i_Value;
int8_t s_Value;
int8_t d_Value;
int8_t m_Value;
int16_t y_Value;
const char* monthArray[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};


void setup() {
  rtc.begin();
  //  setTime(rtc.Unix);
  Serial.begin(9600);
  while (!Serial);  // Wait for Arduino Serial Monitor to open
  //  if (timeStatus() != timeSet) {
  //    Serial.println("Unable to sync with the RTC");
  //  } else {
  //    Serial.println("RTC has set the system time");
  //  }
  Serial.println(rtc.Unix);
  Serial.println(rtc.gettime("d-M-Y, H:i:s"));
  ///////////////////////////////////////////////////////////
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LCD_BACKLIGHT_PIN, OUTPUT);
  pinMode(LCD_CONTRAST_PIN, OUTPUT);
  setSyncProvider(requestSync);  //set function to call when sync required
  setSyncInterval(100);
  unsigned long unixTime = rtc.gettimeUnix();
  setTime(unixTime);
  Serial.print(dayShortStr(weekday()));
  backlightTimeout = millis();
  lcd.begin(16, 2);
  analogWrite(LCD_BACKLIGHT_PIN, brightness_Value); //set backlight on
  analogWrite(LCD_CONTRAST_PIN, contrast_Value); //set some contrast
  lcd.display();
  setDefChars();
  printDotsAndSpaces();
  drawClockValues();
  drawDateValues();
}

void loop() {
  /////////////////////////
  enc.tick();                       // опрос происходит здесь

  if (millis() - currentTime > 1000) {
    currentTime = millis();

    if (Serial.available() > 2) { // wait for at least two characters
      char c = Serial.read();
      Serial.print(" - c - ");
      Serial.println(c);
      if ( c == TIME_HEADER) {
        processSyncMessage();
      } else if (c == BRIGHTNESS_HEADER) {
        processBrightnessMessage();
      } else if (c == CONTRAST_HEADER) {
        processContrastMessage();
      }
    }
    ///////////////
    if (editMode == 0) {
      //      editH = 0;
      //      editI = 0;
      //      editS = 0;
      //      editD = 0;
      //      editM = 0;
      //      editY = 0;
      //      editBrightness = 0;
      //      editContrast = 0;
      digitalWrite(LED_BUILTIN, HIGH);
      if (customFont == 0) {
        drawClockValues();
        drawDateValues();
      } else {
        //        bigNum.print(hour(), 2, 0);
        //        bigNum.print(minute(), 2, 6);
        //        bigNum.print(second(), 2, 12);
        rtc.gettime();
        bigNum.print(rtc.Hours, 2, 1);
        bigNum.print(rtc.minutes, 2, 6);
        bigNum.print(rtc.seconds, 2, 11);
      }
    }
    ////////////////
  }
  if (millis() - currentTime > 10) digitalWrite(LED_BUILTIN, LOW);
  if (millis() - syncTime > 600000) {
    syncTime = millis();
    Serial.println("unix sync");
    rtc.gettime();
    //    unixTime = rtc.gettimeUnix();
    //    //    Serial.println(rtc.Unix);
    //    Serial.println(unixTime);
    //    //    setTime(rtc.Unix);
    //    setTime(unixTime);
  }
  if (millis() - backlightTimeout > 30000) {
    if (backLT_state == 1) {
      backLT_state = 0;
      Serial.print("backLT_state - ");
      Serial.println(backLT_state);        //        lcd.noBacklight();
      analogWrite(LCD_BACKLIGHT_PIN, 0); //set backlight on
      lcd.noDisplay();
    }
  }
  //////////////////////////////////////////
  //////////////////////////////////////////
  //////////////////////////////////////////
  if (enc.held()) {
    if (backLT_state == 1) {
      if (editMode == 0) {

        resetEditStates();
        setDefChars();
        lcd.clear();
        printDotsAndSpaces();
        drawClockValues();
        drawDateValues();
        //        h_Value = hour();
        //        i_Value = minute();
        //        s_Value = second();
        //        d_Value = day();
        //        m_Value = month();
        //        y_Value = year() - 2000;
        updateDateValues();
        Serial.println("Edit Mode");
        Serial.println("Edit Hours");
        editMode = 1;
        editH = 1;
        lcd.setCursor(3, 0);
        lcd.write(126);
      } else if (editMode == 1) {
        //        customFont = 0;
        //        lcd.clear();
        //////////////////////////////////////////////////////////////
        rtc.settime(s_Value, i_Value, h_Value, d_Value, m_Value, y_Value);
        //////////////////////////////////////////////////////////////
        //        unixTime = rtc.gettimeUnix();
        //        setTime(unixTime);
        //        Serial.println(unixTime);
        resetEditStates();
        if (customFont == 0) {
          printDotsAndSpaces();
          drawClockValues();
          drawDateValues();
        } else {
          lcd.clear();
          bigNum.font(cFontNum);
          strcpy(FontStr, FontList[cFontNum]);
          Serial.println(FontStr);
        }

        editMode = 0;
        Serial.println("Exit from Edit Mode");
      }
    }
  }
  //////////////////////////////////////////
  //////////////////////////////////////////
  //////////////////////////////////////////
  if (enc.hasClicks(1)) {
    if (editMode == 0) {
      if (backLT_state == 1) {
        backLT_state = 0;
        Serial.print("backLT_state - ");
        Serial.println(backLT_state);        //        lcd.noBacklight();
        analogWrite(LCD_BACKLIGHT_PIN, 0); //set backlight on
        lcd.noDisplay();
      } else {
        backLT_state = 1;
        backlightTimeout = millis();
        Serial.print("backLT_state - ");
        Serial.println(backLT_state);        //        lcd.backlight();
        analogWrite(LCD_BACKLIGHT_PIN, brightness_Value); //set backlight on
        lcd.display();
      }
    } else if (editMode == 1) {
      if (editH == 1) {
        printDotsAndSpaces();
        lcd.setCursor(6, 0);
        lcd.write(126);
        editH = 0;
        editI = 1;
        Serial.println("editMode - switching to minutes");
      } else if (editI == 1) {
        printDotsAndSpaces();
        lcd.setCursor(9, 0);
        lcd.write(126);
        editI = 0;
        editS = 1;
        Serial.println("editMode - switching to seconds");
      } else if (editS == 1) {
        printDotsAndSpaces();
        lcd.setCursor(4, 1);
        lcd.write(126);
        editS = 0;
        editD = 1;
        Serial.println("editMode - switching to days");
      } else if (editD == 1) {
        printDotsAndSpaces();
        lcd.setCursor(7, 1);
        lcd.write(126);
        editD = 0;
        editM = 1;
        Serial.println("editMode - switching to month");
      } else if (editM == 1) {
        printDotsAndSpaces();
        lcd.setCursor(11, 1);
        lcd.write(126);
        editM = 0;
        editY = 1;
        Serial.println("editMode - switching to year");
      } else if (editY == 1) {
        printDotsAndSpaces();
        editY = 0;
        editMode = 0;
        rtc.settime(s_Value, i_Value, h_Value, d_Value, m_Value, y_Value);
        //        unixTime = rtc.gettimeUnix();
        //        Serial.println(unixTime);
        //        setTime(unixTime);
        drawClockValues();
        drawDateValues();
        Serial.println("editMode - exit from editMode");
      } else if (editBrightness == 1) {
        editBrightness = 0;
        editContrast = 1;
        updateDispSettings();
        Serial.println("editContrast");
      } else if (editContrast == 1) {
        editContrast = 0;
        editBrightness = 1;
        updateDispSettings();
      }
    }
  }

  if (enc.hasClicks(2)) {
    if (backLT_state == 1) {
      if (editMode == 1) {
        if (editBrightness == 0 && editContrast == 0) {
          resetEditStates();
          editBrightness = 1;
          lcd.clear();
          updateDispSettings();
        } else {
          resetEditStates();
          setDefChars();
          editH = 1;
          //          h_Value = hour();
          //          i_Value = minute();
          //          s_Value = second();
          //          d_Value = day();
          //          m_Value = month();
          //          y_Value = year() - 2000;
          updateDateValues();
          Serial.println("Edit Mode");
          Serial.println("Edit Hours");
          printDotsAndSpaces();
          drawClockValues();
          drawDateValues();
          lcd.setCursor(3, 0);
          lcd.write(126);
        }

      } else {

        customFont = 1;
        lcd.clear();
        cFontNum++;
        cFontNum %= 4;
        bigNum.font(cFontNum);
        strcpy(FontStr, FontList[cFontNum]);
        Serial.println(FontStr);
        //        if (cFontNum > 3)cFontNum = 0;
      }
    }
  }
  //////////////////////////////////////////
  //////////////////////////////////////////
  //////////////////////////////////////////
  if (enc.turn() ) {
    if (editMode == 1) {
      if (editH == 1) {
        h_Value += enc.dir();
        if (h_Value < 0)  h_Value = 23;
        h_Value %= 24;
        Serial.print("h_Value - ");
        Serial.println(h_Value);
        lcd.setCursor(4, 0);
        if (h_Value < 10) lcd.print("0");
        lcd.print(h_Value);
      }
      if (editI == 1) {
        i_Value += enc.dir();
        if (i_Value < 0)  i_Value = 59;
        i_Value %= 60;
        Serial.print("i_Value - ");
        Serial.println(i_Value);
        lcd.setCursor(7, 0);
        if (i_Value < 10) lcd.print("0");
        lcd.print(i_Value);
      }
      if (editS == 1) {
        s_Value += enc.dir();
        if (s_Value < 0)  s_Value = 59;
        s_Value %= 60;
        Serial.print("s_Value - ");
        Serial.println(s_Value);
        lcd.setCursor(10, 0);
        if (s_Value < 10) lcd.print("0");
        lcd.print(s_Value);
      }
      if (editD == 1) {
        d_Value += enc.dir();
        if (d_Value < 1)  d_Value = 31;
        if (d_Value > 31)  d_Value = 1;
        Serial.print("d_Value - ");
        Serial.println(d_Value);
        lcd.setCursor(5, 1);
        if (d_Value < 10) lcd.print("0");
        lcd.print(d_Value);
      }
      if (editM == 1) {
        m_Value += enc.dir();
        if (m_Value < 1)  m_Value = 12;
        if (m_Value > 12)  m_Value = 1;
        Serial.print("m_Value - ");
        Serial.println(monthArray[m_Value - 1]);
        lcd.setCursor(8, 1);
        lcd.print(monthArray[m_Value - 1]);
      }
      if (editY == 1) {
        y_Value += enc.dir();
        if (y_Value < 0)  y_Value = 99;
        if (y_Value > 99)  y_Value = 0;
        Serial.print("y_Value - ");
        Serial.println(y_Value);
        lcd.setCursor(12, 1);
        lcd.print("20");
        if (y_Value < 10) lcd.print("0");
        lcd.print(y_Value);
      }
      if (editBrightness == 1) {
        brightness_Value += enc.dir();
        if (brightness_Value < 0)  brightness_Value = 0;
        if (brightness_Value > 99)  brightness_Value = 99;
        updateDispSettings();
      }
      if (editContrast == 1) {
        contrast_Value += enc.dir();
        if (contrast_Value < 0)  contrast_Value = 0;
        if (contrast_Value > 99)  contrast_Value = 99;
        updateDispSettings();
      }
    }
  }
  //////////////////////////////////////////
  //////////////////////////////////////////
  //////////////////////////////////////////
}

void updateDateValues() {
  rtc.gettime();
  h_Value = rtc.Hours;
  i_Value = rtc.minutes;
  s_Value = rtc.seconds;
  d_Value = rtc.day;
  m_Value = rtc.month;
  y_Value = rtc.year;
}

void setDefChars() {
  lcd.createChar(1, s0);
  lcd.createChar(2, s1);
  lcd.createChar(3, s2);
  lcd.createChar(4, s3);
  //  lcd.createChar(5, arrow);
  //  lcd.createChar(6, dd);
}

void updateDispSettings() {
  if (editBrightness == 1 || editContrast == 1) {
    if (editBrightness == 1) {
      lcd.setCursor(10, 1);
      lcd.print(" ");
      lcd.setCursor(10, 0);
      lcd.write(126);
    }
    if (editContrast == 1) {
      lcd.setCursor(10, 0);
      lcd.print(" ");
      lcd.setCursor(10, 1);
      lcd.write(126);
    }
    lcd.setCursor(1, 0);
    lcd.print("bright");
    lcd.setCursor(1, 1);
    lcd.print("contrast");
    lcd.setCursor(12, 0);
    if (brightness_Value < 10) lcd.print("0");
    lcd.print(brightness_Value);
    analogWrite(LCD_BACKLIGHT_PIN, brightness_Value);
    lcd.setCursor(12, 1);
    int8_t val = 99 - contrast_Value;
    if (val < 10) lcd.print("0");
    lcd.print(val);
    analogWrite(LCD_CONTRAST_PIN, contrast_Value);
  }
}
void resetEditStates() {
  editH = 0;
  editI = 0;
  editS = 0;
  editD = 0;
  editM = 0;
  editY = 0;
  editBrightness = 0;
  editContrast = 0;
}

void printDotsAndSpaces() {
  lcd.setCursor(0, 0);
  lcd.write(4);
  lcd.write(3);
  lcd.write(2);
  lcd.write(1);
  lcd.setCursor(12, 0);
  lcd.write(1);
  lcd.write(2);
  lcd.write(3);
  lcd.write(4);
  //  lcd.print("    ");
  lcd.setCursor(6, 0);
  lcd.write(3);
  //  lcd.print(":");
  lcd.setCursor(9, 0);
  lcd.write(3);
  //  lcd.print(":");
  //  lcd.setCursor(0, 1);
  //  lcd.write(0);
  //  lcd.print("_");
  lcd.setCursor(3, 1);
  lcd.write(2);
  lcd.write(1);
  //  lcd.print("_");
  lcd.setCursor(7, 1);
  lcd.write(1);
  //  lcd.print("_");
  lcd.setCursor(11, 1);
  lcd.write(1);
  //  lcd.print("_");
}
void drawClockValues() {
  rtc.gettime();
  //********************
  //**    12:34:56    **
  //**                **
  //********************
  //
  //  Serial.println(rtc.Unix);
  //  lcd.setCursor(0, 0);
  //  lcd.write(second());
  // digital clock display of the time
  ////////////////////////////////////
  lcd.setCursor(4, 0);
  //  if (hour() < 10) lcd.print("0");
  //  lcd.print(hour());
  if (rtc.Hours < 10) lcd.print("0");
  lcd.print(rtc.Hours);
  lcd.setCursor(7, 0);
  //  if (minute() < 10) lcd.print("0");
  //  lcd.print(minute());
  if (rtc.minutes < 10) lcd.print("0");
  lcd.print(rtc.minutes);
  lcd.setCursor(10, 0);
  //  if (second() < 10) lcd.print("0");
  //  lcd.print(second());
  if (rtc.seconds < 10) lcd.print("0");
  lcd.print(rtc.seconds);
}
//////////////////////////////////////
void drawDateValues() {
  lcd.setCursor(0, 1);
  lcd.print(dayShortStr(weekday()));
  lcd.setCursor(5, 1);
  if (rtc.day < 10) lcd.print("0");
  lcd.print(rtc.day);
  lcd.setCursor(8, 1);
  lcd.print(monthArray[rtc.month - 1]);
  //  lcd.print(monthShortStr(month()));
  lcd.setCursor(12, 1);
  lcd.print("20");
  if (rtc.year < 10) lcd.print("0");
  lcd.print(rtc.year);
  ///////////////////////////////////////
}

void processSyncMessage() {
  int8_t value;
  char c = Serial.read();
  Serial.println(c);
  switch (c) {
    case HOUR:
      value = Serial.parseInt();
      rtc.settime(-1, -1, value);
      Serial.println("HOUR");
      Serial.println(value);
      break;
    case MIN:
      value = Serial.parseInt();
      rtc.settime(-1, value);
      Serial.println("MIN");
      Serial.println(value);
      break;
    case SEC:
      value = Serial.parseInt();
      rtc.settime( value);
      Serial.println("SEC");
      Serial.println(value);
      break;
    case DAY:
      value = Serial.parseInt();
      rtc.settime(-1, -1, -1, value);
      Serial.println("DAY");
      Serial.println(value);
      break;
    case MONTH:
      value = Serial.parseInt();
      rtc.settime(-1, -1, -1, -1, value);
      Serial.println("MONTH");
      Serial.println(value);
      break;
    case YEAR:
      value = Serial.parseInt();
      rtc.settime(-1, -1, -1, -1, -1, value);
      Serial.println("YEAR");
      Serial.println(value);
      break;
    default:
      // выполнить, если val ни 1 ни 2
      // default опционален
      break;
  }
  //  unixTime = rtc.gettimeUnix();
  //  Serial.println(unixTime);
  //  setTime(unixTime);
}


void processBrightnessMessage() {
  //  int8_t value;
  brightness_Value = Serial.parseInt();
  Serial.println(brightness_Value);
  analogWrite(LCD_BACKLIGHT_PIN, brightness_Value); //set backlight brightness
}
void processContrastMessage() {
  //  unsigned long value;
  //  int8_t value;
  contrast_Value = Serial.parseInt();
  Serial.println(contrast_Value);
  analogWrite(LCD_CONTRAST_PIN, contrast_Value); //set some contrast
}
time_t requestSync() {
  return 0; // the time will be sent later in response to serial mesg
}
