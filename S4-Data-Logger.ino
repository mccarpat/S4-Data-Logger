#define VERSION "0.0.6"
#define VERSIONDATE "07-29-16"

/*   CURRENT ISSUE
   
     wdt doesn't disable on a rest. it actually sets to 15ms. so it gets stuck in a loop.
     either find a different software reset solution, or fix this.
     http://electronics.stackexchange.com/questions/151865/why-does-my-avr-reset-when-i-call-wdt-disable-to-try-to-turn-the-watchdog-time


*/




/* Places program can get stuck:
   - Initializing the SD card if failure -> RRRRGGxxxxx...
     InitializeSDCard(); // Fail loop: Red 400, green 200, Off 500.    Success loop:  Green 250 x 3
   - Opening the datafile if failure or if no SD card present -> RRxRRGGxxx...
     WriteDataToSDCard();   // Fail: Red 200, off 100, Red 200, Green 200, off 300   Sucess: Green 250
   - Waiting for SD card to be ejected after holding the button. Solid green light. -> G...
   - Waiting for SD card to be reinserted after ejected. Blinking green light. On/Off 500ms. ->  GGGGGxxxxxGGGGGxxxxx...
*/

// https://gist.github.com/jenschr/5713c927c3fb8663d662

/* STATUS:
    Ok so all it does right now is write the four analogreads (voltage of 2.56 is a word  = 256)
    to the serial. Next, you should
    - Implement data tolerance and compression (e.g. the whole thing where if the new value is within tolerance of the old one, just say it is the old value, and if it is the old value, say it is "x". Then look at the line, and replace "x,"'s with a, b, c... e.g.   "x,x,x," => "c" (a=1, b=2, c=3).
    - Write to the SD card
    - Have system sleep between readings, verify that it still functions
    - Implement SD card stopping, ejecting, formatting, error handling
    - Create MAX485 circuit
    - Connect (S4) the resistors and connect them to the analog inputs. Test/Verify.
    - Connect MAX485 to CC, verify that it can read data (use Sofia's code with pin # changes)
    - Code data retrieval from CC, including error handling
    - Write all of this data to the SD card
    - Verify that sleeping between cycles does not interfere with anything
    - Add a watchdog timer? Maybe have it write to the card "watchdog" if "watchdog" not found in last 100 lines or whatever.
    - Make it so that if anything errors and requires a reset (e.g. sd card won't initialize) red LED goes on
    - Write short manual for what to do for when red LED is on, and then for how to swap cards, etc.
    - REDO ALL OF THE ABOVE:  Write wall logger.
*/

