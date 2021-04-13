 #include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "LowPower.h"

// POSSIBLE DISPLAY TYPES
#define DISPLAY_I2C_64X32 1
#define DISPLAY_SPI_128X64 2
// END OF POSSIBLE DISPLAY TYPES

// CONFIGURATION
// You need to adjust the following numbers to your needs
#define DISPLAY_TYPE DISPLAY_I2C_64X32 //Chose your display from the list above
#define DISPLAY_CONTRAST 20 // Display brightness from 0-255. Higher brightness consumes more power. Doesn't work with every display.
#define DISPLAY_ON_TIME SLEEP_1S
#define DISPLAY_OFF_TIME SLEEP_4S

#define HOT_SENSOR // Comment out if you don't have a hot water sensor
#define HOT_AIN A0 // Arduino pin for the hot thermistor (needs to be one of the analog pins beginning with A)
#define HOT_UPPER_THRESHOLD 590 // Highest value where running hot water is detected
#define HOT_LOWER_THRESHOLD 200 // Lowest value where running hot water is detected
#define COLD_SENSOR // Comment out if you don't have a cold water sensor
#define COLD_AIN A1 // Arduino pin for the cold thermistor (needs to be one of the analog pins beginning with A)
#define COLD_UPPER_THRESHOLD 950 // Highest value where running cold water is detected
#define COLD_LOWER_THRESHOLD 835 // Lowest value where running cold water is detected

#define DISPLAY_VCC_PIN 2
#define DISPLAY_GND_PIN 3

#define LIMIT_TIME 6L*3600L // Six hours limit
// END OF CONFIGURATION

#if (!defined HOT_SENSOR) && (!defined COLD_SENSOR)
  #error "HOT_SENSOR and/or COLD_SENSOR needs to be defined!"
#endif

#if !defined DISPLAY_TYPE
  #error "DISPLAY_TYPE needs to be defined!"
#endif

#if DISPLAY_TYPE > 2
  #error "Invalid DISPLAY_TYPE!"
#endif

#if DISPLAY_TYPE == DISPLAY_I2C_64X32
  U8G2_SSD1306_64X32_1F_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
#elif DISPLAY_TYPE == DISPLAY_SPI_128X64
  U8G2_SSD1306_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ 8, /* dc=*/ 9, /* reset=*/ 10);
#endif

#define MAX_MATCH_COUNT 5

#define THERMISTOR_READS 5
#define THERMISTOR_EXTRA_READS 2

#define THERMISTOR_TOTAL_READS THERMISTOR_READS + THERMISTOR_EXTRA_READS

#ifdef HOT_SENSOR
  long secondsSinceHot = 24L*60L*60L;
  short matchCountHot = 0;
#endif
#ifdef COLD_SENSOR
  long secondsSinceCold = 24L*60L*60L;
  short matchCountCold = 0;
#endif


void setup() {
  #ifdef HOT_SENSOR
    pinMode(HOT_AIN, INPUT_PULLUP);
  #endif
  #ifdef COLD_SENSOR
    pinMode(COLD_AIN, INPUT_PULLUP);
  #endif
  pinMode(DISPLAY_VCC_PIN, OUTPUT);
  pinMode(DISPLAY_GND_PIN, OUTPUT);
  digitalWrite(DISPLAY_VCC_PIN, HIGH);
  digitalWrite(DISPLAY_GND_PIN, LOW);
}

int sum(int arr[], int l) {
  int out = 0;
  for (int i=0;i<l;i++) {
    out += arr[i];
  }
  return out;
}

int average(int arr[], int l) {
  return sum(arr,l)/l;
}

int intCompare(const void * a, const void * b) {
   return ( *(int*)a - *(int*)b );
}

int getAverageRemovedExtremes(int arr[], int l, int rem) {
  qsort(arr, l, sizeof(int), intCompare);
  for (int i=0;i<rem;i++) {
    int av = average(arr, l-i);
    int lowDiff = av-arr[0];
    int highDiff = arr[l-i];
    if (lowDiff>highDiff) {
      for (int j=0;j<l-rem;j++) {
        arr[j] = arr[j+1];
      }
    }
  }
  return average(arr, l-rem);
}

