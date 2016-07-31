#define VERSION "0.0.8"
#define VERSIONDATE "07-31-16"

/* System functionality:
 *  - System start (fresh or from a reset)
 *  - If blinking red, SD card read error (sd card is unreadable or not inserted)
 *  - Blinks green for X seconds (allowing user to switch from having just pressed the reset button to pressing button 1 if they wish to swap SD cards)
 *  - (MAIN LOOP START)
 *  - User must hold Button 1 at this point if a swap of SD cards is desired
 *    -SWAP:
 *    - Hold Button 1 for until RED illuminates, continue holding for X seconds.
 *    - RED off, GREEN solid.
 *      - RESET to cancel
 *      - Safe to remove card at this time.
 *      - Once card is removed, green will blink until a new sd card is inserted
 *      - System will alternate flashing RED and GREEN, waiting for RESET to be pressed (done with swap)
 *  - RED will illuminate, indicating it is gathering data or writing to SD card
 *  - If blinking red with slight green flash (RRRRRxxxxxRRRRRgg...), SD card FILE read error. Try a reset, possibly need a new SD card or to format it. Once resolved, system will alternate flashing RED/GREEN. Must press RESET.
 *  - LEDs off, system delays X minutes (does not sleep). If user wishes to swap SD card, press RESET while no RED on, then button 1.
 *  - (MAIN LOOP END)
 *  
 *  07-30-16 - Measured, system draws from 12V source ~70mA while in primary delay(...). On a 110 Ah battery, this will take 1571 hours to drain. System is designed to last 48 hours. 48 hours at 70mA is 3360mAh, or a usage of 3% of the system's total power. Without sleeping the unit. 
 */ 


/* Places program can get stuck:
   - Initializing the SD card if failure: Blinks red
   - Opening the datafile if failure or if no SD card present: Blinks red with slight green flash. After fix, blinks alternating RED/GREEN, needs RESET
   - Waiting for SD card to be ejected after holding the button. Solid green light.
   - Waiting for SD card to be reinserted after ejected. Blinking green.
   - Waiting for user to physically reset system via a physical reset after inserting new SD card. Blinks alternating RED/GREEN, needs RESET.
*/


