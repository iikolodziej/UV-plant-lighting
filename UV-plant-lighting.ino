#include <Arduino.h>
#include <Wire.h>
#include "LiquidCrystal_PCF8574.h"
#include <EduIntro.h>
#include <Adafruit_Sensor.h>
#include <RTClib.h>
#include <EEPROM.h>

TwoWire Wire_1 = TwoWire();

LiquidCrystal_PCF8574 lcd(0x27);  // set the LCD address to 0x27
char lcdDisplay[2][16];           // 4 lines of 16 character buffer

RTC_DS1307 rtc;


unsigned long previousMillis = 0;
const long interval = 5000;
unsigned long previousMillisforbacklight = 0;
int countforbacklight = 0;

int hourupg = 0;
int minupg = 0;
int yearupg;
int monthupg;
int dayupg;
int menu = 0;

DateTime theTime;  // Current clock time

//pprzełącznik trybów
int przelacznik1 = 5;
int przelacznik2 = 6;

int wlacznik_podswietlenia = 4;

int przycisk_enkoder = 7;

DHT11 dht11(D8);

//zmienne do przechowywania temperatury i wilgotnosci
int temperatura;
int wilgotnosc;

#define led_1 9
#define led_2 10
#define led_3 11

int led_1_state = 50;
int led_2_state = 50;
int led_3_state = 50;

int hour_start = 0;
int hour_end = 0;

//get the variable for manually on/off the lamps
int on_off = 0;


// for encoder
volatile int counter = 0;   // licznik impulsów z enkodera
const int encoderPinA = 3;  // pin A enkodera
const int encoderPinB = 2;  // pin B enkodera

void setup() {

  // start clock
  rtc.begin();
  if (!rtc.isrunning()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  //  Wire_1.begin(D1, D2);  // custom i2c port on ESP
  Wire_1.setClock(100000);  // standard 100kHz speed
                            //  Wire_1.setClockStretchLimit(200000); // some devices might need clock stretching
  lcd.begin(16, 2, Wire_1);
  lcd.setBacklight(255);

  //---------Ekran powitalny-------------
  lcd.setCursor(4, 0);
  lcd.print("Heeeej!");
  lcd.setCursor(0, 1);
  lcd.print("Tu roslinki Agi!");
  delay(2000);
  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("Dobrego");
  lcd.setCursor(5, 1);
  lcd.print("Dnia!");
  delay(1000);
  //--------------------------------------

  //przełącznik trybów
  pinMode(przelacznik1, INPUT_PULLUP);
  pinMode(przelacznik2, INPUT_PULLUP);
  pinMode(wlacznik_podswietlenia, INPUT);
  pinMode(przycisk_enkoder, INPUT_PULLUP);

  pinMode(led_1, OUTPUT);
  pinMode(led_2, OUTPUT);
  pinMode(led_3, OUTPUT);

  pinMode(encoderPinA, INPUT_PULLUP);
  pinMode(encoderPinB, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(encoderPinA), handleInterrupt, CHANGE);

  lcd.clear();
  lcd.setBacklight(HIGH);

  led_1_state = EEPROM.read(1);
  led_2_state = EEPROM.read(16);
  led_3_state = EEPROM.read(32);
  hour_start = EEPROM.read(48);
  hour_end = EEPROM.read(64);
}

void loop() {
  /////-------wybor trybow------
  int stan_przelacznika1 = digitalRead(przelacznik1);
  int stan_przelacznika2 = digitalRead(przelacznik2);


  if (stan_przelacznika1 == HIGH && stan_przelacznika2 == LOW) {
    tryb_automatyczny();
  } else if (stan_przelacznika1 == HIGH && stan_przelacznika2 == HIGH) {
    tryb_manualny();
  } else {
    lcd.setBacklight(HIGH);
    // set the settings
    //set the time with buttons
    if (digitalRead(przycisk_enkoder) == LOW) {
      menu = menu + 1;
      delay(500);
    }
    // in which subroutine should we go?
    if (menu == 0) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Ustawienia");
      lcd.setCursor(0, 1);
      lcd.print("Wcisnij przycisk");
    }
    if (menu == 1) {
      DisplaySetHour();
    }
    if (menu == 2) {
      DisplaySetMinute();
    }
    if (menu == 3) {
      Set_led_1_state();
    }
    if (menu == 4) {
      Set_led_2_state();
    }
    if (menu == 5) {
      Set_led_3_state();
    }
    if (menu == 6) {
      Set_hour_start();
    }
    delay(100);
    if (menu == 7) {
      Set_hour_end();
    }
    if (menu == 8) {
      StoreAgg();
      delay(500);
      menu = 0;
    }
    delay(100);
  }
  //-------------------------------
}
//--------------------------------------------------------------------tryb manualny-----------------------------------------------------------
void tryb_manualny() {
  if (digitalRead(przycisk_enkoder) == LOW) {
    if (on_off == 0) {
      on_off = 1;
    } else {
      on_off = 0;
    }
    delay(200);
  }
  if (on_off == 1) {
    analogWrite(led_1, led_1_state);
    analogWrite(led_2, led_2_state);
    analogWrite(led_3, led_3_state);
  } else {
    analogWrite(led_1, 0);
    analogWrite(led_2, 0);
    analogWrite(led_3, 0);
  }
  //   ---------sterowanie podswietleniem-----------
  if (digitalRead(wlacznik_podswietlenia) == 1) {
    countforbacklight = 0;
  }
  unsigned long currentMillisforbacklight = millis();
  if (currentMillisforbacklight - previousMillisforbacklight >= 20000) {
    previousMillisforbacklight = currentMillisforbacklight;
    lcd.setBacklight(LOW);
    countforbacklight = 1;
  } else if (countforbacklight == 0) {
    lcd.setBacklight(HIGH);
  }
  //----------------------------------------------

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Manualny");

    DateTime now = rtc.now();
    lcd.setCursor(1, 1);
    lcd.print(now.hour());
    lcd.setCursor(3, 1);
    lcd.print(':');

    //if the minute is less than 10 display "0" before the single numer of minute
    if (now.minute() < 10) {
      lcd.setCursor(4, 1);
      lcd.print("0");
      lcd.setCursor(5, 1);
      lcd.print(now.minute());
    } else {
      lcd.setCursor(4, 1);
      lcd.print(now.minute());
    }

    delay(1000);
  } else {

    wyswietlanie_temp_wilg();
  }
}


