/*
Features:
 -no Delay
 -temp sensor DS18 and time on lcd
 -on/off relays
*/

//DS18B20 sensor
#include <OneWire.h>
#include <DallasTemperature.h>

//clock library
#include <Time.h>
#include <DS1307RTC.h>

//DISPLAY SET
#include <Wire.h>
//#include <LCD.h>
#include <LiquidCrystal_I2C.h>

#define I2C_ADDR    0x27 // <<----- Add your address here.  Find it from I2C Scanner
#define BACKLIGHT_PIN     3
#define En_pin  2
#define Rw_pin  1
#define Rs_pin  0
#define D4_pin  4
#define D5_pin  5
#define D6_pin  6
#define D7_pin  7

int n = 1;

LiquidCrystal_I2C	lcd(I2C_ADDR,En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin);
//END DISPLAY SET


//DS18B20 sensor setup
// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// arrays to hold device address
DeviceAddress ds18_1;

// Other setups
// set pin numbers:
const int ledPin = 13;      // the number of the LED pin
int sensorPin0 = A0;         //pumpOnTimeHour - starting the pump 
int sensorPin1 = A1;         //pumpTimeDutyOn - pump duty cycle
unsigned int pin1_value = 0;

// Variables will change:
int ledState = LOW;             // ledState used to set the LED

// Pump Relay pin number
const int pumpPin = 8;
int pumpOnTimeHour = 0;
int pumpOnTimeMinute = 10;
unsigned long pumpTimeDutyOn = 15000; //miliseconds
int pumpTimeDutyOff = 30000; //miliseconds
int pumpTimeDutyRepeats = 1; //times to run
int pumpState = 0;           //pump is on or off

// Light Relay pin number
const int lightPin = 9;     
int lightOnTime = 6;
int lightOffTime = 22;

// Timers in miliseconds:
unsigned long previous_led_blink_Millis = 0;        // will store last time LED was updated
unsigned long interval_led_blink = 1000;           // interval at which to blink (milliseconds)
unsigned long previous_temp_get_Millis = 0;
unsigned long interval_temp_get = 15000;
unsigned long previous_light_start_check_Millis = 0;        // will store last time LED was updated
unsigned long interval_light_start_check = 10000;           // (10 sec) interval at which to check time for light and watering

void setup() {
  // start serial port
  Serial.begin(9600);
  Serial.println("Show stuff on lcd with delays...");
  delay(1000);
  
  //Get time from RTC
  //RTC.set(1407544080); // set the RTC and the system time
  setSyncProvider(RTC.get);   // the function to get the time from the RTC 
  //setTime(hour(),minute(),second(),day(),month(),year()); // set time to Saturday 8:29:00am Jan 1 2011
  //setTime(0,0,00,30,8,2014);
  welcome_message();
  
  // set the digital pin as output:
  pinMode(ledPin, OUTPUT);
  pinMode(lightPin, OUTPUT);
  pinMode(pumpPin, OUTPUT);
  
  digitalWrite(pumpPin, HIGH); //turn pump off

  // locate devices on the bus
  Serial.print("Locating devices...");
  sensors.begin();
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  // assign address manually.  the addresses below will beed to be changed
  // to valid device addresses on your bus.  device address can be retrieved
  // by using either oneWire.search(deviceAddress) or individually via
  // sensors.getAddress(deviceAddress, index)
  //insideThermometer = { 0x28, 0x1D, 0x39, 0x31, 0x2, 0x0, 0x0, 0xF0 };

  // Method 1:
  // search for devices on the bus and assign based on an index.  ideally,
  // you would do this to initially discover addresses on the bus and then 
  // use those addresses and manually assign them (see above) once you know 
  // the devices on your bus (and assuming they don't change).
  if (!sensors.getAddress(ds18_1, 0)) Serial.println("Unable to find address for Device 0"); 

  // method 2: search()
  // search() looks for the next device. Returns 1 if a new address has been
  // returned. A zero might mean that the bus is shorted, there are no devices, 
  // or you have already retrieved all of them.  It might be a good idea to 
  // check the CRC to make sure you didn't get garbage.  The order is 
  // deterministic. You will always get the same devices in the same order
  //
  // Must be called before search()
  //oneWire.reset_search();
  // assigns the first address found to insideThermometer
  //if (!oneWire.search(insideThermometer)) Serial.println("Unable to find address for insideThermometer");

  // show the addresses we found on the bus
  Serial.print("Device 0 Address: ");
  printAddress(ds18_1);
  Serial.println();

  light_on();                  //turn light up
  //delay(3000);  
}

