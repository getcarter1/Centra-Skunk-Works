//Engine Hour Meter test code V 1.4
//Added EEPROM persistant Engine Hours 08/09/2018
//Added TFT screen 09/09/2018
//Moved engineRun pin from pin 8 to pin 4 for compatibility with TFT screen.

#include <EEPROM.h>
#include <SPI.h>
#include <SD.h>
#include <TFT.h>  // Arduino LCD library

/* TFT and SD pin definition for the Uno */
/* Using Hardware SPI pins for MOSI (SDA)11, MISO 12 and SCLK (SCL) 13 */
/* SDA and SCL are shared between TFT and SD. Cable Select determines use */
#define sd_cs  8
#define lcd_cs 10
#define dc     9
#define rst    -1 /* Not assigned. Connect TFT rst to hardware reset pin. This enables screen reset with board reset. */

TFT TFTscreen = TFT(lcd_cs, dc, rst); /* create an instance of the TFT library */
PImage logo; /* this variable represents the logo image to be drawn on screen */
char totHours[6]; //Only here for the benefit of the stupid TFT library.
double startTime = 0;  
double stopTime = 0;  
double runTime = 0;  /* Variable for time since current run began */
String uID = "12345678ABCDEF"; /* Initialiser for the Unique Identifier. Likely to be the Chassis number. */
double totTime = 0; /*The main one. Variable for 4 byte (32bit precision, floating point), accumulated run time. Initialising only. Will read real, accumulated totTime in from EEPROM later, and use that value as the starting point */
int engineLed = 13; /* LED indication of running state for test purposes. Could be used for triggering relay, MOSFET, warning lamp, etc, in final production  */
int engineRun = 4; /* input pin. To start "Engine Running" pull pin 4 HIGH. Pull to GND to stop. Most magneto ignition have a kill switch that pulls to GND. */
int engineRunAux = 9; /* second auxiliary input pin. Crank sensor signal or Spark sensor signal to ensure actual engine running condition. Proof against false startup conditions, key left on, stall, starter motor cranking etc */
boolean engineLastState = false;  /* Needed to initialise the variable but we don't want to set it HIGH or LOW as we need to read it and not assume. Setting FALSE appeases the compiler and still leaves our options open.*/  
boolean engineRunning = false; /* OK to assume on first power-up, the engine is not running.
/******************************************************************************************************************************** 
 *  ALL OTHER DECLARATIONS GO HERE
 ********************************************************************************************************************************/
// the setup routine runs once when you press reset or connect power.  
  
void setup()  
{  
   TFTscreen.begin(); /* Initialise TFT */
  TFTscreen.background(0, 0, 0); /* Set background to BLACK */

  Serial.begin(9600); /* initialize serial port to 9600 baud   */
  pinMode(engineLed, OUTPUT); // initialize digital pin 13 as an output. 
  pinMode(engineRun, INPUT);  // initialise digital pin 8 as an input.
  digitalWrite(engineRun, HIGH); // turn on internal pull up resistor. Stops false "running" state if the input was floating. 
 
 /* *************************************************************************************************************************
  *  EEPROM INIT AND READ-IN ROUTINE *
  ****************************************************************************************************************************/
  uID = EEPROM.get(0, uID) ; /* read in 2 byte uID value from EEPROM startinmg at first byte. */
  totTime = EEPROM.get(2,totTime) ; /* read in 2 byte totTime value from EEPROM starting at third byte  */ 
  /**************************************************************************************************************************
   * Will add a routine for checking EEPROM for a unique ID and if not present, handoff to a function to allow writing 
   * of an ID to the EEPROM.  This will be a "write once" mechanism. It should NEVER change value.
   * Something like this....
   * from Serial grab characters and push into String uID
   * then...  
   * for(int i=10; i<x.length; i++){   //start writing at EEPROM byte 10 for a length of number of characters in uID
	*  EEPROM.write(i, x[i]);
  * }
  * Read back the same way.
   **************************************************************************************************************************/ 

  /* First, send some human readable output to the serial port for test purposes*/
  Serial.println("Engine Run on pin 8");  
  Serial.println("Pull to 5v HIGH to run and pull to GND to stop");   
  Serial.println("no debounce on input yet");
  Serial.print("Current total Runtime ");Serial.print(totTime);Serial.println(" seconds");
  /********************************************************************************************************************************************************
  * Initial OLED screen messages will be sent at this point. Greeting, Branding, animations, any other warm and fuzzy shmooze
  **********************************************************************************************************************************************************/
   // try to access the SD card. If that fails (e.g.
  // no card present), the setup process will stop.
  Serial.print(F("Initializing SD card..."));
  if (!SD.begin(sd_cs)) {
    Serial.println(F("failed!"));
    return;
  }
  Serial.println(F("OK!"));
  
  TFTscreen.begin(); // initialize and clear the TFT screen
  logo = TFTscreen.loadImage("centra.bmp");
  TFTscreen.image(logo, 0, 0);
  delay (2000); /* 2 second delay to display splash BMP */

   TFTscreen.background(0, 0, 0);
   TFTscreen.stroke(255, 255, 255); // set the font color to white
   TFTscreen.setTextSize(1);  // set the font size
   TFTscreen.text("Total Engine Hours :\n ", 0, 0); // write the text to the top left corner of the screen
   TFTscreen.setTextSize(3);
   String bigHours = String(totTime);
   bigHours.toCharArray(totHours, 6);
   TFTscreen.stroke(0, 255, 0);
   TFTscreen.text(totHours, 0, 20);
}  

  
/*********************************************************************************************************************************
 * Main program loop starts here.....  
 ********************************************************************************************************************************/
