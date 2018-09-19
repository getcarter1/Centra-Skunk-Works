
#include <Arduino.h>
#include <U8g2lib.h>
#include <EEPROM.h>
 #include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>


U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);   // All Boards without Reset of the Display

char totHours[6];
double startTime = 0;  
double stopTime = 0; 
double sessionTime = 0; 
double currentTime = 0;
double showTime = 0;
double runTime = 0;  /* Variable for time since current run began */
float totTime = 0; /*Variable for accumulated run time. Initialising only. Will read real, accumulated totTime in from EEPROM later, and use that value as the starting point */
int engineLed = D4; /* LED indication of running state for test purposes. Could be used for triggering relay, MOSFET, warning lamp, etc, in final production  */
int engineRun = D6; /* input pin. To start "Engine Running" pull pin 4 HIGH. Pull to GND to stop. Most magneto ignition have a kill switch that pulls to GND. */
int engineRunAux = D5; /* second auxiliary input pin. Crank sensor signal or Spark sensor signal to ensure actual engine running condition. Proof against false startup conditions, key left on, stall, starter motor cranking etc */
boolean engineLastState = false;  /* Needed to initialise the variable but we don't want to set it HIGH or LOW as we need to read it and not assume. Setting FALSE appeases the compiler and still leaves our options open.*/  
boolean engineRunning = false; /* OK to assume on first power-up, the engine is not running. */


void setup(void) {
  u8g2.begin();
  u8g2.clearBuffer();          // clear the internal memory
  u8g2.setFont(u8g2_font_profont12_mr); // choose a suitable font
  u8g2.drawStr(10,50,"Reticulating Splines..");
  u8g2.sendBuffer();
  delay(500);
  WiFi.begin("RPI", "raspberry");   //WiFi connection
  
  while (WiFi.status() != WL_CONNECTED) {  //Wait for the WiFI connection completion
  delay(500);
  Serial.println("Waiting for connection");
  u8g2.clearBuffer();          // clear the internal memory
  u8g2.setFont(u8g2_font_profont12_mr); // choose a suitable font
  u8g2.drawStr(10,50,"Waiting for WiFi");
  u8g2.sendBuffer();
  }
   delay(500);
   u8g2.clearBuffer();
  u8g2.drawStr(20,50,"WiFi ready");
  u8g2.sendBuffer();
   delay(500);
  Serial.begin(9600); /* initialize serial port to 9600 baud   */
  pinMode(engineLed, OUTPUT); // initialize digital pin 13 as an output. 
  pinMode(engineRun, INPUT_PULLUP);  // initialise digital pin 8 as an input.
  //digitalWrite(engineRun, HIGH); // turn on internal pull up resistor. Stops false "running" state if the input was floating. 
 
 /* *************************************************************************************************************************
  *  EEPROM INIT AND READ-IN ROUTINE *
  ****************************************************************************************************************************/
  
  EEPROM.begin(32);
  totTime = EEPROM.get(0,totTime) ; /* read in totTime value from EEPROM   */ 
  
  /* First, send some human readable output to the serial port for test purposes*/
  Serial.println("Engine Run on pin 8");  
  Serial.println("Pull to 5v HIGH to run and pull to GND to stop");   
  Serial.println("no debounce on input yet");
  Serial.print("Current total Runtime ");Serial.print(totTime);Serial.println(" seconds");
  String bigHours = String(totTime);
   bigHours.toCharArray(totHours, 16);
   u8g2.clearBuffer();          // clear the internal memory
  u8g2.setFont(u8g2_font_profont12_mf); // choose a suitable font
  u8g2.drawStr(8,10,"TOTAL ENGINE TIME");  // write something to the internal memory
  u8g2.setFont(u8g2_font_profont22_mn); 
     unsigned long allSeconds=totTime;
     int runHours= allSeconds/3600;
     int secsRemaining=allSeconds%3600;
     int runMinutes=secsRemaining/60;
     int runSeconds=secsRemaining%60;
     char buf[21];
     sprintf(buf,"%02d:%02d:%02d",runHours,runMinutes,runSeconds);      
  u8g2.drawStr(10,50,buf);  // write something to the internal memory
  //u8g2.setFont(u8g2_font_ncenB08_tr);
  //u8g2.drawStr(70,40,"sec");
  u8g2.sendBuffer();          // transfer internal memory to the display
  delay(2000);
}

