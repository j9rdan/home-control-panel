#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

// TESTING (uncomment to DEBUG)

//#define DEBUG
//#define STRUCT DEBUG
#define DATA DEBUG

// LCD - BACKLIGHT COLOURS:

#define RED 0x1       // changingHeat
#define YELLOW 0x3    // changingLight
#define GREEN 0x2     // changingLamp
#define TEAL 0x6      // outside & changingWater
#define BLUE 0x4      // firstFloor (incl. rooms)
#define VIOLET 0x5    // groundFloor (incl. rooms)
#define WHITE 0x7     // startup

// LCD - CUSTOM CHARS:

char leftLCD = ' ';
char upLCD = ' ';
char downLCD = ' ';
char rightLCD = ' ';
char fullPxLCD = ' ';

// STATE DATA:

byte lastHouseState;
byte lastBtnState = 0;
byte currentBtnState;
unsigned long previousTime = 0;
String location = "";
boolean blankOnOff = false;

// HOUSE DATA STRUCTURES:

typedef enum {   // all house states
  startup, 
  firstFloor, 
  groundFloor, 
  outside, 
  bedroom1, 
  bedroom2, 
  bathroom, 
  kitchen, 
  hall, 
  livingRoom, 
  garden, 
  garage, 
  changingLight, 
  changingHeat, 
  changingLamp, 
  changingWater 
} HouseState;

typedef struct {   // struct of type Data containing all device data
  byte level;
  byte onHour;
  byte onMins;
  byte offHour;
  byte offMins;
} Data;

typedef struct {   // struct of type Room containing all house devices of type Data
  Data light;
  Data heat;
  Data lamp;
  Data water;
} Room;

HouseState state = startup;  // initial state = startup
Room room; // generic room used to change values of specific rooms depending on states
Room bed1_s, bed2_s, bathroom_s, kitchen_s, hall_s, livingRoom_s, garden_s, garage_s; // all house rooms


// HELPER FUNCTIONS:

boolean isSinglePress(int button) {   /* returns true if a given button has been pressed once */
  
    if (lastBtnState == 0 && currentBtnState == button)
      return true;
  
    return false;
  }


void createBtnChars() {  /* initialises LCD custom characters and stores for reuse */

  byte leftArrow[] = { B00000, B00100, B01000, B11111, B11111, B01000, B00100, B00000 };
  lcd.createChar(0, leftArrow);
  leftLCD = (char)0;

  byte upArrow[] = { B00000, B00100, B01110, B10101, B00100, B00100, B00100, B00000 };
  lcd.createChar(1, upArrow);
  upLCD = (char)1;

  byte downArrow[] = { B00000, B00100, B00100, B00100, B10101, B01110, B00100, B00000 };
  lcd.createChar(2, downArrow);
  downLCD = (char)2;

  byte rightArrow[] = { B00000, B00100, B00010, B11111, B11111, B00010, B00100, B00000 };
  lcd.createChar(3, rightArrow);
  rightLCD = (char)3;

  byte fullPixel[] = { B11111, B11111, B11111, B11111, B11111, B11111, B11111, B11111 };
  lcd.createChar(4, fullPixel);
  fullPxLCD = (char)4;

}


void print1By1(byte rowPosition, char *text, byte msDelay) {  /* prints a given list of chars 1 by 1 at specified speed */

  unsigned long currentTime = millis();
  static byte i = 0;
  for (i; i <= strlen(text) - 1 && currentTime - previousTime >= msDelay; i++) {
    lcd.setCursor(i, rowPosition);
    lcd.print(text[i]);
    previousTime = currentTime;
  }
}


void showRoomDevices(byte rowPosition, int msDelay) {  /* blinks room devices on specified LCD row with specified interval between blinks */

  unsigned long currentTime = millis();
  lcd.setCursor(0, rowPosition);
  if (currentTime - previousTime > msDelay) {
    previousTime = currentTime;
    if (blankOnOff) {
      lcd.print(upLCD);
      lcd.print(F(" LIGHT   HEAT "));
      lcd.print(downLCD);
    } else {
      lcd.print(F("     LAMP "));
      lcd.print(rightLCD);
    }
    lcd.print(F("                   "));
    blankOnOff = !blankOnOff;
  }
}


void previousStateDebug() { /* tracks back presses via BUTTON_LEFT on Serial monitor */

  location = "";
  Serial.print(F("\nLEFT (x1) - "));

  switch (lastHouseState) {

    case bedroom1:
      Serial.print(F("Back to Bedroom 1 --> ")); Serial.println(location += "First/Bedroom 1/");
      Serial.println(F("Current State: BEDROOM 1"));
      break;

    case bedroom2:
      Serial.print(F("Back to Bedroom 2 --> ")); Serial.println(location += "First/Bedroom 2/");
      Serial.println(F("Current State: BEDROOM 2"));
      break;

    case bathroom:
      Serial.print(F("Back to Bathroom --> ")); Serial.println(location += "First/Bathroom/");
      Serial.println(F("Current State: BATHROOM"));
      break;

    case kitchen:
      Serial.print(F("Back to Kitchen --> ")); Serial.println(location += "Ground/Kitchen/");
      Serial.println(F("Current State: KITCHEN"));
      break;

    case hall:
      Serial.print(F("Back to Hall --> ")); Serial.println(location += "Ground/Hall/");
      Serial.println(F("Current State: HALL"));
      break;

    case livingRoom:
      Serial.print(F("Back to Living Room --> ")); Serial.println(location += "Ground/Living Room/");
      Serial.println(F("Current State: LIVING ROOM"));
      break;

    case garden:
      Serial.print(F("Back to Garden --> ")); Serial.println(location += "Outside/Garden/");
      Serial.println(F("Current State: GARDEN"));
      break;

    case garage:
      Serial.print(F("Back to Garage --> ")); Serial.println(location += "Outside/Garage/");
      Serial.println(F("Current State: GARAGE"));
      break;

  }

}


void initialiseData() { /* initialise mock data for room devices */

  bed1_s.light.level = 0;
  bed1_s.light.onHour = 17;
  bed1_s.light.onMins = 11;
  bed1_s.light.offHour = 2;
  bed1_s.light.offMins = 7;
  bed1_s.heat.level = 35;
  bed1_s.heat.onHour = 16;
  bed1_s.heat.onMins = 57;
  bed1_s.heat.offHour = 6;
  bed1_s.heat.offMins = 23;
  bed1_s.lamp.level = 99;
  bed1_s.lamp.onHour = 18;
  bed1_s.lamp.onMins = 42;
  bed1_s.lamp.offHour = 23;
  bed1_s.lamp.offMins = 43;

  bed2_s.light.level = 2;
  bed2_s.light.onHour = 0;
  bed2_s.light.onMins = 16;
  bed2_s.light.offHour = 20;
  bed2_s.light.offMins = 49;
  bed2_s.heat.level = 9;
  bed2_s.heat.onHour = 8;
  bed2_s.heat.onMins = 30;
  bed2_s.heat.offHour = 12;
  bed2_s.heat.offMins = 23;
  bed2_s.lamp.level = 35;
  bed2_s.lamp.onHour = 23;
  bed2_s.lamp.onMins = 30;
  bed2_s.lamp.offHour = 7;
  bed2_s.lamp.offMins = 0;

  bathroom_s.light.level = 100;
  bathroom_s.light.onHour = 9;
  bathroom_s.light.onMins = 0;
  bathroom_s.light.offHour = 10;
  bathroom_s.light.offMins = 0;
  bathroom_s.heat.level = 50;
  bathroom_s.heat.onHour = 9;
  bathroom_s.heat.onMins = 0;
  bathroom_s.heat.offHour = 10;
  bathroom_s.heat.offMins = 0;
  bathroom_s.lamp.level = 20;
  bathroom_s.lamp.onHour = 19;
  bathroom_s.lamp.onMins = 0;
  bathroom_s.lamp.offHour = 0;
  bathroom_s.lamp.offMins = 0;

  kitchen_s.light.level = 90;
  kitchen_s.light.onHour = 8;
  kitchen_s.light.onMins = 30;
  kitchen_s.light.offHour = 9;
  kitchen_s.light.offMins = 0;
  kitchen_s.heat.level = 70;
  kitchen_s.heat.onHour = 8;
  kitchen_s.heat.onMins = 0;
  kitchen_s.heat.offHour = 15;
  kitchen_s.heat.offMins = 30;
  kitchen_s.lamp.level = 0;
  kitchen_s.lamp.onHour = 18;
  kitchen_s.lamp.onMins = 45;
  kitchen_s.lamp.offHour = 0;
  kitchen_s.lamp.offMins = 0;

  hall_s.light.level = 95;
  hall_s.light.onHour = 9;
  hall_s.light.onMins = 0;
  hall_s.light.offHour = 21;
  hall_s.light.offMins = 30;
  hall_s.heat.level = 58;
  hall_s.heat.onHour = 8;
  hall_s.heat.onMins = 30;
  hall_s.heat.offHour = 22;
  hall_s.heat.offMins = 0;
  hall_s.lamp.level = 0;
  hall_s.lamp.onHour = 18;
  hall_s.lamp.onMins = 42;
  hall_s.lamp.offHour = 23;
  hall_s.lamp.offMins = 30;

  livingRoom_s.light.level = 100;
  livingRoom_s.light.onHour = 17;
  livingRoom_s.light.onMins = 30;
  livingRoom_s.light.offHour = 0;
  livingRoom_s.light.offMins = 0;
  livingRoom_s.heat.level = 85;
  livingRoom_s.heat.onHour = 16;
  livingRoom_s.heat.onMins = 30;
  livingRoom_s.heat.offHour = 0;
  livingRoom_s.heat.offMins = 0;
  livingRoom_s.lamp.level = 53;
  livingRoom_s.lamp.onHour = 21;
  livingRoom_s.lamp.onMins = 45;
  livingRoom_s.lamp.offHour = 0;
  livingRoom_s.lamp.offMins = 0;

  garden_s.lamp.level = 39;
  garden_s.lamp.onHour = 15;
  garden_s.lamp.onMins = 30;
  garden_s.lamp.offHour = 20;
  garden_s.lamp.offMins = 30;
  garden_s.water.level = 5;
  garden_s.water.onHour = 17;
  garden_s.water.onMins = 50;
  garden_s.water.offHour = 19;
  garden_s.water.offMins = 30;

  garage_s.light.level = 80;
  garage_s.light.onHour = 19;
  garage_s.light.onMins = 30;
  garage_s.light.offHour = 0;
  garage_s.light.offMins = 0;
  garage_s.lamp.level = 0;
  garage_s.lamp.onHour = 22;
  garage_s.lamp.onMins = 30;
  garage_s.lamp.offHour = 5;
  garage_s.lamp.offMins = 30;

}