void loop()  {  
    
    
  engineLastState = digitalRead(engineRun); /*This checks if the kill switch is on or off by reading pin engineRun, which we declared to be pin 4,
  and setting engineLastState to pin 4's current logic level. IE: HIGH or LOW.
  If the key is off, the kill switch is grounded and as such, pin 8 will be LOW. If the key is on, the kill switch is lifted from ground and pin 8's internal pullup resistor will pull pin 8 HIGH*/
 
  /* *********************************************************************************************************************************
   *  A second hardware input will be added here. Crank sensor or Spark sensor added for a 2 parameter test of actual running conditions 
   *  to proof against false start conditions. I.E. Key on but engine not actually started or engine cranking but not actually starting 
   *  due to mechanicle failure or lack of fuel, stall condition ETC.
   *  Will read crank sensor or spark and look for multiple pulses in a set number of seconds to establish TRUE condition.
   ***************************************************************************************************************************************/
  
  if (engineRunning == false && engineLastState==HIGH) /***** NB !!!!  MUST use double == in order to do a comparison vs an assignment. 
  Note the difference between engineLastState=HIGH (assignment) and engineLastState==HIGH (comparison). BIG difference in outcome if engineLastState was actually LOW.
  We would wind up resetting our runTime without adding it to totTime in the following code */  

  
  {  
   
   /* turn on the engine / we will only do this once as the engineRunning set to true will prevent entering here again until it is reset to false by Stop. */
      
     digitalWrite(engineLed, HIGH);  
     engineRunning = true;  
     runTime = 0; /* reset to 0 as it is either first start or, we are turning on again and therefore the last runTime has already been added to accumulated totTime   */
     startTime = millis();  
     Serial.println("Engine on, Starting Timer... "); 
     /*******************************************************************************************************************************************************
     * OLED screen will be updated with totTime at this point. This will require a function to be called here.
     * Function will need a timer so that we only update the OLED once every second, and a conversion on totTime to display totTime value on OLED in Hours:Minutes:Seconds
     * We could also display current runTime
      *********************************************************************************************************************************************************/
      TFTscreen.background(0, 0, 0);
      TFTscreen.stroke(0, 255, 0); // set the font color to green
      TFTscreen.setTextSize(1);  // set the font size
      TFTscreen.text("Engine on, Starting Timer... ", 0, 0); // write the text to the top left corner of the screen
   }  
   else if (engineRunning == true && engineLastState ==LOW) /* sense LOW prevents entering here more than once  */
   {  
     /* Turn off the engine but only do the sequence once to prevent messing up the counters. */
     digitalWrite(engineLed, LOW);  
     engineRunning = false;  
     stopTime = millis();  
     runTime +=(stopTime - startTime)/1000; /* last run time in seconds   */
     totTime += runTime; /* add this runTime to accumulated totTime  */
     /*************************************************************************************************************************************************
      * totTime will be written to eeprom at this point to ensure persistant totTime
      *************************************************************************************************************************************************/
     Serial.print("Engine off after ");Serial.print(runTime);Serial.println(" seconds.");  
     Serial.print("Total running time = ");Serial.print(totTime); Serial.print(" seconds. \r\n"); 
     EEPROM.put(2, totTime);
     /**************************************************************************************************************************************************
      * "Finished" OLED screen message can be sent at this point 
      **************************************************************************************************************************************************/
      TFTscreen.background(0, 0, 0);
      TFTscreen.stroke(255, 0, 0); // set the font color to red
      TFTscreen.setTextSize(1);  // set the font size
      TFTscreen.text("Engine off after ", 0, 0); // write the text to the top left corner of the screen
      /****  More maths and variables required for Session runtime to Screen.  This TFT library blows goat !! ****/
      TFTscreen.text("seconds", 0, 20);
      delay(2000);
      TFTscreen.background(0, 0, 0);
      TFTscreen.stroke(255, 255, 255); // set the font color to white
      TFTscreen.setTextSize(1);  // set the font size
      TFTscreen.text("Total Engine Hours :\n ", 0, 0); // write the text to the top left corner of the screen
      TFTscreen.setTextSize(3);
      String bigHours = String(totTime);
      bigHours.toCharArray(totHours, 6);
      TFTscreen.stroke(0, 255, 0);
      TFTscreen.text(totHours, 0, 20);
      TFTscreen.setTextSize(1);
      TFTscreen.stroke(0, 0, 255); //Set font colour BLUE !
      TFTscreen.text("Engine Data uploading...", 0, 60);
      /****************************************************************************************************************************
      * This where we will hand off to the GSM module for transfering of the EEPROM Data
      * 
      * ***************************************************************************************************************************/
   }  
}  