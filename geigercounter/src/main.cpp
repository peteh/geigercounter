#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

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

#define PIN_TICK D5

#define MQTT_HOST "mosquitto.space.revspace.nl"
#define MQTT_PORT 1883

#define TOPIC_GEIGER "revspace/sensors/geiger"

#define LOG_PERIOD 3

static const char CHAR_SMILEY[8] = {
    B00000,
    B10001,
    B00000,
    B00000,
    B10001,
    B01110,
    B00000,
};

static const char CHAR_FULL[8] = {
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
};

static const char CHAR_EMPTY[8] = {
    B11111,
    B10001,
    B10001,
    B10001,
    B10001,
    B10001,
    B11111,
};

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
  // initialize LCD
  lcd.init();
  // turn on LCD backlight
  lcd.backlight();

  lcd.createChar(0, CHAR_SMILEY);
  lcd.createChar(1, CHAR_FULL);
  lcd.createChar(2, CHAR_EMPTY);

  // start counting
  memset(secondcounts, 0, sizeof(secondcounts));
  Serial.println("Starting count ...");

  pinMode(PIN_TICK, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_TICK), tube_impulse, FALLING);
}

unsigned int calculateDanger(float usieverth, float dangerLevel)
{
  return usieverth / dangerLevel;
}

void printDanger(float usieverth)
{
  unsigned int maxdanger = 5;
  // 0: < 0.3 average dose in europe is around 0.1 to 0.3 uSv/h
  // 5: 5times the average of Europe
  unsigned int danger = calculateDanger(usieverth, 0.1);
  if (danger > maxdanger)
  {
    danger = maxdanger;
  }
  for (unsigned int i = 0; i < danger; i++)
  {
    lcd.write(1);
  }
  for (unsigned int i = danger; i < maxdanger; i++)
  {
    lcd.write(2);
  }
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

    float usievert = cpm / SIEVERT_CONVERSION_FACTOR;

    lcd.setCursor(0, 0);
    printDanger(usievert);

    lcd.setCursor(9, 0);
    lcd.printf("%3d cpm", cpm);
    

    lcd.setCursor(5, 1);
    lcd.printf("%2.3f uSv/h", usievert);
  }
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):

  // print the number of seconds since reset:
  digitalWrite(LED_BUILTIN, !(millis() - lastTick < 100));
  delay(10);
}