void printAllData() { /* prints all room data in format: Floor/Room/Type/Name/Action:VAL */

  Serial.print(F("\nFirst/Bedroom 1/Light/Main/Level: "));     Serial.println(bed1_s.light.level);
  Serial.print(F("First/Bedroom 1/Light/Main/On: "));          Serial.print(bed1_s.light.onHour);      Serial.print(F("."));  Serial.println(bed1_s.light.onMins);
  Serial.print(F("First/Bedroom 1/Light/Main/Off: "));         Serial.print(bed1_s.light.offHour);     Serial.print(F("."));  Serial.println(bed1_s.light.offMins);
  Serial.print(F("First/Bedroom 1/Heat/Main/Level: "));        Serial.println(bed1_s.heat.level);
  Serial.print(F("First/Bedroom 1/Heat/Main/On: "));           Serial.print(bed1_s.heat.onHour);       Serial.print(F("."));  Serial.println(bed1_s.heat.onMins);
  Serial.print(F("First/Bedroom 1/Heat/Main/Off: "));          Serial.print(bed1_s.heat.offHour);      Serial.print(F("."));  Serial.println(bed1_s.heat.offMins);
  Serial.print(F("First/Bedroom 1/Lamp/Desk/Level: "));        Serial.println(bed1_s.lamp.level);
  Serial.print(F("First/Bedroom 1/Lamp/Desk/On: "));           Serial.print(bed1_s.lamp.onHour);       Serial.print(F("."));  Serial.println(bed1_s.lamp.onMins);
  Serial.print(F("First/Bedroom 1/Lamp/Desk/Off: "));          Serial.print(bed1_s.lamp.offHour);      Serial.print(F("."));  Serial.println(bed1_s.lamp.offMins);

  Serial.print(F("\nFirst/Bedroom 2/Light/Main/Level: "));     Serial.println(bed2_s.light.level);
  Serial.print(F("First/Bedroom 2/Light/Main/On: "));          Serial.print(bed2_s.light.onHour);      Serial.print(F("."));  Serial.println(bed2_s.light.onMins);
  Serial.print(F("First/Bedroom 2/Light/Main/Off: "));         Serial.print(bed2_s.light.offHour);     Serial.print(F("."));  Serial.println(bed2_s.light.offMins);
  Serial.print(F("First/Bedroom 2/Heat/Main/Level: "));        Serial.println(bed2_s.heat.level);
  Serial.print(F("First/Bedroom 2/Heat/Main/On: "));           Serial.print(bed2_s.heat.onHour);       Serial.print(F("."));  Serial.println(bed2_s.heat.onMins);
  Serial.print(F("First/Bedroom 2/Heat/Main/Off: "));          Serial.print(bed2_s.heat.offHour);      Serial.print(F("."));  Serial.println(bed2_s.heat.offMins);
  Serial.print(F("First/Bedroom 2/Lamp/Desk/Level: "));        Serial.println(bed2_s.lamp.level);
  Serial.print(F("First/Bedroom 2/Lamp/Desk/On: "));           Serial.print(bed2_s.lamp.onHour);       Serial.print(F("."));  Serial.println(bed2_s.lamp.onMins);
  Serial.print(F("First/Bedroom 2/Lamp/Desk/Off: "));          Serial.print(bed2_s.lamp.offHour);      Serial.print(F("."));  Serial.println(bed2_s.lamp.offMins);

  Serial.print(F("\nFirst/Bathroom/Light/Main/Level: "));      Serial.println(bathroom_s.light.level);
  Serial.print(F("First/Bathroom/Light/Main/On: "));           Serial.print(bathroom_s.light.onHour);  Serial.print(F("."));  Serial.println(bathroom_s.light.onMins);
  Serial.print(F("First/Bathroom/Light/Main/Off: "));          Serial.print(bathroom_s.light.offHour); Serial.print(F("."));  Serial.println(bathroom_s.light.offMins);
  Serial.print(F("First/Bathroom/Heat/Main/Level: "));         Serial.println(bathroom_s.heat.level);
  Serial.print(F("First/Bathroom/Heat/Main/On: "));            Serial.print(bathroom_s.heat.onHour);   Serial.print(F("."));  Serial.println(bathroom_s.heat.onMins);
  Serial.print(F("First/Bathroom/Heat/Main/Off: "));           Serial.print(bathroom_s.heat.offHour);  Serial.print(F("."));  Serial.println(bathroom_s.heat.offMins);
  Serial.print(F("First/Bathroom/Lamp/Mirror/Level: "));       Serial.println(bathroom_s.lamp.level);
  Serial.print(F("First/Bathroom/Lamp/Mirror/On: "));          Serial.print(bathroom_s.lamp.onHour);   Serial.print(F("."));  Serial.println(bathroom_s.lamp.onMins);
  Serial.print(F("First/Bathroom/Lamp/Mirror/Off: "));         Serial.print(bathroom_s.lamp.offHour);  Serial.print(F("."));  Serial.println(bathroom_s.lamp.offMins);

  Serial.print(F("\nGround/Kitchen/Light/Main/Level: "));      Serial.println(kitchen_s.light.level);
  Serial.print(F("Ground/Kitchen/Light/Main/On: "));           Serial.print(kitchen_s.light.onHour);   Serial.print(F("."));  Serial.println(kitchen_s.light.onMins);
  Serial.print(F("Ground/Kitchen/Light/Main/Off: "));          Serial.print(kitchen_s.light.offHour);  Serial.print(F("."));  Serial.println(kitchen_s.light.offMins);
  Serial.print(F("Ground/Kitchen/Heat/Main/Level: "));         Serial.println(kitchen_s.heat.level);
  Serial.print(F("Ground/Kitchen/Heat/Main/On: "));            Serial.print(kitchen_s.heat.onHour);    Serial.print(F("."));  Serial.println(kitchen_s.heat.onMins);
  Serial.print(F("Ground/Kitchen/Heat/Main/Off: "));           Serial.print(kitchen_s.heat.offHour);   Serial.print(F("."));  Serial.println(kitchen_s.heat.offMins);
  Serial.print(F("Ground/Kitchen/Lamp/Ceiling/Level: "));      Serial.println(kitchen_s.lamp.level);
  Serial.print(F("Ground/Kitchen/Lamp/Ceiling/On: "));         Serial.print(kitchen_s.lamp.onHour);    Serial.print(F("."));  Serial.println(kitchen_s.lamp.onMins);
  Serial.print(F("Ground/Kitchen/Lamp/Ceiling/Off: "));        Serial.print(kitchen_s.lamp.offHour);   Serial.print(F("."));  Serial.println(kitchen_s.lamp.offMins);

  Serial.print(F("\nGround/Hall/Light/Main/Level: "));         Serial.println(hall_s.light.level);
  Serial.print(F("Ground/Hall/Light/Main/On: "));              Serial.print(hall_s.light.onHour);      Serial.print(F("."));  Serial.println(hall_s.light.onMins);
  Serial.print(F("Ground/Hall/Light/Main/Off: "));             Serial.print(hall_s.light.offHour);     Serial.print(F("."));  Serial.println(hall_s.light.offMins);
  Serial.print(F("Ground/Hall/Heat/Main/Level: "));            Serial.println(hall_s.heat.level);
  Serial.print(F("Ground/Hall/Heat/Main/On: "));               Serial.print(hall_s.heat.onHour);       Serial.print(F("."));  Serial.println(hall_s.heat.onMins);
  Serial.print(F("Ground/Hall/Heat/Main/Off: "));              Serial.print(hall_s.heat.offHour);      Serial.print(F("."));  Serial.println(hall_s.heat.offMins);
  Serial.print(F("Ground/Hall/Lamp/Floor/Level: "));           Serial.println(hall_s.lamp.level);
  Serial.print(F("Ground/Hall/Lamp/Floor/On: "));              Serial.print(hall_s.lamp.onHour);       Serial.print(F("."));  Serial.println(hall_s.lamp.onMins);
  Serial.print(F("Ground/Hall/Lamp/Floor/Off: "));             Serial.print(hall_s.lamp.offHour);      Serial.print(F("."));  Serial.println(hall_s.lamp.offMins);

  Serial.print(F("\nGround/Living Room/Light/Main/Level: "));  Serial.println(hall_s.light.level);
  Serial.print(F("Ground/Living Room/Light/Main/On: "));       Serial.print(hall_s.light.onHour);      Serial.print(F("."));  Serial.println(hall_s.light.onMins);
  Serial.print(F("Ground/Living Room/Light/Main/Off: "));      Serial.print(hall_s.light.offHour);     Serial.print(F("."));  Serial.println(hall_s.light.offMins);
  Serial.print(F("Ground/Living Room/Heat/Main/Level: "));     Serial.println(hall_s.heat.level);
  Serial.print(F("Ground/Living Room/Heat/Main/On: "));        Serial.print(hall_s.heat.onHour);       Serial.print(F("."));  Serial.println(hall_s.heat.onMins);
  Serial.print(F("Ground/Living Room/Heat/Main/Off: "));       Serial.print(hall_s.heat.offHour);      Serial.print(F("."));  Serial.println(hall_s.heat.offMins);
  Serial.print(F("Ground/Living Room/Lamp/Table/Level: "));    Serial.println(hall_s.lamp.level);
  Serial.print(F("Ground/Living Room/Lamp/Table/On: "));       Serial.print(hall_s.lamp.onHour);       Serial.print(F("."));  Serial.println(hall_s.lamp.onMins);
  Serial.print(F("Ground/Living Room/Lamp/Table/Off: "));      Serial.print(hall_s.lamp.offHour);      Serial.print(F("."));  Serial.println(hall_s.lamp.offMins);

  Serial.print(F("\nOutside/Garden/Water/Main/Level: "));      Serial.println(garden_s.water.level);
  Serial.print(F("Outside/Garden/Water/Main/On: "));           Serial.print(garden_s.water.onHour);    Serial.print(F("."));  Serial.println(garden_s.water.onMins);
  Serial.print(F("Outside/Garden/Water/Main/Off: "));          Serial.print(garden_s.water.offHour);   Serial.print(F("."));  Serial.println(garden_s.water.offMins);
  Serial.print(F("Outside/Garden/Lamp/String/Level: "));       Serial.println(garden_s.lamp.level);
  Serial.print(F("Outside/Garden/Lamp/String/On: "));          Serial.print(garden_s.lamp.onHour);     Serial.print(F("."));  Serial.println(garden_s.lamp.onMins);
  Serial.print(F("Outside/Garden/Lamp/String/Off: "));         Serial.print(garden_s.lamp.offHour);    Serial.print(F("."));  Serial.println(garden_s.lamp.offMins);

  Serial.print(F("\nOutside/Garage/Light/Main/Level: "));      Serial.println(garage_s.light.level);
  Serial.print(F("Outside/Garage/Light/Main/On: "));           Serial.print(garage_s.light.onHour);    Serial.print(F("."));  Serial.println(garage_s.light.onMins);
  Serial.print(F("Outside/Garage/Light/Main/Off: "));          Serial.print(garage_s.light.offHour);   Serial.print(F("."));  Serial.println(garage_s.light.offMins);
  Serial.print(F("Outside/Garage/Lamp/Halogen/Level: "));      Serial.println(garage_s.lamp.level);
  Serial.print(F("Outside/Garage/Lamp/Halogen/On: "));         Serial.print(garage_s.lamp.onHour);     Serial.print(F("."));  Serial.println(garage_s.lamp.onMins);
  Serial.print(F("Outside/Garage/Lamp/Halogen/Off: "));        Serial.print(garage_s.lamp.offHour);    Serial.print(F("."));  Serial.println(garage_s.lamp.offMins);

}


