// Moonlite-compatible stepper controller
//
// Original code by orly.andico@gmail.com, 13 April 2014
// Modified by Anat Ruangrassamee (aruangra@yahoo.com), 26 September 2017

#include <EEPROM.h>

#include <LiquidCrystal.h>

#include "OneWire.h"
#include "DallasTemperature.h"

// Data wire is plugged into pin A1 on the Arduino
#define ONE_WIRE_BUS A1
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

#include "ArdumotoStepper.h"

const int stepsPerRevolution = 5370;  // change this to fit the number of steps per revolution for your motor

// initialize the stepper library:
ArdumotoStepper myStepper(stepsPerRevolution);

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);           // select the pins used on the LCD panel

// define some values used by the panel and buttons
int lcd_key     = 0;
int adc_key_in  = 0;
int flg_btnspeed = 0;
int flg_lcdlight = 0;

#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

int read_LCD_buttons(){               // read the buttons
  
    adc_key_in = analogRead(0);       // read the value from the sensor 

    // my buttons when read are centered at these valies: 0, 144, 329, 504, 741
    // we add approx 50 to those values and check to see if we are close
    // We make this the 1st option for speed reasons since it will be the most likely result

    // MODIFICATION (START)
    delay(6); //switch debounce delay. Increase this delay if incorrect switch selections are returned.
    if (adc_key_in != analogRead(0)) return btnNONE;  // double checks the keypress. If the two readings are not equal after debounce delay, it tries again.
    // MODIFICATION (END)

    if (adc_key_in > 1000) return btnNONE; 

    // For V1.1 us this threshold
    if (adc_key_in < 50)   return btnRIGHT;  
    if (adc_key_in < 250)  return btnUP; 
    if (adc_key_in < 450)  return btnDOWN; 
    if (adc_key_in < 650)  return btnLEFT; 
    if (adc_key_in < 850)  return btnSELECT;  

   // For V1.0 comment the other threshold and use the one below:
   /*
     if (adc_key_in < 50)   return btnRIGHT;  
     if (adc_key_in < 195)  return btnUP; 
     if (adc_key_in < 380)  return btnDOWN; 
     if (adc_key_in < 555)  return btnLEFT; 
     if (adc_key_in < 790)  return btnSELECT;   
   */

    return btnNONE;                // when all others fail, return this.
}


#define MAXCOMMAND 8

char inChar;
char cmd[MAXCOMMAND];
char param[MAXCOMMAND];
char line[MAXCOMMAND];
int isRunning = 0;
int speed = 32;
int eoc = 0;
int idx = 0;
long pos=0;
long distanceToGo = 0;
long currentPosition = 1000;
float tempC = 0;
long timer_millis = 0;

//This function will write a 4 byte (32bit) long to the eeprom at
//the specified address to address + 3.
void EEPROMWritelong(int address, long value)
      {
      //Decomposition from a long to 4 bytes by using bitshift.
      //One = Most significant -> Four = Least significant byte
      byte four = (value & 0xFF);
      byte three = ((value >> 8) & 0xFF);
      byte two = ((value >> 16) & 0xFF);
      byte one = ((value >> 24) & 0xFF);

      //Write the 4 bytes into the eeprom memory.
      EEPROM.write(address, four);
      EEPROM.write(address + 1, three);
      EEPROM.write(address + 2, two);
      EEPROM.write(address + 3, one);
      }

long EEPROMReadlong(long address)
      {
      //Read the 4 bytes from the eeprom memory.
      long four = EEPROM.read(address);
      long three = EEPROM.read(address + 1);
      long two = EEPROM.read(address + 2);
      long one = EEPROM.read(address + 3);

      //Return the recomposed long by using bitshift.
      return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
      }


