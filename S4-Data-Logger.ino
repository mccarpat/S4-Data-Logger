#define VERSION "0.0.1"
#define VERSIONDATE "07-25-16"
//temptestline

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
 
 #define PIN_BUTTON1 9
 #define PIN_BUTTON2 8
 #define PIN_RED_LED 6
 #define PIN_GREEN_LED 7
 
 void setup() {
   // Initialize system
   pinMode(PIN_BUTTON1, INPUT);
   pinMode(PIN_BUTTON2, INPUT);
   pinMode(PIN_RED_LED, OUTPUT);
   pinMode(PIN_GREEN_LED, OUTPUT);
   
   
   while(1)
   {
     // Primary code loop
     digitalWrite(PIN_GREEN_LED, digitalRead(PIN_BUTTON1));  //sets the LED to current state of button
     digitalWrite(PIN_RED_LED, digitalRead(PIN_BUTTON2));  //sets the LED to current state of button
     delay(5); // delay to not burn out cpu
   }
 }
 
 
 void loop() {
   // Nothing here
 }