Room getDeviceRoom() { /* returns corresponding room struct based on last state */
  
  switch (lastHouseState) {

    case bedroom1:
      #ifdef STRUCT DEBUG
        Serial.println(F("Returned room: bed 1"));
      #endif
      return bed1_s;
    
    case bedroom2:
      #ifdef STRUCT DEBUG
        Serial.println(F("Returned room: bed 2"));
      #endif
      return bed2_s;
    
    case bathroom:
      #ifdef STRUCT DEBUG
        Serial.println(F("Returned room: bathroom"));
      #endif
      return bathroom_s;
    
    case kitchen:
      #ifdef STRUCT DEBUG
        Serial.println(F("Returned room: kitchen"));
      #endif
      return kitchen_s;
    
    case hall:
      #ifdef STRUCT DEBUG
        Serial.println(F("Returned room: hall"));
      #endif
      return hall_s;
    
    case livingRoom:
      #ifdef STRUCT DEBUG
        Serial.println(F("Returned room: living room"));
      #endif
      return livingRoom_s;
   
    case garden:
      #ifdef STRUCT DEBUG
        Serial.println(F("Returned room: garden"));
      #endif
      return garden_s;
    
    case garage:
      #ifdef STRUCT DEBUG
        Serial.println(F("Returned room: garage"));
      #endif
      return garage_s;
  }
}


// MEMORY EXTENSION (use either one at a time, comment out unused function)