//-------------------------------------------------------------tryb automatyczny---------------------------------------------------------------------
void tryb_automatyczny() {
  DateTime now = rtc.now();

  if(hour_start < hour_end){
  if ((now.hour() >= hour_start) && (now.hour() < hour_end)) {
    DateTime now = rtc.now();
    analogWrite(led_1, led_1_state);
    analogWrite(led_2, led_2_state);
    analogWrite(led_3, led_3_state);
  } else {
    analogWrite(led_1, 0);
    analogWrite(led_2, 0);
    analogWrite(led_3, 0);
  }
  }
else{
    if ((now.hour() >= hour_start) || (now.hour() < hour_end)) {
    DateTime now = rtc.now();
    analogWrite(led_1, led_1_state);
    analogWrite(led_2, led_2_state);
    analogWrite(led_3, led_3_state);
  } else {
    analogWrite(led_1, 0);
    analogWrite(led_2, 0);
    analogWrite(led_3, 0);
  }
}
  //  ---------sterowanie podswietleniem-----------
  if (digitalRead(wlacznik_podswietlenia) == 1) {
    countforbacklight = 0;
  }
  unsigned long currentMillisforbacklight = millis();
  if (currentMillisforbacklight - previousMillisforbacklight >= 20000) {
    previousMillisforbacklight = currentMillisforbacklight;
    lcd.setBacklight(LOW);
    countforbacklight = 1;
  } else if (countforbacklight == 0) {
    lcd.setBacklight(HIGH);
  }
  //----------------------------------------------
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Automatyczny");

    DateTime now = rtc.now();
    lcd.setCursor(1, 1);
    lcd.print(now.hour());
    lcd.setCursor(3, 1);
    lcd.print(':');

    //if the minute is less than 10 display "0" before the single numer of minute
    if (now.minute() < 10) {
      lcd.setCursor(4, 1);
      lcd.print("0");
      lcd.setCursor(5, 1);
      lcd.print(now.minute());
    } else {
      lcd.setCursor(4, 1);
      lcd.print(now.minute());
    }

    lcd.setCursor(7, 1);
    lcd.print("S:");
    lcd.setCursor(9, 1);
    lcd.print(hour_start);

    lcd.setCursor(12, 1);
    lcd.print("K:");
    lcd.setCursor(14, 1);
    lcd.print(hour_end);

    delay(1000);
  } else {

    wyswietlanie_temp_wilg();
  }
}
//-----------------------------------------------wyswietlanie temperatury i wilgotnosci--------------------------------------------------------------
void wyswietlanie_temp_wilg() {


  //odczyt temperatury i wilgotnosci
  dht11.update();
  wilgotnosc = dht11.readHumidity();
  temperatura = dht11.readCelsius();

  lcd.setCursor(0, 0);
  lcd.print("Wilgotnosc: ");
  lcd.setCursor(12, 0);
  lcd.print(wilgotnosc);
  lcd.setCursor(14, 0);
  lcd.print("%");
  lcd.setCursor(0, 1);
  lcd.print("Temperatura: ");
  lcd.setCursor(12, 1);
  lcd.print(temperatura);
  lcd.setCursor(14, 1);
  lcd.print((char)0xDF);
  lcd.setCursor(15, 1);
  lcd.print("C");
}