/*
 *  S4-Logger
 *  
 *  --- Information -----------------
 *  EGR 699 Project Code
 *  Logs S4 data
 *  Patrick McCarthy (PJM)
 *  Sofia Fanourakis
 *  
 *  --- Changelog -------------------
 *  v0.0.6 - PJM - (working on adding a watchdog timer reset so it can continue to write after an eject, then adding formatting)
 *  v0.0.5 - PJM -> 07-29-16 - SD Card writing (dummy data) and safe eject
 *  v0.0.4 - PJM - (working on data compression and delta tolerance) (not done, skipping)
 *  v0.0.3 - PJM - Setting up usage of data[...]
 *  v0.0.2 - PJM -> 07-28-16 - Testing Analog Reads and writing to serial
 *  v0.0.1 - PJM -> 07-25-16 - Rewriting some original code from scratch
 * 
 *  --- Pins ------------------------
 *  // //PC4/A4 (SDA)  -> RTC SDA
 *  // //PC5/A5 (SCL)  -> RTC SCL
 *
 *
 *  D13 / PB5 / 13 / SCK        -        SD CLK
 *  D12 / PB4 / 12 / MISO       -        SD DO
 *  D11 / PB3 / 11 / MOSI       -        SD DI
 *  D10 / PB2 / 10 / SS         -        SD CS      // Can be changed to arbitrary GPIO pin
 *  D4  / PD4 /  4              -        SD CD      // Can be changed to arbitrary GPIO pin
 *  D9  / PB1 /  9 / PCINT1     -        Button 1   // Can be changed to arbitrary GPIO pin
 *  D8  / PB0 /  8 / PCINT0     -        Button 2   // Can be changed to arbitrary GPIO pin
 *  D7  / PD7 /  7              -        Green LED  // Can be changed to arbitrary GPIO pin
 *  D6  / PD6 /  6              -        Red LED    // Can be changed to arbitrary GPIO pin
 *  A0  / PC0 / 14 / Analog 0   -        (Temp) Rocker 1
 *  A1  / PC1 / 15 / Analog 1   -        (Temp) Rocker 2
 *  A2  / PC2 / 16 / Analog 2   -        (Temp) Rocker 3
 *  A3  / PC3 / 17 / Analog 3   -        (Temp) Rocker 4
 *  Use D2 - has INT0, use for brownout interrupt
 *  
 *  --- Notes -----------------------
 *  
 *  //- Need to find two pins that can be pulled up/down to identify unique logging units
 *  //- Supercap calc:  http://electronics.stackexchange.com/questions/4951/how-do-i-calculate-how-fast-a-capacitor-will-discharge
 *  //- For wall logger, each loop write data to a variable. When power dies, write variables to EEPROM, not SD card.  Then move that to SD card when you ahve power.
 *  
 *  // 07-29-16 Note: When writing to card, currently no differentiation between an error opening dataFile or if a card is not present. Same error result.
 */ 
 
 // Define system settings
 #define USING_SERIAL
 #define DEBOUNCE_TIME 10 // 10 ms, one debounce
 #define DATA_LENGTH 20 // Data array has 20 elements
 #define LOG_FILE_NAME "datalog.txt"
 #define SAFE_SDCARD_EJECT_BUTTON_HOLD_SECONDS 3 // 3 seconds of holding button1 to safely eject
 // #define DUPLICATE 'x' // Character used to represent duplicate data from a previous write cycle
 //#define tolerance_Voltage0 5 // If the new reading is within X of the previously recorded reading, do not write new data
 //#define tolerance_Voltage1 5 
 //#define tolerance_Voltage2 5 
 //#define tolerance_Voltage3 5 

 // Define pins
 #define PIN_BUTTON1 9
 #define PIN_BUTTON2 8
 #define PIN_RED_LED 6
 #define PIN_GREEN_LED 7
 #define PIN_SD_CS 10
 #define PIN_SD_CD 4
 
 // Define "functions"
 #define RED_LED_ON digitalWrite(PIN_RED_LED, 1)
 #define RED_LED_OFF digitalWrite(PIN_RED_LED, 0)
 #define GREEN_LED_ON digitalWrite(PIN_GREEN_LED, 1)
 #define GREEN_LED_OFF digitalWrite(PIN_GREEN_LED, 0)
 #define SD_CARD_IS_PRESENT digitalRead(PIN_SD_CD)
 //#define watchdog_clear_status()    MCUSR = 0
 //#define watchdog_feed()            wdt_reset()

 
 // Define array element positions for data
 #define pos_Voltage0 0
 #define pos_Voltage1 1
 #define pos_Voltage2 2
 #define pos_Voltage3 3
 
 // Includes
 #include <SD.h>
 #include <avr/interrupt.h>
 #include <avr/wdt.h>
 
 // Declare functions
 boolean Button1_Pressed( void );
 void MoveDataToOldData( void );
 void ReadAnalogVoltages( void );  // Update AnalogRead voltages and data[...]
 void UpdateWriteData( void );  // Updates writedata based upon data && olddata, and updates holddata
 void InitializeSDCard( void );  // Initializes the SD card interface, and handles error loop (light codes) if necessary
 void WriteDataToSDCard( void ); // Writes data[...] to the SD card, and handles error loop (light codes) if necessary
 void HandleSDCardSwap( void ); // Handles waiting for the SD card to be ejected and another one inserted.
 void EjectRequestCheckAndHandler( void ); // Checks if the users wants to eject the SD card and handles this process
 
 // Initialize variables
 long count = 0;
 word data[DATA_LENGTH] = {0};          // Updated data (e.g. analog inputs or charge controller received data) placed here
 word olddata[DATA_LENGTH] = {0};       // Previous loops data is stored here for comparison purposes
 //word writedata[DATA_LENGTH] = {0};     // This is the data to be written to the SD card (e.g. olddata="5,2,1" && data="4,3,1" -> writedata="4,3,x")
 //word holddata[DATA_LENGTH] = {0};      // This is the data that any "old" writedata represents (cont. above e.g.: writedata="4,3,1")
 word A0_value, A1_value, A2_value, A3_value;
 byte error_status = 0;
 byte GoodToEject = 0;
 //boolean Button1_Pressed = false;
 //boolean Button2_Pressed = false;
 
 // Main program
 void setup() {
   
   // Immediately disable the watchdog timer so as not to get stuck in a loop
   MCUSR = 0;
   wdt_disable();
   //MCUSR &= ~(1<<WDRF);
   //WDTCSR |= (1<<WDCE) | (1<<WDE);
   //WDTCSR = 0x00;
   //wdt_disable();
   
   // Initialize system
   pinMode(PIN_BUTTON1, INPUT);
   pinMode(PIN_BUTTON2, INPUT);
   pinMode(PIN_RED_LED, OUTPUT);
   pinMode(PIN_GREEN_LED, OUTPUT);
   pinMode(PIN_SD_CS, OUTPUT);
   pinMode(PIN_SD_CD, INPUT);
  
  
   //watchdog_clear_status();
   

   
   
   // Initialize serial communication at 9600 bits per second:
   #ifdef USING_SERIAL
   Serial.begin(9600);
   #endif
   
   InitializeSDCard(); // Fail loop: Red 400, green 200, Off 500.    Success loop:  Green 250 x 3
  
   // Primary code loop
   while(1)
   {
     
     MoveDataToOldData();  // Updates olddata to be the previous cycle's data
     
     //digitalWrite(PIN_GREEN_LED, Button1_Pressed());  //sets the LED to current state of button each loop
     //digitalWrite(PIN_RED_LED, Button2_Pressed());  //sets the LED to current state of button each loop
     
     
     // Safely eject if Button1 is pressed for X seconds
     EjectRequestCheckAndHandler(); // Checks if the users wants to eject the SD card and handles this process
     
     
     
     ReadAnalogVoltages();  // Update AnalogRead voltages and data[...]
     
     //UpdateWriteData();  // Updates writedata based upon data && olddata, and updates holddata
     
     WriteDataToSDCard();   // Fail: Red 200, off 100, Red 200, Green 200, off 300   Sucess: Green 250
     
     // Print to the serial monitor if being used
     #ifdef USING_SERIAL
     Serial.print(count);
     Serial.print(": ");
     Serial.print(data[pos_Voltage0]);
     Serial.print(", ");
     Serial.print(data[pos_Voltage1]);
     Serial.print(", ");
     Serial.print(data[pos_Voltage2]);
     Serial.print(", ");
     Serial.print(data[pos_Voltage3]);
     Serial.println("");
     #endif
     
     // Increment count
     count++;
     
     // Delay before next loop iteration
     delay(1000); // Delay between main loop cycles
     
   }
 }
 
     
     
     
     
     
     

     
     

     
     
     
     
     
     

   
     
     
     
     
     
 // Writes data[...] to the SD card, and handles error loop (light codes) if necessary    
 void WriteDataToSDCard( void ) {
   
   // Turn on RED LED to indicate working on the SD Card
   RED_LED_ON;
   
   // Open the file
   File dataFile = SD.open(LOG_FILE_NAME, FILE_WRITE);
   if(!dataFile){ error_status = 1; } else { error_status = 0; }
   //if(SD_CARD_IS_PRESENT) {error_status = 0; } else { error_status = 1; }
   if(SD_CARD_IS_PRESENT) {error_status |= 0; } else { error_status |= 1; }
   
   #ifdef USING_SERIAL
   Serial.print(count);
   Serial.print(": error_status: ");
   Serial.println(error_status);
   #endif
   
   if (!error_status) {
     // File opened
     dataFile.print(count);
     dataFile.println(": line of data");
     dataFile.close();
     
     // Print to Serial Monitor
     #ifdef USING_SERIAL
     Serial.print(count);
     Serial.println(": line of data");
     #endif
     
     // Turn off RED LED to indicate being done with SD Card
     RED_LED_OFF;
     
   } else {
     
     // Unable to open file
     #ifdef USING_SERIAL
     Serial.println("error opening datalog.txt");
     #endif
     
     // Wait until file can be opened
     while(error_status) { // Red 200, off 100, Red 200, Green 200, off 300
       RED_LED_ON;
       delay(200);
       RED_LED_OFF;
       delay(100);
       RED_LED_ON;
       delay(200);
       RED_LED_OFF;
       GREEN_LED_ON;
       delay(200);
       GREEN_LED_OFF;
       delay(300);
       File dataFile = SD.open(LOG_FILE_NAME, FILE_WRITE);
       if(!dataFile){ error_status = 1; } else { error_status = 0; }
       if(SD_CARD_IS_PRESENT) {error_status |= 0; } else { error_status |= 1; }
     }
   } 
   
   /*for( byte i = 0 ; i < DATA_LENGTH ; ++i ){
     olddata[i] = data[i];
   }*/
 }  
     
     
 
     
 void SoftReset( void );
 void SoftReset( void ) {
   wdt_enable(WDTO_1S);
   // Watchdog Timer set up in reset mode with 1s timeout
   //WDTCSR |= (1<<WDCE) | (1<<WDE);   // Set Change Enable bit and Enable Watchdog System Reset Mode.
   //WDTCSR = (1<<WDE) | (1<<WDIE) | (0<<WDP3 )|(1<<WDP2 )|(1<<WDP1)|(0<<WDP0);
   //sei();
   GREEN_LED_ON;
   RED_LED_OFF;
   while(1){
     // This hurts to write
   }
 }  
     
 //ISR(WDT_vect) {
 //  RED_LED_ON;
 //}   
     
     
     
     
     
     
  
 // Updates writedata based upon data && olddata, and updates holddata
 void UpdateWriteData( void ) {
    
   //for( byte i = 0 ; i < DATA_LENGTH ; ++i ){
   //  olddata[i] = data[i];
   //}
    
   //if(abs(data[pos_Voltage0]-holddata[pos_Voltage0])<=tolerance_Voltage0) {
   //  // Within tolerance
   //  writedata[pos_Voltage0] = DUPLICATE;
   //} else {
   //  // Out of tolerance
   //}
    
 }
     