void loop(void) {
  
  engineLastState = digitalRead(engineRun); /*This checks if the kill switch is on or off by reading pin engineRun, which we declared to be pin 4,
  and setting engineLastState to pin 4's current logic level. IE: HIGH or LOW.
  If the key is off, the kill switch is grounded and as such, pin 8 will be LOW. If the key is on, the kill switch is lifted from ground and pin 8's internal pullup resistor will pull pin 8 HIGH*/
 
  /* *********************************************************************************************************************************
   *  A second hardware input will be added here. Crank sensor or Spark sensor added for a 2 parameter test of actual running conditions 
   *  to proof against false start conditions. I.E. Key on but engine not actually started or engine cranking but not actually starting 
   *  due to mechanicle failure or lack of fuel, stall condition ETC.
   *  Will read crank sensor or spark and look for multiple pulses in a set number of seconds to establish TRUE condition.
  
   ***************************************************************************************************************************************/
  if (engineRunning == true && engineLastState==HIGH)
  {
     currentTime = millis();  
     showTime =(currentTime - sessionTime)/1000;
     String smallHours = String(showTime);
     smallHours.toCharArray(totHours, 16);
     u8g2.setFontMode(1);
     u8g2.setFont(u8g2_font_profont12_mf);
     u8g2.drawStr(0,10," Engine on "); 
     u8g2.setFont(u8g2_font_profont22_mn);
     u8g2.drawStr(0,50,totHours);
     u8g2.setFont(u8g2_font_profont12_mf);
     u8g2.drawStr(90,50,"sec");
     u8g2.sendBuffer();          // transfer internal memory to the display
     u8g2.setFontMode(0);
     u8g2.setFont(u8g2_font_profont22_mn);
     u8g2.sendBuffer();
     u8g2.clearBuffer();          // clear the internal memory
}
  else if (engineRunning == false && engineLastState==HIGH) /***** NB !!!!  MUST use double == in order to do a comparison vs an assignment. 
  Note the difference between engineLastState=HIGH (assignment) and engineLastState==HIGH (comparison). BIG difference in outcome if engineLastState was actually LOW.
  We would wind up resetting our runTime without adding it to totTime in the following code */  

  
  {  
   
   /* turn on the engine / we will only do this once as the engineRunning set to true will prevent entering here again until it is reset to false by Stop. */
      
     digitalWrite(engineLed, LOW);
     engineRunning = true;  
     currentTime = 0;
     runTime = 0; /* reset to 0 as it is either first start or, we are turning on again and therefore the last runTime has already been added to accumulated totTime   */
     sessionTime = millis();
     startTime = millis(); 
      
     Serial.println("Engine on, Starting Timer... "); 
     u8g2.clearBuffer();          // clear the internal memory
     u8g2.setFont(u8g2_font_profont12_mf); // choose a suitable font
     u8g2.drawStr(0,10," Engine on ");  // write something to the internal memory
     u8g2.drawStr(0,20," Starting Timer... ");
     u8g2.sendBuffer();          // transfer internal memory to the display
     delay(1000);
   }  
   else if (engineRunning == true && engineLastState ==LOW) /* sense LOW prevents entering here more than once  */
   {  
     /* Turn off the engine but only do the sequence once to prevent messing up the counters. */
     digitalWrite(engineLed, HIGH);
     engineRunning = false;  
     stopTime = millis();  
     runTime +=(stopTime - startTime)/1000; /* last run time in seconds   */
     totTime += runTime; /* add this runTime to accumulated totTime  */
     /*************************************************************************************************************************************************
      * totTime will be written to eeprom at this point to ensure persistant totTime
      *************************************************************************************************************************************************/
     Serial.print("Engine off after ");Serial.print(runTime);Serial.println(" seconds.");  
     Serial.print("Total running time = ");Serial.print(totTime); Serial.print(" seconds. \r\n"); 
     EEPROM.put(0, totTime);
     EEPROM.commit();
     u8g2.clearBuffer();          // clear the internal memory
     u8g2.setFont(u8g2_font_profont12_mf); // choose a suitable font
     u8g2.drawStr(0,10," Engine off after");  // write something to the internal memory
     String smallHours = String(runTime);
     smallHours.toCharArray(totHours, 6);
     u8g2.setFont(u8g2_font_profont22_mn);
     u8g2.drawStr(0,50,totHours);
     u8g2.setFont(u8g2_font_profont12_mf);
     u8g2.drawStr(90,50,"sec");
     u8g2.sendBuffer();          // transfer internal memory to the display
     delay(1000);
     
     u8g2.drawStr(0,60,"Sending data");
     u8g2.sendBuffer();
     /**************************************************************************************************************
      * HTTP JSON DATA UPLOAD SECTION
      * ***********************************************************************************************************/
     HTTPClient http;    //Declare object of class HTTPClient
     http.begin("http://10.1.1.26:8080/api/v1/cZkLsij6KXANn2sKKjRN/telemetry");      //Specify request destination
     http.addHeader("Content-Type", "application/json");  //Specify content-type header
      u8g2.drawStr(0,60,"Sending data.");
     u8g2.sendBuffer();                  
         // Prepare a JSON payload string
  String payload = "{";
  payload += "\"Total Hours\":"; payload += totTime; 
  payload += "}";
      u8g2.drawStr(0,60,"Sending data..");
     u8g2.sendBuffer();
  // Send payload
  char attributes[100];
  payload.toCharArray( attributes, 100 );
  u8g2.drawStr(0,60,"Sending data....");
     u8g2.sendBuffer();
  int httpCode = http.POST( attributes );
  Serial.println( attributes );
     Serial.println(httpCode);   //Print HTTP return code
     Serial.println(payload);    //Print request response payload
     
      u8g2.drawStr(0,60,"Sending data......");
     u8g2.sendBuffer();

     http.end();  //Close connection
     u8g2.setFontMode(0);
     u8g2.setFont(u8g2_font_cu12_hr);
     u8g2.sendBuffer();
     //u8g2.clearBuffer();
     u8g2.setFont(u8g2_font_profont12_mf);
     u8g2.drawStr(0,60,"                    ");          
     u8g2.drawStr(0,60,"Data sent");
     u8g2.sendBuffer();
     delay(2000);
     
     //String bigHours = String(totTime);
     //bigHours.toCharArray(totHours, 16);
     unsigned long allSeconds=totTime;
     int runHours= allSeconds/3600;
     int secsRemaining=allSeconds%3600;
     int runMinutes=secsRemaining/60;
     int runSeconds=secsRemaining%60;
     char buf[21];
     sprintf(buf,"%02d:%02d:%02d",runHours,runMinutes,runSeconds);
     u8g2.clearBuffer();         
     u8g2.setFont(u8g2_font_profont12_mf);
     u8g2.drawStr(8,10,"TOTAL ENGINE TIME");  
     u8g2.setFont(u8g2_font_profont22_mn);       
     u8g2.drawStr(10,50,buf);  
     //u8g2.setFont(u8g2_font_profont22_mn);
     //u8g2.drawStr(70,40,"sec");
     u8g2.sendBuffer();         
}
}
