/*
 * HID RFID Reader Wiegand Interface for Arduino Uno
 * Written by Daniel Smith, 2012.01.30
 * www.pagemac.com
 *
 * This program will decode the wiegand data from a HID RFID Reader (or, theoretically,
 * any other device that outputs weigand data).
 * The Wiegand interface has two data lines, DATA0 and DATA1.  These lines are normall held
 * high at 5V.  When a 0 is sent, DATA0 drops to 0V for a few us.  When a 1 is sent, DATA1 drops
 * to 0V for a few us.  There is usually a few ms between the pulses.
 *
 * Your reader should have at least 4 connections (some readers have more).  Connect the Red wire 
 * to 5V.  Connect the black to ground.  Connect the green wire (DATA0) to Digital Pin 2 (INT0).  
 * Connect the white wire (DATA1) to Digital Pin 3 (INT1).  That's it!
 *
 * Operation is simple - each of the data lines are connected to hardware interrupt lines.  When
 * one drops low, an interrupt routine is called and some bits are flipped.  After some time of
 * of not receiving any bits, the Arduino will decode the data.  I've only added the 26 bit and
 * 35 bit formats, but you can easily add more.
 
*/
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
 
#define MAX_BITS 100                 // max number of bits 
#define WEIGAND_WAIT_TIME  3000      // time to wait for another weigand pulse.  
 
unsigned char databits[MAX_BITS];    // stores all of the data bits
unsigned char bitCount;              // number of bits currently captured
unsigned char flagDone;              // goes low when data is currently being captured
unsigned int weigand_counter;        // countdown until we assume there are no more bits
 
unsigned long facilityCode=0;        // decoded facility code
unsigned long cardCode=0;            // decoded card code

LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

#define PIN_DATA0 2
#define PIN_DATA1 3
#define PIN_RED_LED 4
#define PIN_GRN_LED 5
#define PIN_BEEPER 6
 
// interrupt that happens when INTO goes low (0 bit)
void ISR_INT0()
{
  //Serial.print("0");   // uncomment this line to display raw binary
  bitCount++;
  flagDone = 0;
  weigand_counter = WEIGAND_WAIT_TIME;  
 
}
 
// interrupt that happens when INT1 goes low (1 bit)
void ISR_INT1()
{
  //Serial.print("1");   // uncomment this line to display raw binary
  databits[bitCount] = 1;
  bitCount++;
  flagDone = 0;
  weigand_counter = WEIGAND_WAIT_TIME;  
}

void setup()
{
  pinMode(13, OUTPUT);  // LED
  pinMode(PIN_DATA0, INPUT);     // DATA0 (INT0)
  pinMode(PIN_DATA1, INPUT);     // DATA1 (INT1)
  pinMode(PIN_RED_LED, OUTPUT);     // RED LED
  pinMode(PIN_GRN_LED, OUTPUT);     // GREEN LED
  pinMode(PIN_BEEPER, OUTPUT);     // BEEPER

  digitalWrite(PIN_RED_LED, HIGH);       // turn on pullup resistors  
  digitalWrite(PIN_GRN_LED, HIGH);       // turn on pullup resistors  
  digitalWrite(PIN_BEEPER, HIGH);       // turn on pullup resistors  
 
  Serial.begin(9600);
  Serial.println("RFID Readers");
 
  // binds the ISR functions to the falling edge of INTO and INT1
  attachInterrupt(0, ISR_INT0, FALLING);  
  attachInterrupt(1, ISR_INT1, FALLING);
 
  weigand_counter = WEIGAND_WAIT_TIME;

  lcd.init();                      // initialize the lcd 
  lcd.backlight();
  lcd.home();
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Please tap");
  lcd.setCursor(3, 1);
  lcd.print("RFID card...");
}
 
void loop()
{
  // This waits to make sure that there have been no more data pulses before processing data
  if (!flagDone) {
    if (--weigand_counter == 0)
      flagDone = 1;  
  }
 
  // if we have bits and we the weigand counter went out
  if (bitCount > 0 && flagDone) {
    unsigned char i;

    digitalWrite(13, HIGH);       // turn on pullup resistors  

    Serial.print("Read ");
    Serial.print(bitCount);
    Serial.print(" bits. ");

    // we will decode the bits differently depending on how many bits we have
    // see www.pagemac.com/azure/data_formats.php for mor info
    if (bitCount == 35)
    {

      digitalWrite(PIN_GRN_LED, LOW);       // turn on pullup resistors  
      
      // 35 bit HID Corporate 1000 format
      // facility code = bits 2 to 14
      for (i=2; i<14; i++)
      {
         facilityCode <<=1;
         facilityCode |= databits[i];
      }
 
      // card code = bits 15 to 34
      for (i=14; i<34; i++)
      {
         cardCode <<=1;
         cardCode |= databits[i];
      }
 
      printBits();

    }
    else if (bitCount == 26)
    {

      digitalWrite(PIN_GRN_LED, LOW);       // turn on pullup resistors  
      
      // standard 26 bit format
      // facility code = bits 2 to 9
      for (i=1; i<9; i++)
      {
         facilityCode <<=1;
         facilityCode |= databits[i];
      }
 
      // card code = bits 10 to 23
      for (i=9; i<25; i++)
      {
         cardCode <<=1;
         cardCode |= databits[i];
      }
 
      printBits();  
    }
    else {

      digitalWrite(PIN_RED_LED, LOW);       // turn on pullup resistors  
      
      // you can add other formats if you want!
     Serial.println("Unable to decode."); 
    }

    digitalWrite(13, LOW);       // turn on pullup resistors  
 
    delay(2000); // keep display in 2s

     // cleanup and get ready for the next card
     bitCount = 0;
     facilityCode = 0;
     cardCode = 0;
     for (i=0; i<MAX_BITS; i++) 
     {
       databits[i] = 0;
     }

    digitalWrite(PIN_GRN_LED, HIGH);       // turn on pullup resistors  
    digitalWrite(PIN_RED_LED, HIGH);       // turn on pullup resistors  

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Please tap");
    lcd.setCursor(3, 1);
    lcd.print("RFID card...");
     
  }
}
 
void printBits()
{
      // I really hope you can figure out what this function does
      Serial.print("FacilityCode = ");
      Serial.print(facilityCode);
      Serial.print(", CardCode = ");
      Serial.println(cardCode); 

      // LCD
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Facility: "); lcd.print(facilityCode, DEC);
      lcd.setCursor(0, 1);
      lcd.print("Card:     "); lcd.print(cardCode, DEC);
}
