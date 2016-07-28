#define VERSION "0.0.3"
#define VERSIONDATE "07-28-16"

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
 *  v0.0.3 - PJM - Setting up usage of data[...]
*   v0.0.2 - PJM -> 07-28-16 - Testing Analog Reads and writing to serial
 *  v0.0.1 - PJM -> 07-25-16 - Rewriting some original code from scratch
 * 
 *  --- Pins ------------------------
 *  // //PC4/A4 (SDA)  -> RTC SDA
 *  // //PC5/A5 (SCL)  -> RTC SCL
 *  // //D2/PD2        -> G of 33N10 (N-channel)
 *  // //A3/PC3        -> Voltage at top of AA battery.
 *  // //A2/PC2        -> Voltage at GND of AA battery.
 *  // //D9 / PB1       -> Red LED
 *  // D7 / PD7 / PCINT23       -> Red LED
 *  // D3 / PD3       -> Green LED
 *  // //PD6       -> Blue LED
 *  // //D7 / PD7 / PCINT23       -> Button 1 
 *  // D9 / PB1 / PCINT1       -> Button 1
 *  // D8 / PB0 / PCINT0      -> Button 2
 *  // A0  / PC0      -> Potentiometer
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
 *  //- When power dies, write to EEPROM, not SD card.  Then move that to SD card when you ahve power.
 *  
 *  
 */ 
 
 // Define system settings
 #define USING_SERIAL
 #define DEBOUNCE_TIME 10 // 10 ms, one debounce
 #define DATA_LENGTH 20 // Data array has 20 elements
 
 // Define pins
 #define PIN_BUTTON1 9
 #define PIN_BUTTON2 8
 #define PIN_RED_LED 6
 #define PIN_GREEN_LED 7
 
 // Define array element positions for data
 #define pos_Voltage0 0
 #define pos_Voltage1 1
 #define pos_Voltage2 2
 #define pos_Voltage3 3
 
 // Initialize variables
 boolean Button1_Pressed( void );
 long count = 0;
 word data[DATA_LENGTH] = {0};      // Updated data (e.g. analog inputs or charge controller received data) placed here
 word olddata[DATA_LENGTH] = {0};   // Previous loops data is stored here for comparison purposes
 word writedata[DATA_LENGTH] = {0}; // This is the data to be written to the SD card
 word A0_value, A1_value, A2_value, A3_value;
 
 // Main program
 void setup() {
   // Initialize system
   pinMode(PIN_BUTTON1, INPUT);
   pinMode(PIN_BUTTON2, INPUT);
   pinMode(PIN_RED_LED, OUTPUT);
   pinMode(PIN_GREEN_LED, OUTPUT);
   //**Pin mode for analog reads?
   
   // Initialize serial communication at 9600 bits per second:
   #ifdef USING_SERIAL
   Serial.begin(9600);
   #endif
   
   // Primary code loop
   while(1)
   {
     // Update olddate to have the previous loop's data
     for( byte i = 0 ; i < DATA_LENGTH ; ++i ){
       olddata[i] = data[i];
     }
     
     // Set the LEDs to the current state of the buttons
     digitalWrite(PIN_GREEN_LED, Button1_Pressed());  //sets the LED to current state of button each loop
     digitalWrite(PIN_RED_LED, Button2_Pressed());  //sets the LED to current state of button each loop
     
     // Read in the analog voltages (Example: 2.34 volts yields a value of 234)
     A0_value = float(analogRead(A0) * (5.0 / 1023.0)) * 100.0;
     A1_value = float(analogRead(A1) * (5.0 / 1023.0)) * 100.0;
     A2_value = float(analogRead(A2) * (5.0 / 1023.0)) * 100.0;
     A3_value = float(analogRead(A3) * (5.0 / 1023.0)) * 100.0;
     data[pos_Voltage0] = A0_value;
     data[pos_Voltage1] = A1_value;
     data[pos_Voltage2] = A2_value;
     data[pos_Voltage3] = A3_value;
     
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