void setup()
{  
  Serial.begin(9600);
  //Serial1.begin(9600);

  sensors.begin();
  
  if (flg_lcdlight == 1) {
     analogWrite(10, 25);  
  } else {
     analogWrite(10, 0);                     
  }
  
  memset(line, 0, MAXCOMMAND);
  currentPosition=EEPROMReadlong(0);
  
   lcd.begin(16, 2);               // start the library
   lcd.setCursor(0,0);             // set the LCD cursor   position 
   lcd.print("PnP Focus  T:   ");  // print a simple message on the LCD
   lcd.setCursor(0,1);             // set the LCD cursor   position 
   lcd.print("Position:");  // print a simple message on the LCD

   delay(1000);
   sensors.requestTemperatures();
   tempC = sensors.getTempCByIndex(0);
   lcd.setCursor(13,0);
   if ((tempC < -50) || (tempC > 50)){
     lcd.print("NA  ");
   } else {
     lcd.print(String(round(tempC)) + "C");
   }

   timer_millis=millis();
}



void loop(){


      if ((millis() - timer_millis) > 60000) {
        timer_millis=millis();
        sensors.requestTemperatures();
        tempC = sensors.getTempCByIndex(0);
        lcd.setCursor(13,0);
        if ((tempC < -50) || (tempC > 50)){
          lcd.print("NA  ");
        } else {
          lcd.print(String(round(tempC)) + "C");
        } 
      }
     
    
    lcd_key = read_LCD_buttons();   // read the buttons
    
   switch (lcd_key){             
    
       case btnRIGHT:{
         if (isRunning == 0) {             
            currentPosition = currentPosition + 1;
            if (currentPosition > 65535)
                currentPosition = 0;
            myStepper.step(1);
             if (flg_btnspeed == 1) {
                 delay(2);
             } else {
                 delay(160);                          
             }
         }
         break;
       }
       case btnLEFT:{
         if (isRunning == 0) { 
            currentPosition = currentPosition - 1;
            if (currentPosition < 0)
                currentPosition = 65535;
            myStepper.step(-1);
             if (flg_btnspeed == 1) {
                 delay(2);
             } else {
                 delay(160);                          
             }
         }
         break;
       }    
       case btnUP:{
             flg_lcdlight = 1 - flg_lcdlight;
             if (flg_lcdlight == 1) {
                 analogWrite(10, 25);  
             } else {
                 analogWrite(10, 0);                     
             }
             delay(600);       
             break;
       }
       case btnDOWN:{
         if (isRunning == 0) { 
             flg_btnspeed=1-flg_btnspeed; 
             if (flg_btnspeed == 1) {
                 lcd.setCursor(0,0);             // set the LCD cursor   position 
                 lcd.print("FAST MOVEMENT     ");  // print a simple message on the LCD  
             } else {
                 lcd.setCursor(0,0);             // set the LCD cursor   position 
                 lcd.print("SLOW MOVEMENT     ");  // print a simple message on the LCD                           
             }
             delay(1000);
             lcd.setCursor(0,0);             // set the LCD cursor   position 
             lcd.print("PnP Focus  T:   ");  // print a simple message on the LCD
             lcd.setCursor(13,0);
             if ((tempC < -50) || (tempC > 50)){
               lcd.print("NA  ");
             } else {
               lcd.print(String(round(tempC)) + "C");
             } 
         }
         break;
       }
       case btnSELECT:{
             isRunning = 0; // STOP
             // EEPROM HERE
             EEPROMWritelong(0, currentPosition);      
             lcd.setCursor(0,0);             // set the LCD cursor   position 
             lcd.print("STOPPED & SAVED ");  // print a simple message on the LCD             
             delay(1000);
             lcd.setCursor(0,0);             // set the LCD cursor   position 
             lcd.print("PnP Focus  T:   ");  // print a simple message on the LCD
             lcd.setCursor(13,0);
             if ((tempC < -50) || (tempC > 50)){
               lcd.print("NA  ");
             } else {
               lcd.print(String(round(tempC)) + "C");
             }   
             break;
       }
       case btnNONE:{
             
             break;
       }
   }

    lcd.setCursor(10,1);          
    lcd.print(String(currentPosition)+ "     ");

    if (isRunning) {
      if (distanceToGo > 0) {
        currentPosition = currentPosition + 1;
        if (currentPosition > 65535)
            currentPosition = 0;        
        distanceToGo = distanceToGo - 1;
        myStepper.step(1);
        delay(2);
      } else {
        if (distanceToGo < 0) {
          currentPosition = currentPosition - 1;
          if (currentPosition < 0)
              currentPosition = 65535;
          distanceToGo = distanceToGo + 1;
          myStepper.step(-1);
          delay(2);
        } else {
           isRunning = 0;
        }
      }
    }
     

    // read the command until the terminating # character
    while (Serial.available() && !eoc) {
      inChar = Serial.read();
      //Serial1.write(inChar);
      
      if (inChar != '#' && inChar != ':') {
        line[idx++] = inChar;
        if (idx >= MAXCOMMAND) {
          idx = MAXCOMMAND - 1;
        }
      } 
      else {
        if (inChar == '#') {
          eoc = 1;
        }
      }
    }


  // process the command we got
  if (eoc) {
    memset(cmd, 0, MAXCOMMAND);
    memset(param, 0, MAXCOMMAND);

    int len = strlen(line);
    if (len >= 2) {
      strncpy(cmd, line, 2);
    }

    if (len > 2) {
      strncpy(param, line + 2, len - 2);
    }

    memset(line, 0, MAXCOMMAND);
    eoc = 0;
    idx = 0;



    // motor is moving - 01 if moving, 00 otherwise
    if (!strcasecmp(cmd, "GI")) {
      if (isRunning) {
        Serial.print("01#");
        //Serial1.write("01#");
      } 
      else {
        Serial.print("00#");
        //Serial1.write("00#");

        // EEPROM
        EEPROMWritelong(0, currentPosition);
      }
    }

    // initiate a move
    if (!strcasecmp(cmd, "FG")) {
      isRunning = 1;
    }

    // stop a move
    if (!strcasecmp(cmd, "FQ")) {
      isRunning = 0;
    } 


    // get the current motor position
    if (!strcasecmp(cmd, "GP")) {
      pos = currentPosition;
      char tempString[6];
      sprintf(tempString, "%04X", pos);
      Serial.print(tempString);
      Serial.print("#");
    }

    // set new motor position
    if (!strcasecmp(cmd, "SN")) {
      pos = hexstr2long(param);
      distanceToGo = pos - currentPosition;
    }



    // set current motor position
    if (!strcasecmp(cmd, "SP")) {
      pos = hexstr2long(param);
      currentPosition = pos;

      // EEPROM
      EEPROMWritelong(0, currentPosition);
      
    }

    // set speed, only acceptable values are 02, 04, 08, 10, 20
    if (!strcasecmp(cmd, "SD")) {
      speed = hexstr2long(param);
      // the Moonlite speed setting is ignored.
    }

    // get the current temperature
    if (!strcasecmp(cmd, "GT")) {
           if ((tempC < -50) || (tempC > 50)){
                Serial.print("C6#");
           } else {
                char tempString[6];
                int tpval = (tempC * 2);
                sprintf(tempString, "%04X", (int) tpval);
                Serial.print(tempString);;
                Serial.print("#");
           }
    }

    // get the temperature coefficient.
    if (!strcasecmp(cmd, "GC")) {
      Serial.print("02#");
    }

    // get the new motor position (target)
    if (!strcasecmp(cmd, "GN")) {
      pos = currentPosition + distanceToGo;
      char tempString[6];
      sprintf(tempString, "%04X", pos);
      Serial.print(tempString);
      Serial.print("#");
    }
    
    // get the current motor speed, only values of 02, 04, 08, 10, 20
    if (!strcasecmp(cmd, "GD")) {
      char tempString[6];
      sprintf(tempString, "%02X", speed);
      Serial.print(tempString);
      Serial.print("#");
    }
    
    // whether half-step is enabled or not, always return "00"
    if (!strcasecmp(cmd, "GH")) {
      Serial.print("00#");
    }
 
    // LED backlight value, always return "00"
    if (!strcasecmp(cmd, "GB")) {
      Serial.print("00#");
    }

    // home the motor
    if (!strcasecmp(cmd, "PH")) { 

    }

    // firmware value, always return "10"
    if (!strcasecmp(cmd, "GV")) {
      Serial.print("10#");
      //Serial1.write("10#");
    }
  }
  
} // end loop

long hexstr2long(char *line) {
  long ret = 0;

  ret = strtol(line, NULL, 16);
  return (ret);
}