int freeMemory() {    /* returns amount of unused RAM (space between stack & heap) */
  
  extern int __heap_start, *__brkval;
  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

//#ifdef __arm__
//// should use uinstd.h to define sbrk but Due causes a conflict
//extern "C" char* sbrk(int incr);
//#else // __ARM__
//extern char *__brkval;
//#endif // __arm__
//
//int freeMemory() {
//  char top;
//  #ifdef __arm__
//  return &top - reinterpret_cast<char*>(sbrk(0));
//  #elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
//  return &top - __brkval;
//  #else // __arm__
//  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
//  #endif // __arm__
//}



// UPDATE INFORMATION IN THE STRUCTURE:
  
void saveNewLvl(byte data) { /* updates corresponding room data with user-adjusted level, similar structure for following 'save' functions */

  switch (lastHouseState) {

    case bedroom1:
      switch (state) {
        case changingLight:
          bed1_s.light.level = data;
          break;
        case changingHeat:
          bed1_s.heat.level = data;
          break;
        case changingLamp:
          bed1_s.lamp.level = data;
          break;
      }
      break;

    case bedroom2:
      switch (state) {
        case changingLight:
          bed2_s.light.level = data;
          break;
        case changingHeat:
          bed2_s.heat.level = data;
          break;
        case changingLamp:
          bed2_s.lamp.level = data;
          break;
      }
      break;

    case bathroom:
      switch (state) {
        case changingLight:
          bathroom_s.light.level = data;
          break;
        case changingHeat:
          bathroom_s.heat.level = data;
          break;
        case changingLamp:
          bathroom_s.lamp.level = data;
          break;
      }
      break;

    case kitchen:
      switch (state) {
        case changingLight:
          kitchen_s.light.level = data;
          break;
        case changingHeat:
          kitchen_s.heat.level = data;
          break;
        case changingLamp:
          kitchen_s.lamp.level = data;
          break;
      }
      break;

    case hall:
      switch (state) {
        case changingLight:
          hall_s.light.level = data;
          break;
        case changingHeat:
          hall_s.heat.level = data;
          break;
        case changingLamp:
          hall_s.lamp.level = data;
          break;
      }
      break;

    case livingRoom:
      switch (state) {
        case changingLight:
          livingRoom_s.light.level = data;
          break;
        case changingHeat:
          livingRoom_s.heat.level = data;
          break;
        case changingLamp:
          livingRoom_s.lamp.level = data;
          break;
      }
      break;

    case garden:
      switch (state) {
        case changingLamp:
          garden_s.lamp.level = data;
          break;
        case changingWater:
          garden_s.water.level = data;
          break;
      }
      break;

    case garage:
      switch (state) {
        case changingLight:
          garage_s.light.level = data;
          break;
        case changingLamp:
          garage_s.lamp.level = data;
          break;
      }
      break;
  }
}


void saveNewOnHour(byte data) {

  switch (lastHouseState) {

    case bedroom1:
      switch (state) {
        case changingLight:
          bed1_s.light.onHour = data;
          break;
        case changingHeat:
          bed1_s.heat.onHour = data;
          break;
        case changingLamp:
          bed1_s.lamp.onHour = data;
          break;
      }
      break;

    case bedroom2:
      switch (state) {
        case changingLight:
          bed2_s.light.onHour = data;
          break;
        case changingHeat:
          bed2_s.heat.onHour = data;
          break;
        case changingLamp:
          bed2_s.lamp.onHour = data;
          break;
      }
      break;

    case bathroom:
      switch (state) {
        case changingLight:
          bathroom_s.light.onHour = data;
          break;
        case changingHeat:
          bathroom_s.heat.onHour = data;
          break;
        case changingLamp:
          bathroom_s.lamp.onHour = data;
          break;
      }
      break;

    case kitchen:
      switch (state) {
        case changingLight:
          kitchen_s.light.onHour = data;
          break;
        case changingHeat:
          kitchen_s.heat.onHour = data;
          break;
        case changingLamp:
          kitchen_s.lamp.onHour = data;
          break;
      }
      break;

    case hall:
      switch (state) {
        case changingLight:
          hall_s.light.onHour = data;
          break;
        case changingHeat:
          hall_s.heat.onHour = data;
          break;
        case changingLamp:
          hall_s.lamp.onHour = data;
          break;
      }
      break;

    case livingRoom:
      switch (state) {
        case changingLight:
          livingRoom_s.light.onHour = data;
          break;
        case changingHeat:
          livingRoom_s.heat.onHour = data;
          break;
        case changingLamp:
          livingRoom_s.lamp.onHour = data;
          break;
      }
      break;

    case garden:
      switch (state) {
        case changingLamp:
          garden_s.lamp.onHour = data;
          break;
        case changingWater:
          garden_s.water.onHour = data;
          break;
      }
      break;

    case garage:
      switch (state) {
        case changingLight:
          garage_s.light.onHour = data;
          break;
        case changingLamp:
          garage_s.lamp.onHour = data;
          break;
      }
      break;

  }

}


void saveNewOffHour(byte data) {

  switch (lastHouseState) {

    case bedroom1:
      switch (state) {
        case changingLight:
          bed1_s.light.offHour = data;
          break;
        case changingHeat:
          bed1_s.heat.offHour = data;
          break;
        case changingLamp:
          bed1_s.lamp.offHour = data;
          break;
      }
      break;

    case bedroom2:
      switch (state) {
        case changingLight:
          bed2_s.light.offHour = data;
          break;
        case changingHeat:
          bed2_s.heat.offHour = data;
          break;
        case changingLamp:
          bed2_s.lamp.offHour = data;
          break;
      }
      break;

    case bathroom:
      switch (state) {
        case changingLight:
          bathroom_s.light.offHour = data;
          break;
        case changingHeat:
          bathroom_s.heat.offHour = data;
          break;
        case changingLamp:
          bathroom_s.lamp.offHour = data;
          break;
      }
      break;

    case kitchen:
      switch (state) {
        case changingLight:
          kitchen_s.light.offHour = data;
          break;
        case changingHeat:
          kitchen_s.heat.offHour = data;
          break;
        case changingLamp:
          kitchen_s.lamp.offHour = data;
          break;
      }
      break;

    case hall:
      switch (state) {
        case changingLight:
          hall_s.light.offHour = data;
          break;
        case changingHeat:
          hall_s.heat.offHour = data;
          break;
        case changingLamp:
          hall_s.lamp.offHour = data;
          break;
      }
      break;

    case livingRoom:
      switch (state) {
        case changingLight:
          livingRoom_s.light.offHour = data;
          break;
        case changingHeat:
          livingRoom_s.heat.offHour = data;
          break;
        case changingLamp:
          livingRoom_s.lamp.offHour = data;
          break;
      }
      break;

    case garden:
      switch (state) {
        case changingLamp:
          garden_s.lamp.offHour = data;
          break;
        case changingWater:
          garden_s.water.offHour = data;
          break;
      }
      break;

    case garage:
      switch (state) {
        case changingLight:
          garage_s.light.offHour = data;
          break;
        case changingLamp:
          garage_s.lamp.offHour = data;
          break;
      }
      break;
  }

}


void saveNewOnMins(byte data) {

  switch (lastHouseState) {

    case bedroom1:
      switch (state) {
        case changingLight:
          bed1_s.light.onMins = data;
          break;
        case changingHeat:
          bed1_s.heat.onMins = data;
          break;
        case changingLamp:
          bed1_s.lamp.onMins = data;
          break;
      }
      break;

    case bedroom2:
      switch (state) {
        case changingLight:
          bed2_s.light.onMins = data;
          break;
        case changingHeat:
          bed2_s.heat.onMins = data;
          break;
        case changingLamp:
          bed2_s.lamp.onMins = data;
          break;
      }
      break;

    case bathroom:
      switch (state) {
        case changingLight:
          bathroom_s.light.onMins = data;
          break;
        case changingHeat:
          bathroom_s.heat.onMins = data;
          break;
        case changingLamp:
          bathroom_s.lamp.onMins = data;
          break;
      }
      break;

    case kitchen:
      switch (state) {
        case changingLight:
          kitchen_s.light.onMins = data;
          break;
        case changingHeat:
          kitchen_s.heat.onMins = data;
          break;
        case changingLamp:
          kitchen_s.lamp.onMins = data;
          break;
      }
      break;

    case hall:
      switch (state) {
        case changingLight:
          hall_s.light.onMins = data;
          break;
        case changingHeat:
          hall_s.heat.onMins = data;
          break;
        case changingLamp:
          hall_s.lamp.onMins = data;
          break;
      }
      break;

    case livingRoom:
      switch (state) {
        case changingLight:
          livingRoom_s.light.onMins = data;
          break;
        case changingHeat:
          livingRoom_s.heat.onMins = data;
          break;
        case changingLamp:
          livingRoom_s.lamp.onMins = data;
          break;
      }
      break;

    case garden:
      switch (state) {
        case changingLamp:
          garden_s.lamp.onMins = data;
          break;
        case changingWater:
          garden_s.water.onMins = data;
          break;
      }
      break;

    case garage:
      switch (state) {
        case changingLight:
          garage_s.light.onMins = data;
          break;
        case changingLamp:
          garage_s.lamp.onMins = data;
          break;
      }
      break;
  }
}


void saveNewOffMins(byte data) {

  switch (lastHouseState) {

    case bedroom1:
      switch (state) {
        case changingLight:
          bed1_s.light.offMins = data;
          break;
        case changingHeat:
          bed1_s.heat.offMins = data;
          break;
        case changingLamp:
          bed1_s.lamp.offMins = data;
          break;
      }
      break;

    case bedroom2:
      switch (state) {
        case changingLight:
          bed2_s.light.offMins = data;
          break;
        case changingHeat:
          bed2_s.heat.offMins = data;
          break;
        case changingLamp:
          bed2_s.lamp.offMins = data;
          break;
      }
      break;

    case bathroom:
      switch (state) {
        case changingLight:
          bathroom_s.light.offMins = data;
          break;
        case changingHeat:
          bathroom_s.heat.offMins = data;
          break;
        case changingLamp:
          bathroom_s.lamp.offMins = data;
          break;
      }
      break;

    case kitchen:
      switch (state) {
        case changingLight:
          kitchen_s.light.offMins = data;
          break;
        case changingHeat:
          kitchen_s.heat.offMins = data;
          break;
        case changingLamp:
          kitchen_s.lamp.offMins = data;
          break;
      }
      break;

    case hall:
      switch (state) {
        case changingLight:
          hall_s.light.offMins = data;
          break;
        case changingHeat:
          hall_s.heat.offMins = data;
          break;
        case changingLamp:
          hall_s.lamp.offMins = data;
          break;
      }
      break;

    case livingRoom:
      switch (state) {
        case changingLight:
          livingRoom_s.light.offMins = data;
          break;
        case changingHeat:
          livingRoom_s.heat.offMins = data;
          break;
        case changingLamp:
          livingRoom_s.lamp.offMins = data;
          break;
      }
      break;

    case garden:
      switch (state) {
        case changingLamp:
          garden_s.lamp.offMins = data;
          break;
        case changingWater:
          garden_s.water.offMins = data;
          break;
      }
      break;

    case garage:
      switch (state) {
        case changingLight:
          garage_s.light.offMins = data;
          break;
        case changingLamp:
          garage_s.lamp.offMins = data;
          break;
      }
      break;
  }

}


void adjustLvl() {  /* set device level via UP/DOWN to ++/-- at given speed & SELECT to confirm */

  byte value;
  boolean confirmReady = false;
  boolean valueSet = false;

  switch (state) {    // get current level depending on room
    case changingLight:
      value = room.light.level;
      break;
    case changingHeat:
      value = room.heat.level;
      break;
    case changingLamp:
      value = room.lamp.level;
      break;
    case changingWater:
      value = room.water.level;
      break;
  }

  #ifdef DATA DEBUG
    Serial.print(F("\nInitial level: ")); Serial.println(value);
  #endif

  /* LCD DISPLAY - TOP ROW */
  lcd.home();
  lcd.print(leftLCD);
  lcd.setCursor(9, 0);
  lcd.print(F("ON: "));

  /*LCD DISPLAY - BOTTOM ROW */
  lcd.setCursor(0, 1);
  lcd.print(upLCD);
  lcd.print(downLCD);
  lcd.print(F(" [SEL] SET LVL"));

  while (value >= 0 && value <= 100 && !confirmReady) {

    int btnPress = lcd.readButtons();

    /* FORMAT VALUE DISPLAY */
    if (value < 10 && !valueSet) {  // only print if new values have not yet been set
      lcd.setCursor(13, 0);
      lcd.print(0); lcd.print(0); lcd.print(value);
    } else if (value < 100 && !valueSet) {
      lcd.setCursor(13, 0);
      lcd.print(0); lcd.print(value);
    } else if (value == 100 && !valueSet) {
      lcd.setCursor(13, 0);
      lcd.print(value);
    }

    switch (btnPress) {   // UP = increase level, DOWN = decrease level
      case BUTTON_DOWN:
        value--;
        delay(150);
        break;
      case BUTTON_UP:
        ++value;
        delay(150);
        break;
      case BUTTON_SELECT:
        confirmReady = true;
        lcd.clear();
        break;
    }
  }

  if (confirmReady) { // press select to save value to corresponding room
    valueSet = true;
    saveNewLvl(value);
    
    #ifdef DATA DEBUG     // ensures values are set after 
       room = getDeviceRoom();
       switch (state) {    
          case changingLight:
            Serial.print(F("Final level: ")); Serial.println(room.light.level);
            break;
          case changingHeat:
            Serial.print(F("Final level: ")); Serial.println(room.heat.level);
            break;
          case changingLamp:
            Serial.print(F("Final level: ")); Serial.println(room.lamp.level);        
            break;
          case changingWater:
            Serial.print(F("Final level: ")); Serial.println(room.water.level);
            break;
    }
    #endif
  }

}


void adjustOnHour() { /* similar to above and for following adjust functions - UP/DOWN to ++/-- onHour, SELECT to confirm */

  static byte value;
  static byte onMins;
  boolean confirmReady = false;
  boolean valueSet = false;
  room = getDeviceRoom();

  switch (state) {    // get current hour values & mins depending on room
    case changingLight:
      value = room.light.onHour;
      onMins = room.light.onMins;
      break;
    case changingHeat:
      value = room.heat.onHour;
      onMins = room.heat.onMins;
      break;
    case changingLamp:
      value = room.lamp.onHour;
      onMins = room.lamp.onMins;
      break;
    case changingWater:
      value = room.water.onHour;
      onMins = room.water.onMins;
      break;
  }

  #ifdef DATA DEBUG
    Serial.print(F("\nInitial onHour: ")); Serial.println(value);
  #endif

  /* LCD DISPLAY - TOP ROW */
  lcd.clear();
  lcd.home();
  lcd.print(leftLCD);
  lcd.setCursor(2, 0);
  lcd.print(F("ON TIME: "));

  /* LCD DISPLAY - BOTTOM ROW */
  lcd.setCursor(0, 1);
  lcd.print(upLCD);
  lcd.print(downLCD);
  lcd.print(F(" [SEL] SET HR"));

  while (value >= 0 && value < 24 && !valueSet) {

    int btnPress = lcd.readButtons();

    /* FORMAT VALUE DISPLAY */
    if (value < 10 && onMins < 10 && !valueSet) {
      lcd.setCursor(11, 0);
      lcd.print(F("0"));  lcd.print(value); lcd.print(F(".0"));  lcd.print(onMins);
    } else if (value > 10 && onMins < 10 && !valueSet) {
      lcd.setCursor(11, 0);
      lcd.print(value); lcd.print(F("."));  lcd.print(F("0")); lcd.print(onMins);
    } else if (value < 10 && onMins > 10 && !valueSet) {
      lcd.setCursor(11, 0);
      lcd.print(F("0"));  lcd.print(value); lcd.print(F(".")); lcd.print(onMins);
    } else {
      lcd.setCursor(11, 0);
      lcd.print(value); lcd.print(F(".")); lcd.print(onMins);
    }

    switch (btnPress) {   // UP = increase level, DOWN = decrease level
      case BUTTON_DOWN:
        value--;
        delay(150);
        break;
      case BUTTON_UP:
        ++value;
        delay(150);
        break;
      case BUTTON_SELECT:
        confirmReady = true;
        lcd.clear();
        break;
    }

    if (confirmReady) {   // press select to save value to corresponding room
      valueSet = true;
      saveNewOnHour(value);

      #ifdef DATA DEBUG 
       room = getDeviceRoom();
       switch (state) {    
          case changingLight:
            Serial.print(F("Final onHour: ")); Serial.println(room.light.onHour);
            break;
          case changingHeat:
            Serial.print(F("Final onHour: ")); Serial.println(room.heat.onHour);
            break;
          case changingLamp:
            Serial.print(F("Final onHour: ")); Serial.println(room.lamp.onHour);        
            break;
          case changingWater:
            Serial.print(F("Final onHour: ")); Serial.println(room.water.onHour);
            break;
      }
      #endif
    }

  }

}


void adjustOffHour() {

  static byte value;
  static byte offMins;
  boolean confirmReady = false;
  boolean valueSet = false;
  room = getDeviceRoom();

  switch (state) {
    case changingLight:
      value = room.light.offHour;
      offMins = room.light.offMins;
      break;
    case changingHeat:
      value = room.heat.offHour;
      offMins = room.heat.offMins;
      break;
    case changingLamp:
      value = room.lamp.offHour;
      offMins = room.lamp.offMins;
      break;
    case changingWater:
      value = room.water.offHour;
      offMins = room.water.offMins;
      break;
  }

  #ifdef DATA DEBUG
    Serial.print(F("\nInitial offHour: ")); Serial.println(value);
  #endif

  /* LCD DISPLAY - TOP ROW */
  lcd.clear();
  lcd.home();
  lcd.print(leftLCD);
  lcd.setCursor(2, 0);
  lcd.print(F("OFF TIME "));

  /* LCD DISPLAY - BOTTOM ROW */
  lcd.setCursor(0, 1);
  lcd.print(upLCD);
  lcd.print(downLCD);
  lcd.print(F(" [SEL] SET HR"));

  while (value >= 0 && value < 24 && !valueSet) {

    int btnPress = lcd.readButtons();

    /* FORMAT VALUE DISPLAY */
    if (value < 10 && offMins < 10 && !valueSet) {
      lcd.setCursor(11, 0);
      lcd.print(F("0"));  lcd.print(value); lcd.print(F(".0"));  lcd.print(offMins);
    } else if (value > 10 && offMins < 10 && !valueSet) {
      lcd.setCursor(11, 0);
      lcd.print(value); lcd.print(F("."));  lcd.print(F("0")); lcd.print(offMins);
    } else if (value < 10 && offMins > 10 && !valueSet) {
      lcd.setCursor(11, 0);
      lcd.print(F("0"));  lcd.print(value); lcd.print(F(".")); lcd.print(offMins);
    } else {
      lcd.setCursor(11, 0);
      lcd.print(value); lcd.print(F(".")); lcd.print(offMins);
    }
    switch (btnPress) {
      case BUTTON_DOWN:
        value--;
        delay(150);
        break;
      case BUTTON_UP:
        ++value;
        delay(150);
        break;
      case BUTTON_SELECT:
        confirmReady = true;
        lcd.clear();
        break;
    }

    if (confirmReady) {
      valueSet = true;
      saveNewOffHour(value);

      #ifdef DATA DEBUG 
       room = getDeviceRoom();
       switch (state) {    
          case changingLight:
            Serial.print(F("Final offHour: ")); Serial.println(room.light.offHour);
            break;
          case changingHeat:
            Serial.print(F("Final offHour: ")); Serial.println(room.heat.offHour);
            break;
          case changingLamp:
            Serial.print(F("Final offHour: ")); Serial.println(room.lamp.offHour);        
            break;
          case changingWater:
            Serial.print(F("Final offHour: ")); Serial.println(room.water.offHour);
            break;
      }
      #endif
    }

  }

}


void adjustOnMins() {

  static byte value;
  static byte onHour;
  boolean confirmReady = false;
  boolean valueSet = false;
  room = getDeviceRoom(); // get new hour from previous function to set onHour

  switch (state) {
    case changingLight:
      value = room.light.onMins;
      onHour = room.light.onHour;
      break;
    case changingHeat:
      value = room.heat.onMins;
      onHour = room.heat.onHour;
      break;
    case changingLamp:
      value = room.lamp.onMins;
      onHour = room.lamp.onHour;
      break;
    case changingWater:
      value = room.water.onMins;
      onHour = room.water.onHour;
      break;
  }

  #ifdef DATA DEBUG
    Serial.print(F("\nInitial onMins: ")); Serial.println(value);
  #endif

  /* LCD DISPLAY - TOP ROW */
  lcd.home();
  lcd.print(leftLCD);
  lcd.setCursor(2, 0);
  lcd.print(F("ON TIME: "));

  /*LCD DISPLAY - BOTTOM ROW */
  lcd.setCursor(0, 1);
  lcd.print(upLCD);
  lcd.print(downLCD);
  lcd.print(F(" [SEL] SET MINS"));

  while (value >= 0 && value < 60 && !valueSet) {

    int btnPress = lcd.readButtons();

    /* FORMAT VALUE DISPLAY */
    if (onHour < 10 && value < 10 && !valueSet) {
      lcd.setCursor(11, 0);
      lcd.print(F("0"));  lcd.print(onHour); lcd.print(F(".0"));  lcd.print(value);
    } else if (onHour > 10 && value < 10 && !valueSet) {
      lcd.setCursor(11, 0);
      lcd.print(onHour); lcd.print(F("."));  lcd.print(F("0")); lcd.print(value);
    } else if (onHour < 10 && value >= 10 && !valueSet) {
      lcd.setCursor(11, 0);
      lcd.print(F("0"));  lcd.print(onHour); lcd.print(F(".")); lcd.print(value);
    } else {
      lcd.setCursor(11, 0);
      lcd.print(onHour); lcd.print(F(".")); lcd.print(value);
    }

    switch (btnPress) {
      case BUTTON_DOWN:
        value--;
        delay(150);
        break;
      case BUTTON_UP:
        ++value;
        delay(150);
        break;
      case BUTTON_SELECT:
        confirmReady = true;
        lcd.clear();
        break;
    }

    if (confirmReady) {
      saveNewOnMins(value);
      valueSet = true;

      #ifdef DATA DEBUG 
       room = getDeviceRoom();
       switch (state) {    
          case changingLight:
            Serial.print(F("Final onMins: ")); Serial.println(room.light.onMins);
            break;
          case changingHeat:
            Serial.print(F("Final onMins: ")); Serial.println(room.heat.onMins);
            break;
          case changingLamp:
            Serial.print(F("Final onMins: ")); Serial.println(room.lamp.onMins);        
            break;
          case changingWater:
            Serial.print(F("Final onMins: ")); Serial.println(room.water.onMins);
            break;
      }
      #endif
    }

  }

  lcd.clear();

}


void adjustOffMins() {

  static byte value;
  static byte offHour;
  boolean confirmReady = false;
  boolean valueSet = false;
  room = getDeviceRoom();

  switch (state) {
    case changingLight:
      value = room.light.offMins;
      offHour = room.light.offHour;
      break;
    case changingHeat:
      value = room.heat.offMins;
      offHour = room.heat.offHour;
      break;
    case changingLamp:
      value = room.lamp.offMins;
      offHour = room.lamp.offHour;
      break;
    case changingWater:
      value = room.water.offMins;
      offHour = room.water.offHour;
      break;
  }

  #ifdef DATA DEBUG
    Serial.print(F("\nInitial offMins: ")); Serial.println(value);
  #endif

  /* LCD DISPLAY - TOP ROW */
  lcd.home();
  lcd.print(leftLCD);
  lcd.setCursor(2, 0);
  lcd.print(F("OFF TIME "));

  /*LCD DISPLAY - BOTTOM ROW */
  lcd.setCursor(0, 1);
  lcd.print(upLCD);
  lcd.print(downLCD);
  lcd.print(F(" [SEL] SET MINS"));

  while (value >= 0 && value < 60 && !valueSet) {

    int btnPress = lcd.readButtons();

    /* FORMAT VALUE DISPLAY */
    if (offHour < 10 && value < 10 && !valueSet) {
      lcd.setCursor(11, 0);
      lcd.print(F("0"));  lcd.print(offHour); lcd.print(F(".0"));  lcd.print(value);
    } else if (offHour > 10 && value < 10 && !valueSet) {
      lcd.setCursor(11, 0);
      lcd.print(offHour); lcd.print(F("."));  lcd.print(F("0")); lcd.print(value);
    } else if (offHour < 10 && value >= 10 && !valueSet) {
      lcd.setCursor(11, 0);
      lcd.print(F("0"));  lcd.print(offHour); lcd.print(F(".")); lcd.print(value);
    } else {
      lcd.setCursor(11, 0);
      lcd.print(offHour); lcd.print(F(".")); lcd.print(value);
    }

    switch (btnPress) {
      case BUTTON_DOWN:
        value--;
        delay(150);
        break;
      case BUTTON_UP:
        ++value;
        delay(150);
        break;
      case BUTTON_SELECT:
        confirmReady = true;
        lcd.clear();
        break;
    }

    if (confirmReady) {
      saveNewOffMins(value);
      valueSet = true;

      #ifdef DATA DEBUG 
       room = getDeviceRoom();
       switch (state) {    
          case changingLight:
            Serial.print(F("Final offMins: ")); Serial.println(room.light.offMins);
            break;
          case changingHeat:
            Serial.print(F("Final offMins: ")); Serial.println(room.heat.offMins);
            break;
          case changingLamp:
            Serial.print(F("Final offMins: ")); Serial.println(room.lamp.offMins);        
            break;
          case changingWater:
            Serial.print(F("Final offMins: ")); Serial.println(room.water.offMins);
            break;
      }
      #endif
    }

  }

  lcd.clear();

}



// HOUSE STATE FUNCTIONS:

void startupState() {   /* initial state */

  currentBtnState = lcd.readButtons();
  unsigned long currentTime = millis();

  /* LCD DISPLAY */
  lcd.setBacklight(WHITE);
  lcd.home();
  lcd.print(F("Select a floor.."));
  lcd.setCursor(0, 1);
  if (currentTime - previousTime > 1500) {
    previousTime = currentTime;
    if (blankOnOff) {
      lcd.print(F("[SEL]  SEND DATA"));
    } else {
      lcd.print(upLCD);
      lcd.print(F(" 1 GRND "));
      lcd.print(downLCD);
      lcd.print(F(" OUT "));
      lcd.print(rightLCD);
    }
    lcd.print(F("                   "));
    blankOnOff = !blankOnOff;
  }

  /* BUTTON PRESSES: UP = 1st floor,  DOWN = Ground Floor, RIGHT = Outside, SELECT = data print to serial monitor */

  if (isSinglePress(BUTTON_UP)) {
    state = firstFloor;
    location += "First/";
    lcd.clear();  // prepare for next state
    #ifdef DEBUG
      Serial.print(F("\nUP (x1) - Entering 1st floor --> ")); Serial.println(location);
      Serial.println(F("Current state: FIRST FLOOR"));
    #endif
  }

  if (isSinglePress(BUTTON_DOWN)) {
    state = groundFloor;
    location += "Ground/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nDOWN (x1) - Entering Ground floor --> ")); Serial.println(location);
      Serial.println(F("Current state: GROUND FLOOR"));
    #endif
  }

  if (isSinglePress(BUTTON_RIGHT)) {
    state = outside;
    location += "Outside/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nRIGHT (x1) - Going Outside --> ")); Serial.println(location);
      Serial.println(F("Current state: OUTSIDE"));
    #endif
  }

  if (isSinglePress(BUTTON_SELECT)) {
    lcd.clear();
    lcd.print(F("Loading rooms..."));
    #ifndef DEBUG
      lcd.setCursor(0, 1);
      for (int i = 0; i < 17; i++) {
        lcd.print(fullPxLCD);
        delay(50);
      }
    #endif
    printAllData();
    lcd.clear();
  }

  lastBtnState = currentBtnState;

}


