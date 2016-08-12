#include <DS3232RTC.h>
#include <Time.h>
#include <Wire.h>
#include "DHT.h"

DHT dht;
const int dhtPin = 12;

//// Set your own pins with these defines !
//#define DS1302_SCLK_PIN   6    // Arduino pin for the Serial Clock
//#define DS1302_IO_PIN     7    // Arduino pin for the Data I/O
//#define DS1302_CE_PIN     8    // Arduino pin for the Chip Enable
//
//#define DS1302_GND_PIN 9
//#define DS1302_VCC_PIN 10
//
//// Init the DS1302
//// Set pins:  CE, IO,CLK
//DS1302RTC rtc(DS1302_CE_PIN, DS1302_IO_PIN, DS1302_SCLK_PIN);
DS3232RTC rtc;

#include <Wire.h> 
#include <LiquidCrystal_I2C.h>



#define BACKLIGHT_PIN     13

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); // Addr, En, Rw, Rs, d4, d5, d6, d7, backlighpin, polarity

//LiquidCrystal_I2C lcd(0x38, BACKLIGHT_PIN, POSITIVE);  // Set the LCD I2C address


// Creat a set of new characters
const uint8_t charBitmap[][8] = {
   { 0xc, 0x12, 0x12, 0xc, 0, 0, 0, 0 },
   { 0x6, 0x9, 0x9, 0x6, 0, 0, 0, 0 },
   { 0x0, 0x6, 0x9, 0x9, 0x6, 0, 0, 0x0 },
   { 0x0, 0xc, 0x12, 0x12, 0xc, 0, 0, 0x0 },
   { 0x0, 0x0, 0xc, 0x12, 0x12, 0xc, 0, 0x0 },
   { 0x0, 0x0, 0x6, 0x9, 0x9, 0x6, 0, 0x0 },
   { 0x0, 0x0, 0x0, 0x6, 0x9, 0x9, 0x6, 0x0 },
   { 0x0, 0x0, 0x0, 0xc, 0x12, 0x12, 0xc, 0x0 }
   
};

#include <Timezone.h>    //https://github.com/JChristensen/Timezone

//US Eastern Time Zone (New York, Detroit)
TimeChangeRule myDst = {"EDT", Second, Sun, Mar, 2, -240};    //UTC - 4 hours
TimeChangeRule myStd = {"EST", First, Sun, Nov, 2, -300};     //UTC - 5 hours
Timezone myTZ(myDst, myStd);

TimeChangeRule *tcr;

void setup()
{
  
  pinMode(13, OUTPUT); // DHT power
  digitalWrite(13, HIGH);
  pinMode(dhtPin, INPUT);  // DHT signal
  
  Serial.begin(115200);
   int charBitmapSize = (sizeof(charBitmap ) / sizeof (charBitmap[0]));

  // Switch on the backlight
  pinMode ( BACKLIGHT_PIN, OUTPUT );
  digitalWrite ( BACKLIGHT_PIN, HIGH );
  
  lcd.begin(16,2);               // initialize the lcd 

   for ( int i = 0; i < charBitmapSize; i++ )
   {
      lcd.createChar ( i, (uint8_t *)charBitmap[i] );
   }

  lcd.home ();                   // go home
//  lcd.print("Hello, ARDUINO ");  
//  lcd.setCursor ( 0, 1 );        // go to the next line
//  lcd.print (" FORUM - fm   ");
//  delay ( 1000 );
//  lcd.setBacklight(0);
//  delay ( 1000 );
  lcd.clear();
  lcd.print("Initializing...");
  lcd.setBacklight(1);
  

  delay(500);
  
//   // Check clock oscillation  
  lcd.clear();
  
  // Check write-protection
  lcd.setCursor(0,1);
  
  delay ( 2000 );
  // Setup Time library  
  lcd.clear();
  lcd.print("RTC Sync");
  setSyncProvider(rtc.get); // the function to get the time from the RTC
  if(timeStatus() == timeSet)
    lcd.print(" Ok!");
  else
    lcd.print(" FAIL!");

  delay ( 2000 );

  lcd.clear();
  
  lcd.setCursor(0, 0);
  lcd.print("DHT setup: ");
  dht.setup(dhtPin); 
  delay(dht.getMinimumSamplingPeriod());
  lcd.setCursor(2, 1);
  lcd.print(dht.getStatusString());
  delay(1500);
  lcd.clear();  
}