void loop()
{
  // here is where you'd put code that needs to be running all the time.
   //delay(950);
   pumpOnTimeHour = map(analogRead(sensorPin0), 0, 1023, 0, 23);
   pin1_value = map(analogRead(sensorPin1), 0, 1023, 0, 60);
   pumpTimeDutyOn = pin1_value * 1000 * 60; // run the pump in minutes
   pump_ON(); //used to test the pump 
   
  //=======temp get begin ===========//
  unsigned long current_temp_get_Millis = millis();

  if ((current_temp_get_Millis - previous_temp_get_Millis) > interval_temp_get) {
    previous_temp_get_Millis = millis();
    
    
    sensors.requestTemperatures(); // Send the command to get temperatures
    // It responds almost immediately. Let's print out the data
    printTemperature(ds18_1); // Use a simple function to print out the data
    display_temp_on_lcd(ds18_1);
  }
  else{
  }
  //======temp get end =============// 

  // ====== blink LED begin ============ //
  unsigned long current_led_blink_Millis = millis();

  if(current_led_blink_Millis - previous_led_blink_Millis > interval_led_blink) {
    // save the last time you blinked the LED 
    previous_led_blink_Millis = current_led_blink_Millis;   

    //display refresh
    digitalClockDisplay_serial();
    digitalClockDisplay_lcd();
    lcd.setCursor (12,0);        // [place],[line] --> go to after the clock on line 1
    lcd.print("H=");
    if (pumpOnTimeHour < 10) lcd.print('0');
    lcd.print(pumpOnTimeHour);
    Serial.print("Watering time On=");
    Serial.println(pumpOnTimeHour);
    lcd.setCursor (6,0);        // [place],[line] --> go to after the clock on line 1
    lcd.print("D=");
    if (pin1_value < 10) lcd.print('0');
    lcd.print(pin1_value);
    Serial.print("Pump On Duration=");
    Serial.print(pin1_value);
    Serial.println(" minutes.");
    Serial.print("Pump Test value=");
    Serial.println(analogRead(sensorPin1));

    // toggle the LED state
    if (ledState == LOW) {
      ledState = HIGH;
    }
    else {
      ledState = LOW;
    }
    // set the LED with the ledState of the variable:
    digitalWrite(ledPin, ledState);
  }
  else{
  }
  //======== blink LED end ==========//
  
  //=======light_start_check begin ===========//
  unsigned long current_light_start_check = millis();

  if ((current_light_start_check - previous_light_start_check_Millis) > interval_light_start_check) {
    previous_light_start_check_Millis = millis();
    
    //lcd.clear(); //refresh the lcd
    
    // Check time to light lights UP
    if ( hour() >= lightOnTime && hour() < lightOffTime && !digitalRead(lightPin) ) {
      light_on();
    }
    
    // Check time to shut lights DOWN
    if ( hour() >= lightOffTime || hour() < lightOnTime ) {
      light_off();
    }
    
    // Check time to water the plants
    if (hour() == pumpOnTimeHour && minute() == pumpOnTimeMinute) {
      pump_cycle(pumpTimeDutyOn, pumpTimeDutyOff, pumpTimeDutyRepeats); 
    }
  }
  else{
  }
  //======light_start_check end =============// 
}
// function to print the temperature for a device
void printTemperature(DeviceAddress deviceAddress) //28331531050000F6
{
  // method 2 - faster
  float tempC = sensors.getTempC(deviceAddress);
  Serial.print("Temp C: ");
  Serial.println(tempC,1);
}
// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

void display_temp_on_lcd (DeviceAddress deviceAddress) {

  lcd.setCursor (6,1);        // [place],[line] --> go to after the clock on line 1
  lcd.print("T=");
  lcd.print(sensors.getTempC(deviceAddress),1); //displays 1 decimal symbol
  lcd.print((char)223); //degree symbol
};
void digitalClockDisplay_serial(){
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print("/");
  Serial.print(month());
  Serial.print("/");
  Serial.print(year()); 
  Serial.println(); 
}
void digitalClockDisplay_lcd(){
  //lcd.clear();
  lcd.setCursor (0,0);        // go to start of 1nd line
  // digital clock display of the time
  lcd.print(hour());
  printDigits_lcd(minute());
  //printDigits_lcd(second());
  //lcd.print(" ");
  lcd.setCursor (0,1);        // go to start of 1nd line
  lcd.print(day());
  lcd.print(".");
  lcd.print(month());
  //lcd.print("/");
  //lcd.print(year()); 
  //lcd.println(); 
}
void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}
void printDigits_lcd(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  lcd.print(":");
  if(digits < 10)
    lcd.print('0');
  lcd.print(digits);
}
void light_on() {
    digitalWrite(lightPin, LOW);
    Serial.println("Light is ON.");
    lcd.setCursor(15,1);
    lcd.print("*");
}

void light_off() {
    digitalWrite(lightPin, HIGH);
    Serial.println("Light is OFF.");
    lcd.setCursor(15,1);
    lcd.print(" ");
}