void firstFloorState() {

  currentBtnState = lcd.readButtons();
  state = firstFloor;

  /* LCD DISPLAY - TOP ROW */
  lcd.setBacklight(BLUE);
  lcd.home();
  #ifndef DEBUG
    print1By1(0, "   FIRST FLOOR", 120); // occurs once
    unsigned long currentTime = millis();
    lcd.home();
    lcd.print(leftLCD);
    if (millis() > 7500) { // after 7.5secs, top row prints row so user can see name after leaving & re-entering
      lcd.home();
      lcd.print(leftLCD);
      lcd.print(F("  FIRST FLOOR"));
    }
  #else
    unsigned long currentTime = millis();  // when debugging, remove 1by1 print animation for more speed
    lcd.print(leftLCD);
    lcd.print(F("  FIRST FLOOR"));
  #endif
  
  /* LCD DIPLAY - BOTTOM ROW */
  lcd.setCursor(0, 1);

  if (currentTime - previousTime > 1500) {  // blinking options alternate every 1.5secs
    previousTime = currentTime;
    if (blankOnOff) {
      lcd.print(downLCD);
      lcd.print(F(" BED 1  BED 2 "));
      lcd.print(rightLCD);
    } else {
      lcd.print(F(" [SEL] BATHROOM"));
    }
    lcd.print(F("                   "));
    blankOnOff = !blankOnOff;
  }

  /* BUTTON PRESSES: LEFT = previous, DOWN = bed 1, RIGHT = bed 2, SELECT = bathroom */

  if (isSinglePress(BUTTON_LEFT)) {
    state = startup;
    location = ""; // reset room path
    lcd.clear();
    #ifdef DEBUG
      Serial.println(F("\nLEFT (x1) - Back to startup --> "));
      Serial.println(F("Current State: STARTUP"));
    #endif
  }

  if (isSinglePress(BUTTON_DOWN)) {
    state = bedroom1;
    location += "Bedroom 1/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nDOWN (x1) - Entering Bedroom 1 --> ")); Serial.println(location);
      Serial.println(F("Current State: BEDROOM 1"));
    #endif
  }

  if (isSinglePress(BUTTON_RIGHT)) {
    state = bedroom2;
    location += "Bedroom 2/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nRIGHT (x1) - Entering Bedroom 2 --> ")); Serial.println(location);
      Serial.println(F("Current State: BEDROOM 2"));
    #endif
  }

  if (isSinglePress(BUTTON_SELECT)) {
    state = bathroom;
    location += "Bathroom/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nSELECT (x1) - Entering Bathroom --> ")); Serial.println(location);
      Serial.println(F("Current State: BATHROOM"));
    #endif
  }

  lastBtnState = currentBtnState;

}


