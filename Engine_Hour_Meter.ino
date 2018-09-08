//Engine Hour Meter test code V 1.2
#include <EEPROM.h>
double startTime = 0;  
double stopTime = 0;  
double runTime = 0;  /* Variable for time since current run began */
double totTime = 0; /*Variable for accumulated run time. Initialising only. Will read real, accumulated totTime in from EEPROM later, and use that value as the starting point */
int engineLed = 13; /* LED indication of running state for test purposes. Could be used for triggering relay, MOSFET, warning lamp, etc, in final production  */
int engineRun = 8; /* input pin. To start "Engine Running" pull pin 8 HIGH. Pull to GND to stop. Most magneto ignition have a kill switch that pulls to GND. */
int engineRunAux = 9; /* second auxiliary input pin. Crank sensor signal or Spark sensor signal to ensure actual engine running condition. Proof against false startup conditions, key left on, stall, starter motor cranking etc */
boolean engineLastState = false;  /* Needed to initialise the variable but we don't want to set it HIGH or LOW as we need to read it and not assume. Setting FALSE appeases the compiler and still leaves our options open.*/  
boolean engineRunning = false; /* OK to assume on first power-up, the engine is not running.
/******************************************************************************************************************************** 
 *  ALL OTHER DECLARATIONS GO HERE
 ********************************************************************************************************************************/
// the setup routine runs once when you press reset or connect power.  
  
void setup()  
{                 
  Serial.begin(9600); /* initialize serial port to 9600 baud   */
  pinMode(engineLed, OUTPUT); // initialize digital pin 13 as an output. 
  pinMode(engineRun, INPUT);  // initialise digital pin 8 as an input.
  digitalWrite(engineRun, HIGH); // turn on internal pull up resistor. Stops false "running" state if the input was floating. 
 /* *************************************************************************************************************************
  *  EEPROM INIT AND READ-IN ROUTINE GOES HERE *
  ****************************************************************************************************************************/
  totTime = EEPROM.get(2,totTime) ; /* This is for testing ONLY.  In production, we will read in totTime value from an EEPROM   */ 
  /* First, send some human readable output to the serial port for test purposes*/
  Serial.println("Engine Run on pin 8");  
  Serial.println("Pull to 5v HIGH to run and pull to GND to stop");   
  Serial.println("no debounce on input yet");
  Serial.print("Current total Runtime ");Serial.print(totTime);Serial.println(" seconds");
  /********************************************************************************************************************************************************
  * In future changes, Initial OLED screen messages will be sent at this point as well. Greeting, Branding, animations, any other warm and fuzzy shmooze
  **********************************************************************************************************************************************************/
}  

  
/*********************************************************************************************************************************
 * Main program loop starts here.....  
 ********************************************************************************************************************************/
void loop()  
{  
  engineLastState = digitalRead(engineRun); /*This checks if the kill switch is on or off by reading pin engineRun, which we declared to be pin 8,
  and setting engineLastState to pin 8's current logic level. IE: HIGH or LOW.
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
   }  
}  