// **************************************************************************************
// ******************************** COMPLETE FUNCTIONS **********************************
// **************************************************************************************


 // Single debounce check if Button1 is pressed (no depressed debounce)
 boolean Button1_Pressed( void ) {
   if(digitalRead(PIN_BUTTON1)) {
     delay(DEBOUNCE_TIME);
     if(digitalRead(PIN_BUTTON1)) {
       return true;
     }
   }
   return false;
 }
  
  
 // Single debounce check if Button2 is pressed (no depressed debounce)
 boolean Button2_Pressed( void ) {
   if(digitalRead(PIN_BUTTON2)) {
     delay(DEBOUNCE_TIME);
     if(digitalRead(PIN_BUTTON2)) {
       return true;
     }
   }
   return false;
 }
  
  
 // Satisfy the Arduino compiler
 void loop() {
   // Nothing here
 }
  
  
 // Updates olddata to be the previous cycle's data
 void MoveDataToOldData( void ) {
   for( byte i = 0 ; i < DATA_LENGTH ; ++i ){
     olddata[i] = data[i];
   }
 }
  
  
 // Update AnalogRead voltages and data[...]
 void ReadAnalogVoltages( void ) {
   // Read in the analog voltages (Example: 2.34 volts yields a value of 234)
   A0_value = float(analogRead(A0) * (5.0 / 1023.0)) * 100.0;
   A1_value = float(analogRead(A1) * (5.0 / 1023.0)) * 100.0;
   A2_value = float(analogRead(A2) * (5.0 / 1023.0)) * 100.0;
   A3_value = float(analogRead(A3) * (5.0 / 1023.0)) * 100.0;
   data[pos_Voltage0] = A0_value;
   data[pos_Voltage1] = A1_value;
   data[pos_Voltage2] = A2_value;
   data[pos_Voltage3] = A3_value;
 }
 
 
 // Initializes the SD card interface and handles error loop if there is a problem  
 void InitializeSDCard( void ) {
   // see if the card is present and can be initialized:
   error_status = !SD.begin(PIN_SD_CS);
   if (error_status) {
     #ifdef USING_SERIAL
     Serial.println("Card failed, or not present");
     #endif
     
     while(error_status) {
       RED_LED_ON;
       delay(500);
       RED_LED_OFF;
       GREEN_LED_ON;
       delay(200);
       GREEN_LED_OFF;
       delay(300);
       error_status = !SD.begin(PIN_SD_CS);
     }
     
   }
   #ifdef USING_SERIAL
   Serial.println("Card initialized.");
   #endif
   
   GREEN_LED_ON;
   delay(250);
   GREEN_LED_OFF;
   delay(250);
   GREEN_LED_ON;
   delay(250);
   GREEN_LED_OFF;
   delay(250);
   GREEN_LED_ON;
   delay(250);
   GREEN_LED_OFF; 
 }
 
 
 // Handles waiting for the SD card to be ejected and another one inserted.
 void HandleSDCardSwap( void ) {

   // Wait until card is ejected
   GREEN_LED_ON;
   RED_LED_OFF;
   while(SD_CARD_IS_PRESENT) {
     // We're just waiting for the card to be ejected. That's it. Hopefully somebody ejects it.
     delay(500);
   }
   
   // Wait until a card is inserted
   while(!SD_CARD_IS_PRESENT) {
     // We're just waiting for the card to be inserted. That's it. Hopefully somebody puts one in.
     GREEN_LED_OFF;
     delay(500);
     GREEN_LED_ON;
     delay(500);
   }
   
   // Do you have to reinitialize the SD card? May need to force a watchdog timer reset.
   RED_LED_OFF;
   GREEN_LED_OFF;
   
   // Only way to reinitialize the file, without modifying sd.cpp, is a soft reset
   SoftReset();
 }
     
     
 // Checks if the users wants to eject the SD card and handles this process
 void EjectRequestCheckAndHandler( void ) { 
   if(Button1_Pressed()) {
       RED_LED_ON;
       GREEN_LED_OFF;
       
       // The following is a simple debounce for X seconds, checking once a second if the button is held down.
       for( byte i = 0 ; i < SAFE_SDCARD_EJECT_BUTTON_HOLD_SECONDS ; ++i ){
         delay(1000);
         GoodToEject = 1;
         if(!Button1_Pressed()) {
           // Button 1 is not held down!
           GoodToEject = 0;
           break;
         }
       }
       
       if(GoodToEject) {
           // Good to eject, button was pressed each check over X seconds
           HandleSDCardSwap(); // Handles waiting for the SD card to be ejected and another one inserted.
         } else {
           // Not good to eject, button wasn't held, do nothing
       }
       
       GoodToEject = 0;
   }    
 }