void groundFloorState() {   /* same structure as firstFloorState() */

  currentBtnState = lcd.readButtons();
  state = groundFloor;

  /* LCD DISPLAY - TOP ROW */
  lcd.setBacklight(VIOLET);
  lcd.home();
  #ifndef DEBUG
    print1By1(0, "  GROUND  FLOOR", 120);
    unsigned long currentTime = millis();
    lcd.home();
    lcd.print(leftLCD);
    if (millis() > 7500) {
      lcd.home();
      lcd.print(leftLCD);
      lcd.print(F(" GROUND  FLOOR"));
    }
  #else
    unsigned long currentTime = millis();
    lcd.print(leftLCD);
    lcd.print(F(" GROUND  FLOOR"));
  #endif
  
  /* LCD DIPLAY - BOTTOM ROW */
  lcd.setCursor(0, 1);

  if (currentTime - previousTime > 1500) {
    previousTime = currentTime;
    if (blankOnOff) {
      lcd.print(downLCD);
      lcd.print(F(" KITCHEN/HALL "));
      lcd.print(rightLCD);
    } else {
      lcd.print(F("[SEL]  LIVING RM"));
    }
    lcd.print(F("                   "));
    blankOnOff = !blankOnOff;
  }

  /* BUTTON PRESSES: LEFT = previous, DOWN = kitchen, RIGHT = hall, SELECT = living room */

  if (isSinglePress(BUTTON_LEFT)) {
    state = startup;
    location = "";
    lcd.clear();
    #ifdef DEBUG
      Serial.println(F("\nLEFT (x1) - Back to startup --> "));
      Serial.println(F("Current State: STARTUP"));
    #endif;
  }

  if (isSinglePress(BUTTON_DOWN)) {
    state = kitchen;
    location += "Kitchen/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nDOWN (x1) - Entering Kitchen --> ")); Serial.println(location);
      Serial.println(F("Current State: KITCHEN"));
    #endif
  }

  if (isSinglePress(BUTTON_RIGHT)) {
    state = hall;
    location += "Hall/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nRIGHT (x1) - Entering Hall --> ")); Serial.println(location);
      Serial.println(F("Current State: HALL"));
    #endif
  }

  if (isSinglePress(BUTTON_SELECT)) {
    state = livingRoom;
    location += "Living Room/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nSELECT (x1) - Entering Living Room --> ")); Serial.println(location);
      Serial.println(F("Current State: LIVING ROOM"));
    #endif
  }

  lastBtnState = currentBtnState;

}


void outsideState() {

  currentBtnState = lcd.readButtons();
  state = outside;

  /* LCD DISPLAY - TOP ROW */
  lcd.setBacklight(TEAL);
  lcd.home();
  #ifndef DEBUG
    print1By1(0, "     OUTSIDE", 120);
    unsigned long currentTime = millis();
    lcd.home();
    lcd.print(leftLCD);
    if (millis() > 7500) {
      lcd.home();
      lcd.print(leftLCD);
      lcd.print(F("    OUTSIDE"));
    }
  #else
    unsigned long currentTime = millis();
    lcd.print(leftLCD);
    lcd.print(F("    OUTSIDE"));
  #endif

  /* LCD DIPLAY - BOTTOM ROW */
  lcd.setCursor(0, 1);
  lcd.print(downLCD);
  lcd.print(F("GARDEN  GARAGE"));
  lcd.print(rightLCD);

  /* BUTTON PRESSES: LEFT = previous, DOWN = kitchen, RIGHT = hall, SELECT = living room */

  if (isSinglePress(BUTTON_LEFT)) {
    state = startup;
    location = "";
    lcd.clear();
    #ifdef DEBUG
      Serial.println(F("\nLEFT (x1) - Back to startup --> "));
      Serial.println(F("Current State: STARTUP"));
    #endif
  }

  if (isSinglePress(BUTTON_DOWN)) {
    state = garden;
    location += "Garden/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nDOWN (x1) - Entering Garden --> ")); Serial.println(location);
      Serial.println(F("Current State: GARDEN"));
    #endif
  }

  if (isSinglePress(BUTTON_RIGHT)) {
    state = garage;
    location += "Garage/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nRIGHT (x1) - Entering Garage --> ")); Serial.println(location);
      Serial.println(F("Current State: GARAGE"));
    #endif
  }

  lastBtnState = currentBtnState;

}


void bedroom1State() {  /* similar structure for all other rooms */

  currentBtnState = lcd.readButtons();
  state = bedroom1;

  /* LCD DISPLAY - TOP ROW */
  lcd.setBacklight(BLUE);
  lcd.home();
  lcd.print(leftLCD);
  lcd.print(F("   BEDROOM 1"));

  /* LCD DIPLAY - BOTTOM ROW */
  showRoomDevices(1, 1500);

  /* BUTTON PRESSES: LEFT = previous, UP = light adjust, DOWN = heat adjust, RIGHT = lamp adjust */

  if (isSinglePress(BUTTON_LEFT)) {
    state = firstFloor;
    location = "";
    location += "First/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nLEFT (x1) - Back to 1st floor --> ")); Serial.println(location);
      Serial.println(F("Current State: FIRST FLOOR"));
    #endif
  }

  if (isSinglePress(BUTTON_UP)) {
    state = changingLight;
    location += "Light/Main/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nUP (x1) - Adjusting light settings --> ")); Serial.println(location);
      Serial.println(F("Current State: CHANGING LIGHT"));
    #endif
  }

  if (isSinglePress(BUTTON_DOWN)) {
    state = changingHeat;
    location += "Heat/Main/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nDOWN (x1) - Adjusting heat settings --> ")); Serial.println(location);
      Serial.println(F("Current State: CHANGING HEAT"));
    #endif
  }

  if (isSinglePress(BUTTON_RIGHT)) {
    state = changingLamp;
    location += "Lamp/Desk/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nRIGHT (x1) - Adjusting lamp settings --> ")); Serial.println(location);
      Serial.println(F("Current State: CHANGING LAMP"));
    #endif
  }

  lastBtnState = currentBtnState;
  lastHouseState = bedroom1;

}


void bedroom2State() {

  currentBtnState = lcd.readButtons();
  state = bedroom2;

  /* LCD DISPLAY - TOP ROW */
  lcd.setBacklight(BLUE);
  lcd.home();
  lcd.print(leftLCD);
  lcd.print(F("   BEDROOM 2"));

  /* LCD DIPLAY - BOTTOM ROW */
  showRoomDevices(1, 1500);

  /* BUTTON PRESSES: LEFT = previous, UP = light adjust, DOWN = heat adjust, RIGHT = lamp adjust */

  if (isSinglePress(BUTTON_LEFT)) {
    state = firstFloor;
    location = "";
    location += "First/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nLEFT (x1) - Back to 1st floor --> ")); Serial.println(location);
      Serial.println(F("Current State: FIRST FLOOR"));
    #endif
  }

  if (isSinglePress(BUTTON_UP)) {
    state = changingLight;
    location += "Light/Main/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nUP (x1) - Adjusting light settings --> ")); Serial.println(location);
      Serial.println(F("Current State: CHANGING LIGHT"));
    #endif
  }

  if (isSinglePress(BUTTON_DOWN)) {
    state = changingHeat;
    location += "Heat/Main/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nDOWN (x1) - Adjusting heat settings --> ")); Serial.println(location);
      Serial.println(F("Current State: CHANGING HEAT"));
    #endif
  }

  if (isSinglePress(BUTTON_RIGHT)) {
    state = changingLamp;
    location += "Lamp/Desk/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nRIGHT (x1) - Adjusting lamp settings --> ")); Serial.println(location);
      Serial.println(F("Current State: CHANGING LAMP"));
    #endif
  }

  lastBtnState = currentBtnState;
  lastHouseState = bedroom2;

}