void loop() {
  int readVals[THERMISTOR_TOTAL_READS];
  #ifdef HOT_SENSOR
    for (int i=0;i<THERMISTOR_TOTAL_READS;i++) {
      readVals[i] = analogRead(HOT_AIN);
    }
    int readValHot = getAverageRemovedExtremes(readVals, THERMISTOR_TOTAL_READS, THERMISTOR_EXTRA_READS);
    if (readValHot<HOT_UPPER_THRESHOLD && readValHot>HOT_LOWER_THRESHOLD) {
      matchCountHot++;
    } else {
      matchCountHot = 0;
    }
    if (matchCountHot >= MAX_MATCH_COUNT) {
      secondsSinceHot = 0;
      matchCountHot = 0;
    }
  #endif
  #ifdef COLD_SENSOR
    for (int i=0;i<THERMISTOR_TOTAL_READS;i++) {
      readVals[i] = analogRead(COLD_AIN);
    }
    int readValCold = getAverageRemovedExtremes(readVals, THERMISTOR_TOTAL_READS, THERMISTOR_EXTRA_READS);
    if (readValCold<COLD_UPPER_THRESHOLD && readValCold>COLD_LOWER_THRESHOLD) {
      matchCountCold++;
    } else {
      matchCountCold = 0;
    }
    if (matchCountCold >= MAX_MATCH_COUNT) {
      secondsSinceCold = 0;
      matchCountCold = 0;
    }
  #endif

  #if (defined HOT_SENSOR) && (defined COLD_SENSOR)
    long secondsSince = (secondsSinceCold > secondsSinceHot)? secondsSinceCold : secondsSinceHot;
  #elif (defined HOT_SENSOR)
    long secondsSince = secondsSinceHot;
  #elif (defined COLD_SENSOR)
    long secondsSince = secondsSinceCold;
  #endif

  u8g2.begin();
  u8g2.setContrast(DISPLAY_CONTRAST);
  u8g2.setFont(u8g2_font_inb19_mr );
  u8g2.clearBuffer();
  #if DISPLAY_TYPE == DISPLAY_I2C_64X32
    u8g2.setFont(u8g2_font_inb19_mr);
    u8g2.setCursor(0,19);
  #elif DISPLAY_TYPE == DISPLAY_SPI_128X64
    u8g2.setFont(u8g2_font_fub35_tf);
    u8g2.setCursor(0,35);
  #endif
  if (secondsSince<60) {
    u8g2.print(secondsSince);
    u8g2.print("s");
  } else if (secondsSince<3600L) {
    u8g2.print(secondsSince/60L);
    u8g2.print("m");
  } else {
    u8g2.print(secondsSince/3600L);
    u8g2.print("h");
  }
  u8g2.print(" ");
  if (secondsSince<LIMIT_TIME) {
    #if DISPLAY_TYPE == DISPLAY_I2C_64X32
      u8g2.print("O");
    #elif DISPLAY_TYPE == DISPLAY_SPI_128X64
      u8g2.drawLine(100, 20, 109, 33); // Drawing Checkmark
      u8g2.drawLine(100, 21, 109, 34);
      u8g2.drawLine(100, 22, 109, 35);
  
      u8g2.drawLine(109, 33, 124, 4);
      u8g2.drawLine(109, 34, 124, 5);
      u8g2.drawLine(109, 35, 124, 6);
    #endif
  } else {
    #if DISPLAY_TYPE == DISPLAY_I2C_64X32
      u8g2.print("X");
    #elif DISPLAY_TYPE == DISPLAY_SPI_128X64
      u8g2.drawStr(96,35,"X");
    #endif
  }

  #if DISPLAY_TYPE == DISPLAY_I2C_64X32
    u8g2.setFont(u8g2_font_t0_11_tf);
    u8g2.setCursor(0,32);
  #elif DISPLAY_TYPE == DISPLAY_SPI_128X64
    u8g2.setFont(u8g2_font_t0_22_tf);
    u8g2.setCursor(20,64);
  #endif
  #ifdef HOT_SENSOR
    u8g2.print(readValHot);
    if (secondsSinceHot>LIMIT_TIME) {
      u8g2.print("X");
    } else {
      u8g2.print(" ");
    }
  #endif
  #if (defined HOT_SENSOR) && (defined COLD_SENSOR)
    u8g2.print("  ");
  #endif
  #ifdef COLD_SENSOR
    u8g2.print(readValCold);
    if (secondsSinceCold>LIMIT_TIME) {
      u8g2.print("X");
    }
  #endif
  
  
  u8g2.sendBuffer();
  LowPower.powerDown(DISPLAY_ON_TIME, ADC_OFF, BOD_OFF);
  //u8g2.clearBuffer();
  //u8g2.sendBuffer();
  digitalWrite(DISPLAY_VCC_PIN, LOW);
  digitalWrite(DISPLAY_GND_PIN, LOW);
  LowPower.powerDown(DISPLAY_OFF_TIME, ADC_OFF, BOD_OFF);
  digitalWrite(DISPLAY_VCC_PIN, HIGH);
  digitalWrite(DISPLAY_GND_PIN, LOW);
  #ifdef HOT_SENSOR
    secondsSinceHot += 5;
  #endif
  #ifdef COLD_SENSOR
    secondsSinceCold += 5;
  #endif
}
