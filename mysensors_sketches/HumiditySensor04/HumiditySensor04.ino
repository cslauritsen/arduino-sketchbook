 
#include <SPI.h>
#include <MySensor.h>  
#include <DHT.h>
#include <Adafruit_BMP085.h>

#define DEBUG

#define BATTERY_SENSE_PIN A0
#define MOTION_PIN 2   // The digital input you attached your motion sensor.  (Only 2 and 3 generates interrupt!)
#define INTERRUPT MOTION_PIN-2 // Usually the interrupt = pin -2 (on uno/nano anyway)

#define REPEATER_NODE false
#define PARENT_NODEID AUTO
#define NODEID 0x04

#define CHILD_ID_HUM 0
#define CHILD_ID_TEMP 1
#define CHILD_ID_BARO 2
#define CHILD_ID_TEMP2 3

#define HUMIDITY_SENSOR_DIGITAL_PIN 3
unsigned long SLEEP_TIME = 60L * 60L * 1000L; // Sleep time between reads (in milliseconds)


const float ALTITUDE = 320; // Shaker heights, in meters

MySensor gw;
DHT dht;
Adafruit_BMP085 bmp = Adafruit_BMP085();      // Digital Pressure Sensor 
float lastTemp;
float lastHum;
boolean metric = true; 

MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);

MyMessage msgBaro(CHILD_ID_BARO, V_PRESSURE);
MyMessage msgTemp2(CHILD_ID_TEMP2, V_TEMP);

bool bmpConnected = false;

void setup()  
{ 

   // use the 1.1 V internal reference
#if defined(__AVR_ATmega2560__)
   analogReference(INTERNAL1V1);
#else
   analogReference(INTERNAL);
#endif

  
  Serial.begin(57600);
  pinMode(4, OUTPUT);  // power
  pinMode(5, OUTPUT); // power

  pinMode(7, OUTPUT); // ground

  digitalWrite(4, HIGH);
  digitalWrite(5, HIGH);
  digitalWrite(7, LOW);
  
  gw.begin(NULL, NODEID, REPEATER_NODE, PARENT_NODEID);
  dht.setup(HUMIDITY_SENSOR_DIGITAL_PIN); 

  // Send the Sketch Version Information to the Gateway
  gw.sendSketchInfo("Humidity", "1.0");

  // Register all sensors to gw (they will be created as child devices)
  gw.present(CHILD_ID_HUM, S_HUM);
  gw.present(CHILD_ID_TEMP, S_TEMP);

  
  if (bmp.begin()) {
          bmpConnected = true;
  }

  if (bmpConnected) {
    gw.present(CHILD_ID_BARO, S_BARO);
    gw.present(CHILD_ID_TEMP2, S_TEMP);
  }
  
  metric = gw.getConfig().isMetric;
}

void loop()      
{  
  delay(dht.getMinimumSamplingPeriod());

  float temperature = dht.getTemperature();
  if (isnan(temperature)) {
      Serial.println("Failed reading temperature from DHT");
  } else if (temperature != lastTemp) {
    lastTemp = temperature;
    if (!metric) {
      temperature = dht.toFahrenheit(temperature);
    }
    gw.send(msgTemp.set(temperature, 1));
    Serial.print("T: ");
    Serial.println(temperature);
  }
  
  float humidity = dht.getHumidity();
  if (isnan(humidity)) {
      Serial.println("Failed reading humidity from DHT");
  } else if (humidity != lastHum) {
      lastHum = humidity;
      gw.send(msgHum.set(humidity, 1));
      Serial.print("H: ");
      Serial.println(humidity);
  }

  if (bmpConnected) {
    float pressure = bmp.readSealevelPressure(ALTITUDE) / 100.0;
    float temperature2 = bmp.readTemperature();
    if (!metric) {
      temperature2 = dht.toFahrenheit(temperature2);
    }
  
    gw.send(msgBaro.set(pressure, 1));
    gw.send(msgTemp2.set(temperature2, 1));
  }
  sendBatteryVoltage();
  gw.sleep(INTERRUPT,CHANGE, SLEEP_TIME);  
}

void sendBatteryVoltage() {
   static int oldBatteryPcnt;
   static int batterySensorValue;
   static float batteryV;
   static int batteryPcnt;
   static int invocations;
   
   batterySensorValue = analogRead(BATTERY_SENSE_PIN);
   #ifdef DEBUG
   Serial.println(batterySensorValue);
   #endif
   
   // Chad's setup:
   // Vmax = 3.3V
   // Resistor series:  100k + 56k
   //  V_a0 = 5.6e4Ω / 1.56e5Ω * Vmax
   //       =  0.31 * 3.3V
   //       =  1.1V = Arduino Analog Ref Voltage
   #define V_MAX 3.3
   #define ANALOG_RESOLUTION_BITS 1023
   #define VOLTS_PER_BIT (V_MAX/ANALOG_RESOLUTION_BITS)  // <=  V_MAX / ANALOG_RESOLUTION_BITS
   
   batteryV  = batterySensorValue * VOLTS_PER_BIT;
   
/*   
                        CONNECT A0 HERE
                              |
                              v
   
                          0.1666*6.6V
                  6.6V       = 1.1V             0.0833*1.1V          0*6.6V
         [6.6V] ----- 1M ------------- 100k ---------------- 100k ------+
                              |                                       |
                             ___   Noise Filter Cap                   |   
                             ___  C1 1uF                              |  
                              |                                       |
                            -----                                   -----
                             ---                                     ---
                              -                                       -
 */   
   
   
   batteryPcnt = 100 * batteryV / V_MAX;

   #ifdef DEBUG
   Serial.print("Battery Voltage: ");
   Serial.print(batteryV);
   Serial.println(" V");

   Serial.print("Battery percent: ");
   Serial.print(batteryPcnt);
   Serial.println(" %");
   #endif
   
   gw.sendBatteryLevel(batteryPcnt);
}