//------------------------------------------------------------ustawienia---------------------------------------------------------------



//--------------------------------set real hour------------------------------
void DisplaySetHour() {
  // time setting
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Set hour");

  DateTime now = rtc.now();
  if (counter >= 24) {
    counter = 24;
  }

  hourupg = counter;

  lcd.setCursor(0, 1);
  lcd.print(hourupg);
  delay(200);
  lcd.clear();
  //
}

//-------------------------------------set real minute------------------------------------------------------
void DisplaySetMinute() {
  // Setting the minutes
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Set minute");

  if (counter >= 60) {
    counter = 60;
  }

  minupg = counter;

  lcd.setCursor(0, 1);
  lcd.print(minupg);
  delay(200);
  lcd.clear();
}

//-------------------------------------store arguments------------------------------------------------------
void StoreAgg() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Saving...");
  lcd.setCursor(0, 18);
  delay(200);
  lcd.clear();
  rtc.adjust(DateTime(yearupg, monthupg, dayupg, hourupg, minupg, 0));

  EEPROM.write(1, led_1_state);
  EEPROM.write(16, led_2_state);
  EEPROM.write(32, led_3_state);
  EEPROM.write(48, hour_start);
  EEPROM.write(64, hour_end);
  delay(200);
}

void read_adj_set() {
}


void handleInterrupt() {
  // sprawdzenie stanu pinów A i B enkodera
  int a = digitalRead(encoderPinA);
  int b = digitalRead(encoderPinB);
  delay(50);
  if (a == b) {
    // obrót w prawo
    counter++;
  } else {
    // obrót w lewo
    counter--;
  }
  if (counter < 0) {
    counter = 0;
  }
}

void Set_led_1_state() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Set led1 power");

  if (counter >= 255) {
    counter = 255;
  }

  led_1_state = counter;

  lcd.setCursor(0, 1);
  lcd.print(led_1_state);
  delay(200);
  lcd.clear();
}

void Set_led_2_state() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Set led2 power");

  if (counter >= 255) {
    counter = 255;
  }

  led_2_state = counter;

  lcd.setCursor(0, 1);
  lcd.print(led_2_state);
  delay(200);
  lcd.clear();
}

void Set_led_3_state() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Set led3 power");

  if (counter >= 255) {
    counter = 255;
  }

  led_3_state = counter;

  lcd.setCursor(0, 1);
  lcd.print(led_3_state);
  delay(200);
  lcd.clear();
}

void Set_hour_start() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Set hour start");

  DateTime now = rtc.now();
  if (counter >= 24) {
    counter = 24;
  }

  hour_start = counter;

  lcd.setCursor(0, 1);
  lcd.print(hour_start);
  delay(200);
  lcd.clear();
}

void Set_hour_end() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Set hour end");

  DateTime now = rtc.now();
  if (counter >= 24) {
    counter = 24;
  }

  hour_end = counter;

  lcd.setCursor(0, 1);
  lcd.print(hour_end);
  delay(200);
  lcd.clear();
}