/* TO DO:
 *  - Need to find two pins that can be pulled up/down to identify unique logging units
    - Code data retrieval from CC, including error handling
    - Write all of this data to the SD card
    - Verify that sleeping between cycles does not interfere with anything
    - If no compression, is MoveDataToOldData(); needed?
    - Write short manual for what to do for when red LED is on, and then for how to swap cards, etc.
    - REDO ALL OF THE ABOVE:  Write wall logger.
    - Implement data tolerance and compression? (e.g. the whole thing where if the new value is within tolerance of the old one, just say it is the old value, and if it is the old value, say it is "x". Then look at the line, and replace "x,"'s with a, b, c... e.g.   "x,x,x," => "c" (a=1, b=2, c=3).
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
 *  v0.0.8 - PJM -> 07-31-16 - System cleaned up and working: No reading real data from analog, no reading data from CC. Just the SD card workings.
  * v0.0.7 - PJM -> 07-30-16 - (see "issues" in code) (WOrking on)
 *  v0.0.6 - PJM - (working on adding a watchdog timer reset so it can continue to write after an eject, then adding formatting) (chose to not add formatting)
 *  v0.0.5 - PJM -> 07-29-16 - SD Card writing (dummy data) and safe eject
 *  v0.0.4 - PJM - (working on data compression and delta tolerance) (not done, skipping)
 *  v0.0.3 - PJM - Setting up usage of data[...]
 *  v0.0.2 - PJM -> 07-28-16 - Testing Analog Reads and writing to serial
 *  v0.0.1 - PJM -> 07-25-16 - Rewriting some original code from scratch
 * 
 *  --- Pins ------------------------
 *  // *** WALL LOGGER ***
 *  // PC4/A4 (SDA)  -> RTC SDA
 *  // PC5/A5 (SCL)  -> RTC SCL
 *  // Use D2/D3 - has INT0/INT1, use for brownout interrupt
 *  //- Supercap calc:  http://electronics.stackexchange.com/questions/4951/how-do-i-calculate-how-fast-a-capacitor-will-discharge
 *  //- For wall logger, each loop write data to a variable. When power dies, write variables to EEPROM, not SD card.  Then move that to SD card when you ahve power.
 *  
 *  *** S4 DATA LOGGER ***
 *  D13 / PB5 / 13 / SCK        -        SD CLK
 *  D12 / PB4 / 12 / MISO       -        SD DO
 *  D11 / PB3 / 11 / MOSI       -        SD DI
 *  D10 / PB2 / 10 / SS         -        SD CS      // Can be changed to arbitrary GPIO pin
 *  D4  / PD4 /  4              -        SD CD      // Can be changed to arbitrary GPIO pin
 *  D2  / PD2 /  2 / INT0       -        Button 1   // Can be changed to arbitrary GPIO pin (Pins 2 and 3 can wake from sleep if implemented)
 *  D7  / PD7 /  7              -        Green LED  // Can be changed to arbitrary GPIO pin
 *  D6  / PD6 /  6              -        Red LED    // Can be changed to arbitrary GPIO pin
 *  D8  / PB0 /  8              -        ID0 (10k pull-up/down) (System ID is ID0&ID1, so 00, 01, 10, or 11, or 0, 1, 2, or 3)
 *  D9  / PB1 /  9              -        ID1 (10k pull-up/down)
 *  A0  / PC0 / 14 / Analog 0   -        (Temp) Rocker 1
 *  A1  / PC1 / 15 / Analog 1   -        (Temp) Rocker 2
 *  A2  / PC2 / 16 / Analog 2   -        (Temp) Rocker 3
 *  A3  / PC3 / 17 / Analog 3   -        (Temp) Rocker 4
 *  
 *  
 *  --- Notes -----------------------
 *
 */ 
 
 // Define system settings
 #define USING_SERIAL
 #define DEBOUNCE_TIME 10 // 10 ms, one debounce
 #define SYSTEM_BOOT_TIME 5 // When the system starts up (such as after a reset) it flashes the green led for this many seconds before starting the main code
 #define DATA_LENGTH 20 // Data array has 20 elements
 #define LOG_FILE_NAME "datalog.txt"
 #define SAFE_SDCARD_EJECT_BUTTON_HOLD_SECONDS 3 // 3 seconds of holding button1 to safely eject
 #define PRIMARY_DELAY_SECONDS 10 // 300 seconds is 5 minutes. Number of seconds between subsequent data read/writes. (sleep time, essentially, sans actual mcu sleep)


 // Define pins
 #define PIN_BUTTON1 2
 #define PIN_RED_LED 6
 #define PIN_GREEN_LED 7
 #define PIN_SD_CS 10
 #define PIN_SD_CD 4
 #define PIN_ID0 8
 #define PIN_ID1 9

 
 // Define "functions"
 #define RED_LED_ON digitalWrite(PIN_RED_LED, 1)
 #define RED_LED_OFF digitalWrite(PIN_RED_LED, 0)
 #define GREEN_LED_ON digitalWrite(PIN_GREEN_LED, 1)
 #define GREEN_LED_OFF digitalWrite(PIN_GREEN_LED, 0)
 #define SD_CARD_IS_PRESENT digitalRead(PIN_SD_CD)

 
 // Define array element positions for data
 #define pos_Voltage0 0
 #define pos_Voltage1 1
 #define pos_Voltage2 2
 #define pos_Voltage3 3
 #define pos_SystemID 4

 
 // Includes
 #include <SD.h>
 #include <avr/interrupt.h>

 
 // Declare functions
 boolean Button1_Pressed( void );
 void MoveDataToOldData( void );
 void ReadAnalogVoltages( void );  // Update AnalogRead voltages and data[...]
 void InitializeSDCard( void );  // Initializes the SD card interface, and handles error loop (light codes) if necessary
 void WriteDataToSDCard( void ); // Writes data[...] to the SD card, and handles error loop (light codes) if necessary
 void HandleSDCardSwap( void ); // Handles waiting for the SD card to be ejected and another one inserted.
 void EjectRequestCheckAndHandler( void ); // Checks if the users wants to eject the SD card and handles this process
 byte SystemID( void ); // Returns a systemID (10k pull-up/downs on pins 8 and 9) (System ID is ID0&ID1, so 00, 01, 10, or 11, or 0, 1, 2, or 3)

 
 // Initialize variables
 long count = 0;
 word data[DATA_LENGTH] = {0};          // Updated data (e.g. analog inputs or charge controller received data) placed here
 word olddata[DATA_LENGTH] = {0};       // Previous loops data is stored here for comparison purposes
 word writedata[DATA_LENGTH] = {0};     // This is the data to be written to the SD card (e.g. olddata="5,2,1" && data="4,3,1" -> writedata="4,3,x")
 word holddata[DATA_LENGTH] = {0};      // This is the data that any "old" writedata represents (cont. above e.g.: writedata="4,3,1")
 word A0_value, A1_value, A2_value, A3_value;
 byte error_status = 0;
 byte GoodToEject = 0;

 
 // Main program
 void setup() {
   
   // Initialize pins
   pinMode(PIN_BUTTON1, INPUT);
   pinMode(PIN_RED_LED, OUTPUT);
   pinMode(PIN_GREEN_LED, OUTPUT);
   pinMode(PIN_SD_CS, OUTPUT);
   pinMode(PIN_SD_CD, INPUT);
   pinMode(PIN_ID0, INPUT);
   pinMode(PIN_ID1, INPUT);
   
   // Initialize serial communication at 9600 bits per second:
   #ifdef USING_SERIAL
   Serial.begin(9600);
   #endif
   
   InitializeSDCard(); // Blinks RED if no SD card

   // Give the user some time to switch to pressing Button1 if desired after resetting the system
   // This way they can press reset while it is in the main delay(...) and then press Button1 to swap SD cards.
   for( byte i = 0 ; i < SYSTEM_BOOT_TIME ; ++i ){
     GREEN_LED_ON;
     delay(500);
     GREEN_LED_OFF;
     delay(500);
   }


   // Primary code loop
   while(1)
   {
     
     MoveDataToOldData();  // Updates olddata to be the previous cycle's data 
     
     // Safely eject if Button1 is pressed for X seconds
     EjectRequestCheckAndHandler(); // Checks if the users wants to eject the SD card and handles this process

     // Turn on RED LED to indicate running critical processes (i.e. do NOT press RESET)
     RED_LED_ON;
     delay(1000); // Give it a second so they don't press the button just after everything starts
     
     ReadAnalogVoltages();  // Update AnalogRead voltages and data[...]
     data[pos_SystemID] = (word)SystemID();  // Update data[...] with SystemID
     
     WriteDataToSDCard();   // Fail: Red 200, off 100, Red 200, Green 200, off 300   Sucess: Green 250

    // Turn off RED LED to indicate done with critical processes (RESET can be pressed)
    RED_LED_OFF;
     
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
     Serial.print(", System ID: ");
     Serial.print(data[pos_SystemID]);
     Serial.println("");
     #endif
     
     // Increment count
     count++;
     if(count>10000) { count = 0; } // Mostly for debugging.
     
     // Delay before next loop iteration
     delay(1000*PRIMARY_DELAY_SECONDS); // Delay between main loop cycles
     
   }
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


  
 // Writes data[...] to the SD card, and handles error loop (light codes) if necessary    
 void WriteDataToSDCard( void ) {
   
   // Open the file
   File dataFile = SD.open(LOG_FILE_NAME, FILE_WRITE);
   if(!dataFile){ error_status = 1; } else { error_status = 0; }
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
     
   } else {
     
     // Unable to open file
     #ifdef USING_SERIAL
     Serial.println("error opening datalog.txt");
     #endif
     
     // Wait until file can be opened
     while(error_status) {
       RED_LED_ON;
       delay(500);
       RED_LED_OFF;
       delay(500);
       RED_LED_ON;
       delay(500);
       RED_LED_OFF;
       GREEN_LED_ON;
       delay(250);
       GREEN_LED_OFF;
       File dataFile = SD.open(LOG_FILE_NAME, FILE_WRITE);
       if(!dataFile){ error_status = 1; } else { error_status = 0; }
       if(SD_CARD_IS_PRESENT) {error_status |= 0; } else { error_status |= 1; }
     }

     while(1) {
     // User must manually reset the device at this point
     // Only way to reinitialize the file without modifying sd.cpp
     GREEN_LED_ON;
     RED_LED_OFF;
     delay(500);
     RED_LED_ON;
     GREEN_LED_OFF;
     delay(500);
     }
   } 
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
   // See if the card is present and can be initialized:
   error_status = !SD.begin(PIN_SD_CS);
   if (error_status) {
     #ifdef USING_SERIAL
     Serial.println("Card failed, or not present");
     #endif
     
     while(error_status) {
       RED_LED_ON;
       delay(500);
       RED_LED_OFF;
       delay(500);
       error_status = !SD.begin(PIN_SD_CS);
     }
     
   }
   #ifdef USING_SERIAL
   Serial.println("Card initialized.");
   #endif
   
   // No green/etc flash if successful, program just continues
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
   
   while(1) {
     // User must manually reset the device at this point
     // Only way to reinitialize the file without modifying sd.cpp
     GREEN_LED_ON;
     RED_LED_OFF;
     delay(500);
     RED_LED_ON;
     GREEN_LED_OFF;
     delay(500);
   }
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
           GREEN_LED_OFF;
           RED_LED_OFF;
         } else {
           // Not good to eject, button wasn't held, do nothing
       }
       
       GoodToEject = 0;
   }    
 }

 // Returns a systemID (10k pull-up/downs on pins 8 and 9) (System ID is ID0&ID1, so 00, 01, 10, or 11, or 0, 1, 2, or 3)
 byte SystemID( void ) {
   byte ID = 0;
   ID |= digitalRead(PIN_ID0) << 1;
   ID |= digitalRead(PIN_ID1);
   return ID;
 }
 