void bathroomState() {

  currentBtnState = lcd.readButtons();
  state = bathroom;

  /* LCD DISPLAY - TOP ROW */
  lcd.setBacklight(BLUE);
  lcd.home();
  lcd.print(leftLCD);
  lcd.print(F("   BATHROOM"));

  /* LCD DIPLAY - BOTTOM ROW */
  showRoomDevices(1, 1500);

  /* BUTTON PRESSES: LEFT = previous, UP = light adjust, DOWN = heat adjust, RIGHT = lamp adjust */

  if (isSinglePress(BUTTON_LEFT)) {
    state = firstFloor;
    location = "";
    location += "First/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nLEFT (x1) - Back to 1st floor --> ")); Serial.println(location);
      Serial.println(F("Current State: FIRST FLOOR"));
    #endif
  }

  if (isSinglePress(BUTTON_UP)) {
    state = changingLight;
    location += "Light/Main/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nUP (x1) - Adjusting light settings --> ")); Serial.println(location);
      Serial.println(F("Current State: CHANGING LIGHT"));
    #endif
  }

  if (isSinglePress(BUTTON_DOWN)) {
    state = changingHeat;
    location += "Heat/Main/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nDOWN (x1) - Adjusting heat settings --> ")); Serial.println(location);
      Serial.println(F("Current State: CHANGING HEAT"));
    #endif
  }

  if (isSinglePress(BUTTON_RIGHT)) {
    state = changingLamp;
    location += "Lamp/Mirror/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nRIGHT (x1) - Adjusting lamp settings --> ")); Serial.println(location);
      Serial.println(F("Current State: CHANGING LAMP"));
    #endif
  }

  lastBtnState = currentBtnState;
  lastHouseState = bathroom;

}


void kitchenState() {

  currentBtnState = lcd.readButtons();
  state = kitchen;

  /* LCD DISPLAY - TOP ROW */
  lcd.setBacklight(VIOLET);
  lcd.home();
  lcd.print(leftLCD);
  lcd.print(F("    KITCHEN"));

  /* LCD DIPLAY - BOTTOM ROW */
  showRoomDevices(1, 1500);

  /* BUTTON PRESSES: LEFT = previous, UP = light adjust, DOWN = heat adjust, RIGHT = lamp adjust */

  if (isSinglePress(BUTTON_LEFT)) {
    state = groundFloor;
    location = "";
    location += "Ground/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nLEFT (x1) - Back to Ground floor --> ")); Serial.println(location);
      Serial.println(F("Current State: GROUND FLOOR"));
    #endif
  }

  if (isSinglePress(BUTTON_UP)) {
    state = changingLight;
    location += "Light/Main/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nUP (x1) - Adjusting light settings --> ")); Serial.println(location);
      Serial.println(F("Current State: CHANGING LIGHT"));
    #endif
  }

  if (isSinglePress(BUTTON_DOWN)) {
    state = changingHeat;
    location += "Heat/Main/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nDOWN (x1) - Adjusting heat settings --> ")); Serial.println(location);
      Serial.println(F("Current State: CHANGING HEAT"));
    #endif
  }

  if (isSinglePress(BUTTON_RIGHT)) {
    state = changingLamp;
    location += "Lamp/Ceiling/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nRIGHT (x1) - Adjusting lamp settings --> ")); Serial.println(location);
      Serial.println(F("Current State: CHANGING LAMP"));
    #endif
  }

  lastBtnState = currentBtnState;
  lastHouseState = kitchen;

}


void hallState() {

  currentBtnState = lcd.readButtons();
  state = hall;

  /* LCD DISPLAY - TOP ROW */
  lcd.setBacklight(VIOLET);
  lcd.home();
  lcd.print(leftLCD);
  lcd.print(F("     HALL"));

  /* LCD DIPLAY - BOTTOM ROW */
  showRoomDevices(1, 1500);

  /* BUTTON PRESSES: LEFT = previous, UP = light adjust, DOWN = heat adjust, RIGHT = lamp adjust */

  if (isSinglePress(BUTTON_LEFT)) {
    state = groundFloor;
    location = "";
    location += "Ground/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nLEFT (x1) - Back to Ground floor --> ")); Serial.println(location);
      Serial.println(F("Current State: GROUND FLOOR"));
    #endif
  }

  if (isSinglePress(BUTTON_UP)) {
    state = changingLight;
    location += "Light/Main/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nUP (x1) - Adjusting light settings --> ")); Serial.println(location);
      Serial.println(F("Current State: CHANGING LIGHT"));
    #endif
  }

  if (isSinglePress(BUTTON_DOWN)) {
    state = changingHeat;
    location += "Heat/Main/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nDOWN (x1) - Adjusting heat settings --> ")); Serial.println(location);
      Serial.println(F("Current State: CHANGING HEAT"));
    #endif
  }

  if (isSinglePress(BUTTON_RIGHT)) {
    state = changingLamp;
    location += "Lamp/Floor/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nRIGHT (x1) - Adjusting lamp settings --> ")); Serial.println(location);
      Serial.println(F("Current State: CHANGING LAMP"));
    #endif
  }

  lastBtnState = currentBtnState;
  lastHouseState = hall;

}


void livingRoomState() {

  currentBtnState = lcd.readButtons();
  state = livingRoom;

  /* LCD DISPLAY - TOP ROW */
  lcd.setBacklight(VIOLET);
  lcd.home();
  lcd.print(leftLCD);
  lcd.print(F("  LIVING ROOM"));

  /* LCD DIPLAY - BOTTOM ROW */
  showRoomDevices(1, 1500);

  /* BUTTON PRESSES: LEFT = previous, UP = light adjust, DOWN = heat adjust, RIGHT = lamp adjust */

  if (isSinglePress(BUTTON_LEFT)) {
    state = groundFloor;
    location = "";
    location += "Ground/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nLEFT (x1) - Back to Ground floor --> ")); Serial.println(location);
      Serial.println(F("Current State: GROUND FLOOR"));
    #endif
  }

  if (isSinglePress(BUTTON_UP)) {
    state = changingLight;
    location += "Light/Main/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nUP (x1) - Adjusting light settings --> ")); Serial.println(location);
      Serial.println(F("Current State: CHANGING LIGHT"));
    #endif
  }

  if (isSinglePress(BUTTON_DOWN)) {
    state = changingHeat;
    location += "Heat/Main/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nDOWN (x1) - Adjusting heat settings --> ")); Serial.println(location);
      Serial.println(F("Current State: CHANGING HEAT"));
    #endif
  }

  if (isSinglePress(BUTTON_RIGHT)) {
    state = changingLamp;
    location += "Lamp/Table/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nRIGHT (x1) - Adjusting lamp settings --> ")); Serial.println(location);
      Serial.println(F("Current State: CHANGING LAMP"));
    #endif
  }

  lastBtnState = currentBtnState;
  lastHouseState = livingRoom;

}


void gardenState() {

  currentBtnState = lcd.readButtons();
  state = garden;

  /* LCD DISPLAY - TOP ROW */
  lcd.setBacklight(TEAL);
  lcd.home();
  lcd.print(leftLCD);
  lcd.print(F("    GARDEN"));

  /* LCD DIPLAY - BOTTOM ROW */
  lcd.setCursor(0, 1);
  lcd.print(upLCD);
  lcd.print(F(" WATER   LAMP "));
  lcd.print(rightLCD);

  /* BUTTON PRESSES: LEFT = previous, UP = light adjust, DOWN = heat adjust, RIGHT = lamp adjust */

  if (isSinglePress(BUTTON_LEFT)) {
    state = outside;
    location = "";
    location += "Outside/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nLEFT (x1) - Back outside --> ")); Serial.println(location);
      Serial.println(F("Current State: OUTSIDE"));
    #endif
  }

  if (isSinglePress(BUTTON_UP)) {
    state = changingWater;
    location += "Water/Main/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nUP (x1) - Adjusting water settings --> ")); Serial.println(location);
      Serial.println(F("Current State: CHANGING WATER"));
    #endif
  }

  if (isSinglePress(BUTTON_RIGHT)) {
    state = changingLamp;
    location += "Lamp/String/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nRIGHT (x1) - Adjusting lamp settings --> ")); Serial.println(location);
      Serial.println(F("Current State: CHANGING LAMP"));
    #endif
  }

  lastBtnState = currentBtnState;
  lastHouseState = garden;

}


void garageState() {

  currentBtnState = lcd.readButtons();
  state = garage;

  /* LCD DISPLAY - TOP ROW */
  lcd.setBacklight(TEAL);
  lcd.home();
  lcd.print(leftLCD);
  lcd.print(F("    GARAGE"));

  /* LCD DIPLAY - BOTTOM ROW */
  lcd.setCursor(0, 1);
  lcd.print(upLCD);
  lcd.print(F(" LIGHT   LAMP "));
  lcd.print(rightLCD);

  /* BUTTON PRESSES: LEFT = previous, UP = light adjust, DOWN = heat adjust, RIGHT = lamp adjust */

  if (isSinglePress(BUTTON_LEFT)) {
    state = outside;
    location = "";
    location += "Outside/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nLEFT (x1) - Back Outside --> ")); Serial.println(location);
      Serial.println(F("Current State: OUTSIDE"));
    #endif
  }

  if (isSinglePress(BUTTON_UP)) {
    state = changingLight;
    location += "Light/Main/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nUP (x1) - Adjusting light settings --> ")); Serial.println(location);
      Serial.println(F("Current State: CHANGING LIGHT"));
    #endif
  }

  if (isSinglePress(BUTTON_RIGHT)) {
    state = changingLamp;
    location += "Lamp/Halogen/";
    lcd.clear();
    #ifdef DEBUG
      Serial.print(F("\nRIGHT (x1) - Adjusting lamp settings --> ")); Serial.println(location);
      Serial.println(F("Current State: CHANGING LAMP"));
    #endif
  }

  lastBtnState = currentBtnState;
  lastHouseState = garage;

}


