#include <Arduino.h>
// LCD connected according to Tutorial https://diyi0t.com/lcd-display-tutorial-for-arduino-and-esp8266/

// include the library code:
#include <Wire.h>
#include "LiquidCrystal.h"

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int RS = D2, EN = D3, d4 = D5, d5 = D6, d6 = D7, d7 = D8;

// if you use the NodeMCU 12E the suggested pins are
// const int RS = 4, EN = 0, d4 = 12 , d5 = 13, d6 = 15, d7 = 3;

LiquidCrystal lcd(RS, EN, d4, d5, d6, d7);

// the total count value
static volatile unsigned long counts = 0;
static volatile unsigned long triggered = 0;

static int secondcounts[60];
static unsigned long int secidx_prev = 0;
static unsigned long int count_prev = 0;
static unsigned long int second_prev = 0;

static unsigned long int lastTick = 0;

static bool ledState = false;

static const float SIEVERT_CONVERSION_FACTOR = 153.8;

#define PIN_TICK D1

#define MQTT_HOST "mosquitto.space.revspace.nl"
#define MQTT_PORT 1883

#define TOPIC_GEIGER "revspace/sensors/geiger"

#define LOG_PERIOD 3

// interrupt routine
IRAM_ATTR static void tube_impulse(void)
{
  counts++;
  triggered++;
  lastTick = millis();
  ledState = !ledState;
}

void setup()
{
  Serial.begin(115200);
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("CPM:");

  // start counting
  memset(secondcounts, 0, sizeof(secondcounts));
  Serial.println("Starting count ...");

  pinMode(PIN_TICK, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_TICK), tube_impulse, FALLING);
}

void loop()
{
  // update the circular buffer every second
  unsigned long int second = millis() / 1000;
  unsigned long int secidx = second % 60;
  if (secidx != secidx_prev)
  {
    // new second, store the counts from the last second
    unsigned long int count = counts;
    secondcounts[secidx_prev] = count - count_prev;
    count_prev = count;
    secidx_prev = secidx;
  }
  // report every LOG_PERIOD
  if ((second - second_prev) >= LOG_PERIOD)
  {
    second_prev = second;

    // calculate sum
    int cpm = 0;
    for (int i = 0; i < 60; i++)
    {
      cpm += secondcounts[i];
    }

    lcd.setCursor(0, 1);
    lcd.print(cpm);
    float msievert = cpm/SIEVERT_CONVERSION_FACTOR;

    lcd.setCursor(10, 1);
    lcd.print(msievert, 3);
  }
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  
  // print the number of seconds since reset:
  digitalWrite(LED_BUILTIN, !(millis() - lastTick < 100));
  delay(10);
}