void pump_cycle(unsigned long On, int Off, int Repeats) {
    for(int i=1; i<=Repeats; i++) {
      lcd.setCursor(14,1);
      lcd.print("W");
      pwm_pump_on();
      delay(On);
      lcd.setCursor(14,1);
      lcd.print("_");
      digitalWrite(pumpPin, HIGH);
      if (i != Repeats)
        delay(Off);
    }
    lcd.setCursor(14,1);
    lcd.print(" ");
}

void welcome_message() {
  lcd.begin (16,2); //  <<----- My LCD was 16x2
  // Switch on the backlight
  lcd.setBacklightPin(BACKLIGHT_PIN,POSITIVE);
  lcd.setBacklight(HIGH);
  lcd.clear();
  lcd.home();
  if ( month() == 8 && day() == 30 ) {
    delay(100);
    lcd.setBacklight(LOW);
    delay(100);
    lcd.setBacklight(HIGH);
    delay(150);
    lcd.setBacklight(LOW);
    mitko_birthday();
    delay(75);
    lcd.setBacklight(HIGH);
    delay(200);
    lcd.setBacklight(LOW);
    delay(50);
    lcd.setBacklight(HIGH);
    delay(150);
    lcd.setBacklight(LOW);
    delay(40);
    lcd.setBacklight(HIGH);
    delay(45);
    lcd.setBacklight(LOW);
    delay(35);
    lcd.setBacklight(HIGH);
    delay(40);
    lcd.setBacklight(LOW);
    delay(30);
    lcd.setBacklight(HIGH);
  }
  else {
    lcd.home (); // go home
    lcd.setCursor (0,0);
    lcd.print("Watering system");
    lcd.setCursor (4,1);        // [position] [line]
    lcd.print("to Mitko");
    delay(1500);
    lcd.setCursor (2,1);
    lcd.print("from Kaloyan");
  }
  delay(3000);
  lcd.clear();
}

void mitko_birthday(){
  lcd.home (); // go home
  lcd.setCursor (1,0);
  lcd.print("Happy Birthday");
  lcd.setCursor (5,1);        // go to start of 2nd line
  lcd.print("Mitko");
}

void pump_ON() {
  pumpState = digitalRead(pumpPin);
  if (analogRead(sensorPin1) >= 1000 && pumpState == HIGH) {
    delay(1000);
    if (analogRead(sensorPin1) >= 1000) {
      lcd.setCursor(14,1);
      lcd.print("W");
      pwm_pump_on();
    }
  }
  else {
      if (analogRead(sensorPin1) < 900) {
      delay(100);
      if (analogRead(sensorPin1) < 900) {
        digitalWrite(pumpPin, HIGH); //turn the pump off
        lcd.setCursor(14,1);
        lcd.print(" ");
      }
    }
  }
}

void pwm_pump_on() {

      lcd.setBacklight(LOW);
//      delay(10);
//      digitalWrite(pumpPin, LOW); //turn the pump on
//      delay(5);
//      digitalWrite(pumpPin, HIGH); //turn the pump off
//      delay(15);
//      digitalWrite(pumpPin, LOW); //turn the pump on
//      delay(10);
//      digitalWrite(pumpPin, HIGH); //turn the pump off
//      delay(10);
      digitalWrite(pumpPin, LOW); //turn the pump on
      delay(15);
      digitalWrite(pumpPin, HIGH); //turn the pump off
      delay(3);
      digitalWrite(pumpPin, LOW); //turn the pump on
      delay(20);
      digitalWrite(pumpPin, HIGH); //turn the pump off
      delay(3);
      digitalWrite(pumpPin, LOW); //turn the pump on
      delay(30);
      digitalWrite(pumpPin, HIGH); //turn the pump off
      delay(3);
      digitalWrite(pumpPin, LOW); //turn the pump on
      delay(40);
      digitalWrite(pumpPin, HIGH); //turn the pump off
      delay(3);
      digitalWrite(pumpPin, LOW); //turn the pump on
      delay(50);
      digitalWrite(pumpPin, HIGH); //turn the pump off
      delay(3);
      digitalWrite(pumpPin, LOW); //turn the pump on
      delay(60);
      digitalWrite(pumpPin, HIGH); //turn the pump off
      delay(3);
      digitalWrite(pumpPin, LOW); //turn the pump on
      delay(75);
      digitalWrite(pumpPin, HIGH); //turn the pump off
      delay(3);
      digitalWrite(pumpPin, LOW); //turn the pump on
      delay(100);
      digitalWrite(pumpPin, HIGH); //turn the pump off
      delay(3);
      digitalWrite(pumpPin, LOW); //turn the pump on
      delay(200);
      digitalWrite(pumpPin, HIGH); //turn the pump off
      delay(2);
      digitalWrite(pumpPin, LOW); //turn the pump on
      delay(300);
      digitalWrite(pumpPin, HIGH); //turn the pump off
      delay(1);
      digitalWrite(pumpPin, LOW); //turn the pump on
      lcd.setBacklight(HIGH);

}