void changingLightState() {   /* set values of level, ontime & offtime values - same structure for other device-changing states */

  currentBtnState = lcd.readButtons();
  state = changingLight;
  room = getDeviceRoom();         // get current values

  /* LCD DISPLAY - TOP ROW */
  lcd.setBacklight(YELLOW);
  lcd.home();
  lcd.print(leftLCD);

  if (room.light.level == 0) {     // CASE 1: light is off --> set level & schedule on time
    /* LCD DISPLAY - BOTTOM ROW */
    lcd.setCursor(13, 0);
    lcd.print(F("OFF"));
    lcd.setCursor(0, 1);
    lcd.print(F("[SEL]  LVL/TIMES"));

    /* BUTTON PRESSES: LEFT = previous, 1st SELECT = turn on, 2nd SELECT = confirm level, 3rd SELECT = confirm hour, 4th SELECT = confirm mins */

    if (isSinglePress(BUTTON_SELECT)) {
      room.light.level = 1;
      adjustLvl();
      adjustOnHour();
      adjustOnMins();
    }

  } else {                        // CASE 2: light is on --> set level & schedule off time
    if (room.light.level > 99) {
      lcd.setCursor(9, 0);
      lcd.print(F("ON: ")); lcd.print(room.light.level);
    } else if (room.light.level < 10) {
      lcd.setCursor(9, 0);
      lcd.print(F("ON: ")); lcd.print(F("00"));  lcd.print(room.light.level);
    } else if (room.light.level >= 10 && room.light.level <= 99) {
      lcd.setCursor(9, 0);
      lcd.print(F("ON: ")); lcd.print(F("0"));  lcd.print(room.light.level);
    }
    lcd.setCursor(0, 1);
    lcd.print(F("[SEL]  LVL/TIMES"));

    if (isSinglePress(BUTTON_SELECT)) {
      adjustLvl();
      adjustOffHour();
      adjustOffMins();
    }
  }


  if (isSinglePress(BUTTON_LEFT)) {
    state = lastHouseState;
    lcd.clear();
    #ifdef DEBUG
      previousStateDebug();
    #endif
  }

  lastBtnState = currentBtnState;

}


void changingHeatState() {

  currentBtnState = lcd.readButtons();
  state = changingHeat;
  room = getDeviceRoom();

  /* LCD DISPLAY - TOP ROW */
  lcd.setBacklight(RED);
  lcd.home();
  lcd.print(leftLCD);

  if (room.heat.level == 0) {
    /* LCD DISPLAY - BOTTOM ROW */
    lcd.setCursor(13, 0);
    lcd.print(F("OFF"));
    lcd.setCursor(0, 1);
    lcd.print(F("[SEL]  LVL/TIMES"));

    /* BUTTON PRESSES: LEFT = previous, 1st SELECT = turn on, 2nd SELECT = confirm level, 3rd SELECT = confirm hour, 4th SELECT = confirm mins */

    if (isSinglePress(BUTTON_SELECT)) {
      room.heat.level = 1;
      adjustLvl();
      adjustOnHour();
      adjustOnMins();
    }

  } else {
    if (room.heat.level > 99) {
      lcd.setCursor(9, 0);
      lcd.print(F("ON: ")); lcd.print(room.heat.level);
    } else if (room.heat.level < 10) {
      lcd.setCursor(9, 0);
      lcd.print(F("ON: ")); lcd.print(F("00"));  lcd.print(room.heat.level);
    } else if (room.heat.level >= 10 && room.heat.level <= 99) {
      lcd.setCursor(9, 0);
      lcd.print(F("ON: ")); lcd.print(F("0"));  lcd.print(room.heat.level);
    }
    lcd.setCursor(0, 1);
    lcd.print(F("[SEL]  LVL/TIMES"));

    if (isSinglePress(BUTTON_SELECT)) {
      adjustLvl();
      adjustOffHour();
      adjustOffMins();
    }
  }


  if (isSinglePress(BUTTON_LEFT)) {
    state = lastHouseState;
    lcd.clear();
    #ifdef DEBUG
      previousStateDebug();
    #endif
  }

  lastBtnState = currentBtnState;

}


void changingLampState() {

  currentBtnState = lcd.readButtons();
  state = changingLamp;
  room = getDeviceRoom();

  /* LCD DISPLAY - TOP ROW */
  lcd.setBacklight(GREEN);
  lcd.home();
  lcd.print(leftLCD);

  if (room.lamp.level == 0) {
    /* LCD DISPLAY - BOTTOM ROW */
    lcd.setCursor(13, 0);
    lcd.print(F("OFF"));
    lcd.setCursor(0, 1);
    lcd.print(F("[SEL]  LVL/TIMES"));

    /* BUTTON PRESSES: LEFT = previous, 1st SELECT = turn on, 2nd SELECT = confirm level, 3rd SELECT = confirm hour, 4th SELECT = confirm mins */

    if (isSinglePress(BUTTON_SELECT)) {
      room.lamp.level = 1;
      adjustLvl();
      adjustOnHour();
      adjustOnMins();
    }

  } else {
    if (room.lamp.level > 99) {
      lcd.setCursor(9, 0);
      lcd.print(F("ON: ")); lcd.print(room.lamp.level);
    } else if (room.lamp.level < 10) {
      lcd.setCursor(9, 0);
      lcd.print(F("ON: ")); lcd.print(F("00"));  lcd.print(room.lamp.level);
    } else if (room.lamp.level >= 10 && room.lamp.level <= 99) {
      lcd.setCursor(9, 0);
      lcd.print(F("ON: ")); lcd.print(F("0"));  lcd.print(room.lamp.level);
    }
    lcd.setCursor(0, 1);
    lcd.print(F("[SEL]  LVL/TIMES"));

    if (isSinglePress(BUTTON_SELECT)) {
      adjustLvl();
      adjustOffHour();
      adjustOffMins();
    }
  }


  if (isSinglePress(BUTTON_LEFT)) {
    state = lastHouseState;
    lcd.clear();
    #ifdef DEBUG
      previousStateDebug();
    #endif
  }

  lastBtnState = currentBtnState;

}


void changingWaterState() {

  currentBtnState = lcd.readButtons();
  state = changingWater;
  room = getDeviceRoom();

  /* LCD DISPLAY - TOP ROW */
  lcd.setBacklight(TEAL);
  lcd.home();
  lcd.print(leftLCD);

  if (room.water.level == 0) {
    /* LCD DISPLAY - BOTTOM ROW */
    lcd.setCursor(13, 0);
    lcd.print(F("OFF"));
    lcd.setCursor(0, 1);
    lcd.print(F("[SEL]  LVL/TIMES"));

    /* BUTTON PRESSES: LEFT = previous, 1st SELECT = turn on, 2nd SELECT = confirm level, 3rd SELECT = confirm hour, 4th SELECT = confirm mins */

    if (isSinglePress(BUTTON_SELECT)) {
      room.water.level = 1;
      adjustLvl();
      adjustOnHour();
      adjustOnMins();
    }

  } else {
    if (room.water.level > 99) {
      lcd.setCursor(9, 0);
      lcd.print(F("ON: ")); lcd.print(room.water.level);
    } else if (room.water.level < 10) {
      lcd.setCursor(9, 0);
      lcd.print(F("ON: ")); lcd.print(F("00"));  lcd.print(room.water.level);
    } else if (room.water.level >= 10 && room.water.level <= 99) {
      lcd.setCursor(9, 0);
      lcd.print(F("ON: ")); lcd.print(F("0"));  lcd.print(room.water.level);
    }
    lcd.setCursor(0, 1);
    lcd.print(F("[SEL]  LVL/TIMES"));

    if (isSinglePress(BUTTON_SELECT)) {
      adjustLvl();
      adjustOffHour();
      adjustOffMins();
    }
  }


  if (isSinglePress(BUTTON_LEFT)) {
    state = lastHouseState;
    lcd.clear();
    #ifdef DEBUG
      previousStateDebug();
    #endif
  }

  lastBtnState = currentBtnState;

}




//===========================================================================================================


void setup() {

  lcd.begin(16, 2);
  Serial.begin(9600);
  delay(150);         // prevent duplicate Serial print on startup
  createBtnChars();
  initialiseData();
  Serial.println(F("ENHANCED: LAMP, OUTSIDE, QUERY, MEMORY"));
  #ifdef DEBUG
    Serial.println(F("\nCurrent state: STARTUP"));
  #endif
}

void loop() {

  char input = Serial.read();
  
  if (input == 'Q')   // QUERY EXTENSION
    printAllData();

  else if (input == 'M') {   // MEMORY EXTENSION
    Serial.print(F("\nAvailable memory: ")); Serial.print(freeMemory());  Serial.println(F(" bytes"));
  }

  else if (input == 'R' && state == changingLight) { 
    Serial.print('\n' + location); Serial.print(F("Level: ")); Serial.println(room.light.level);
    Serial.print(location); Serial.print(F("On: "));    Serial.print(room.light.onHour);  Serial.print(F(".")); Serial.println(room.light.onMins);
    Serial.print(location); Serial.print(F("Off: "));   Serial.print(room.light.offHour); Serial.print(F(".")); Serial.println(room.light.offMins);
  } else if (input == 'R' && state == changingHeat) {
    Serial.print('\n' + location); Serial.print(F("Level: ")); Serial.println(room.heat.level);
    Serial.print(location); Serial.print(F("On: "));    Serial.print(room.heat.onHour);   Serial.print(F(".")); Serial.println(room.heat.onMins);
    Serial.print(location); Serial.print(F("Off: "));   Serial.print(room.heat.offHour);  Serial.print(F(".")); Serial.println(room.heat.offMins);
  } else if (input == 'R' && state == changingLamp) {
    Serial.print('\n' + location); Serial.print(F("Level: ")); Serial.println(room.lamp.level);
    Serial.print(location); Serial.print(F("On: "));    Serial.print(room.lamp.onHour);   Serial.print(F(".")); Serial.println(room.lamp.onMins);
    Serial.print(location); Serial.print(F("Off: "));   Serial.print(room.lamp.offHour);  Serial.print(F(".")); Serial.println(room.lamp.offMins);
  } else if (input == 'R' && state == changingWater) {
    Serial.print('\n' + location); Serial.print(F("Level: ")); Serial.println(room.water.level);
    Serial.print(location); Serial.print(F("On: "));    Serial.print(room.water.onHour);   Serial.print(F(".")); Serial.println(room.water.onMins);
    Serial.print(location); Serial.print(F("Off: "));   Serial.print(room.water.offHour);  Serial.print(F(".")); Serial.println(room.water.offMins);
  } else if (input == 'R')
    Serial.println(F("Error: Choose a room device to see its data"));

  switch (state) {

    case startup:
      startupState();
      break;

    case firstFloor:
      firstFloorState();
      break;

    case groundFloor:
      groundFloorState();
      break;

    case outside:
      outsideState();
      break;

    case bedroom1:
      bedroom1State();
      break;

    case bedroom2:
      bedroom2State();
      break;

    case bathroom:
      bathroomState();
      break;

    case kitchen:
      kitchenState();
      break;

    case hall:
      hallState();
      break;

    case livingRoom:
      livingRoomState();
      break;

    case garden:
      gardenState();
      break;

    case garage:
      garageState();
      break;

    case changingLight:
      changingLightState();
      break;

    case changingHeat:
      changingHeatState();
      break;

    case changingLamp:
      changingLampState();
      break;

    case changingWater:
      changingWaterState();
      break;
  }

}