float temperature = -1;
float humidity = -1;
char setBuf[12];
long m = 0;
long lastM = -1;
long lastM2 = -1;
long lastM3 = -1;
bool dateNotTemp = true;
bool utcNotLocal = false;

void loop()
{
  m = millis();
  if (m - lastM >= dht.getMinimumSamplingPeriod()) {
    temperature = dht.getTemperature();
    humidity = dht.getHumidity();
    lastM = m;
  }
  
  memset(setBuf, 0, sizeof(setBuf));
  
  if (Serial.available() > 4) {
    if ('S' == Serial.read()) {
      for (int i=0; Serial.available() && i < sizeof(setBuf); i++) {
        setBuf[i] = Serial.read();
      }
    }
  }
  
  if (*setBuf) {
    long unixtime = atol(setBuf);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Setting time");    
    lcd.setCursor(2, 1);
    lcd.print(setBuf);
    rtc.set(unixtime);
    delay(500);
    lcd.clear();
    lcd.print("Time has reset");
    delay(500);
    lcd.clear();
  }
  
  // Display time centered on the upper line 
  time_t utc = rtc.get(); //now();
  time_t x = utc;
  if (!utcNotLocal) {
    x = myTZ.toLocal(utc, &tcr);
  }
  
  lcd.setCursor(4, 0);
  print2digits(hour(x));
  lcd.print(":");
  print2digits(minute(x));
  lcd.print(":");
  print2digits(second(x));
  lcd.print(" ");  
  lcd.print(utcNotLocal ? "UTC" : tcr->abbrev);


  lcd.setCursor(0, 1);
  if (dateNotTemp) {
    // Display abbreviated Day-of-Week in the lower left corner
    lcd.print(dayShortStr(weekday()));
    lcd.print("  ");
    // Display date in the lower right corner
    lcd.setCursor(5, 1);
    lcd.print(" ");
    print2digits(month(x));
    lcd.print("/");
    print2digits(day(x));
    lcd.print("/");
    lcd.print(year(x));
  }
  else {
    DHT::DHT_ERROR_t ds = dht.getStatus();
    switch(ds) {
      case DHT::ERROR_NONE:
        lcd.print((int)humidity);
        lcd.print(" %RH ");
        lcd.print(dht.toFahrenheit(temperature));
        lcd.print(" *F ");
      break;
      default:
        lcd.print(dht.getStatusString());
      break;
    }
  }

  if (m - lastM2 > 5000) { 
    dateNotTemp = !dateNotTemp;
    lastM2 = m;
  }
  
//  if (m - lastM3 > 2000) {
//    utcNotLocal = !utcNotLocal;
//    lastM3 = m;
//  }
  
  // Warning!
  if(timeStatus() != timeSet) {
    lcd.setCursor(0, 1);
    lcd.print(F("RTC ERROR: SYNC!"));
  }

  doLog();
  delay ( 1000 ); // Wait approx 1 sec
}



void print2digits(int number) {
  // Output leading zero
  if (number >= 0 && number < 10) {
    lcd.write('0');
  }
  lcd.print(number);
}

void doLog() {
  time_t utc = rtc.get();
//  printTime(utc, "UTC");
  time_t local = myTZ.toLocal(utc, &tcr);
  printTime(local, tcr->abbrev);
  Serial.print(',');
  Serial.print(temperature);
  Serial.print(',');
  Serial.print(dht.toFahrenheit(temperature));
  Serial.print(',');
  Serial.print(humidity);
  Serial.println();
}


void printTime(time_t t, char *tz)
{
    sPrintI00(hour(t));
    sPrintDigits(minute(t));
    sPrintDigits(second(t));
    Serial.print(' ');
    Serial.print(dayShortStr(weekday(t)));
    Serial.print(' ');
    sPrintI00(day(t));
    Serial.print(' ');
    Serial.print(monthShortStr(month(t)));
    Serial.print(' ');
    Serial.print(year(t));
    Serial.print(' ');
    Serial.print(tz);
}

//Print an integer in "00" format (with leading zero).
//Input value assumed to be between 0 and 99.
void sPrintI00(int val)
{
    if (val < 10) Serial.print('0');
    Serial.print(val, DEC);
    return;
}

//Print an integer in ":00" format (with leading zero).
//Input value assumed to be between 0 and 99.
void sPrintDigits(int val)
{
    Serial.print(':');
    if(val < 10) Serial.print('0');
    Serial.print(val, DEC);
}


