/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * REVISION HISTORY
 * Version 1.0 - Henrik Ekblad
 * 
 * DESCRIPTION
 * Motion Sensor example using HC-SR501 
 * http://www.mysensors.org/build/motion
 *
 */

 #define DEBUG 

#include <MySensor.h>  
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>

#define BARO_CHILD 0
#define TEMP_CHILD 1
#define MOTION_CHILD 2
#define BATTERY_SENSE_PIN A0

unsigned long SLEEP_TIME = 1000L * 60L * 60L; // Sleep time between reports (in milliseconds)
#define MOTION_PIN 2   // The digital input you attached your motion sensor.  (Only 2 and 3 generates interrupt!)
#define INTERRUPT MOTION_PIN-2 // Usually the interrupt = pin -2 (on uno/nano anyway)


const float ALTITUDE = 320; // Shaker heights, in meters


const char *weather[] = { "stable", "sunny", "cloudy", "unstable", "thunderstorm", "unknown" };
enum FORECAST
{
	STABLE = 0,			// "Stable Weather Pattern"
	SUNNY = 1,			// "Slowly rising Good Weather", "Clear/Sunny "
	CLOUDY = 2,			// "Slowly falling L-Pressure ", "Cloudy/Rain "
	UNSTABLE = 3,		// "Quickly rising H-Press",     "Not Stable"
	THUNDERSTORM = 4,	// "Quickly falling L-Press",    "Thunderstorm"
	UNKNOWN = 5			// "Unknown (More Time needed)
};

Adafruit_BMP085 bmp = Adafruit_BMP085();      // Digital Pressure Sensor 

MySensor gw;

float lastPressure = -1;
float lastTemp = -1;
int lastForecast = -1;

const int LAST_SAMPLES_COUNT = 5;
float lastPressureSamples[LAST_SAMPLES_COUNT];

// this CONVERSION_FACTOR is used to convert from Pa to kPa in forecast algorithm
// get kPa/h be dividing hPa by 10 
#define CONVERSION_FACTOR (1.0/10.0)

int minuteCount = 0;
bool firstRound = true;
// average value is used in forecast algorithm.
float pressureAvg;
// average after 2 hours is used as reference value for the next iteration.
float pressureAvg2;

float dP_dt;
boolean metric;
MyMessage tempMsg(TEMP_CHILD, V_TEMP);
MyMessage pressureMsg(BARO_CHILD, V_PRESSURE);
MyMessage forecastMsg(BARO_CHILD, V_FORECAST);
MyMessage motionMsg(MOTION_CHILD, V_TRIPPED);

bool bmpConnected = false;
#define REPEATER_NODE false
#define PARENT_NODEID AUTO
#define NODEID 0x21


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
   // Vmax = 6.6V
   // Resistor series:  1M + 100k + 100k
   //  V_a0 = 2e5Ω / 1.2e6Ω * Vmax
   //       =  0.166666 * 6.6V
   //       =  1.1V = Arduino Analog Ref Voltage
   #define V_MAX 6.6
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

   if (invocations++ % 5 == 0 || oldBatteryPcnt != batteryPcnt) {
     gw.sendBatteryLevel(batteryPcnt);
     oldBatteryPcnt = batteryPcnt;
   }
}

void setup()  
{  
  analogReference(INTERNAL);
  Serial.begin(19200);
  gw.begin(NULL, NODEID, REPEATER_NODE, PARENT_NODEID);
  pinMode(8, OUTPUT);
  pinMode(7, OUTPUT);
  digitalWrite(8, HIGH);
  digitalWrite(7, LOW);
  pinMode(6, OUTPUT);
  pinMode(5, OUTPUT);
  digitalWrite(6, HIGH);
  digitalWrite(5, LOW);
  
  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Chad Closet", "1.0");

  pinMode(MOTION_PIN, INPUT);      // sets the motion sensor digital pin as input
  // Register all sensors to gw (they will be created as child devices)
  gw.present(MOTION_CHILD, S_MOTION);
  
  
	if (bmp.begin()) {
          bmpConnected = true;
	}

      if (bmpConnected) {
	// Register sensors to gw (they will be created as child devices)
	gw.present(BARO_CHILD, S_BARO);
	gw.present(TEMP_CHILD, S_TEMP);
      }
	metric = gw.getConfig().isMetric;
  
  sendBatteryVoltage();
  
}

void loop()     
{     
  if (bmpConnected) {
	float pressure = bmp.readSealevelPressure(ALTITUDE) / 100.0;
	float temperature = bmp.readTemperature();

	if (!metric) 
	{
		// Convert to fahrenheit
		temperature = temperature * 9.0 / 5.0 + 32.0;
	}

	int forecast = sample(pressure);

	Serial.print("Temperature = ");
	Serial.print(temperature);
	Serial.println(metric ? " *C" : " *F");
	Serial.print("Pressure = ");
	Serial.print(pressure);
	Serial.println(" hPa");
	Serial.print("Forecast = ");
	Serial.println(weather[forecast]);


	if (temperature != lastTemp) 
	{
		gw.send(tempMsg.set(temperature, 1));
		lastTemp = temperature;
	}

	if (pressure != lastPressure) 
	{
		gw.send(pressureMsg.set(pressure, 0));
		lastPressure = pressure;
	}

	if (forecast != lastForecast)
	{
		gw.send(forecastMsg.set(weather[forecast]));
		lastForecast = forecast;
	}
  }
  
  
  // Read digital motion value
  boolean tripped = digitalRead(MOTION_PIN) == HIGH; 
        
  Serial.print("Motion: ");
  Serial.println(tripped);
  gw.send(motionMsg.set(tripped?"1":"0"));  // Send tripped value to gw 
 
  // Sleep until interrupt comes in on motion sensor. Send update every SLEEP_TIME. 
  gw.sleep(INTERRUPT,CHANGE, SLEEP_TIME);  
  sendBatteryVoltage();
}



float getLastPressureSamplesAverage()
{
	float lastPressureSamplesAverage = 0;
	for (int i = 0; i < LAST_SAMPLES_COUNT; i++)
	{
		lastPressureSamplesAverage += lastPressureSamples[i];
	}
	lastPressureSamplesAverage /= LAST_SAMPLES_COUNT;

	return lastPressureSamplesAverage;
}



// Algorithm found here
// http://www.freescale.com/files/sensors/doc/app_note/AN3914.pdf
// Pressure in hPa -->  forecast done by calculating kPa/h
int sample(float pressure)
{
	// Calculate the average of the last n minutes.
	int index = minuteCount % LAST_SAMPLES_COUNT;
	lastPressureSamples[index] = pressure;

	minuteCount++;
	if (minuteCount > 185)
	{
		minuteCount = 6;
	}

	if (minuteCount == 5)
	{
		pressureAvg = getLastPressureSamplesAverage();
	}
	else if (minuteCount == 35)
	{
		float lastPressureAvg = getLastPressureSamplesAverage();
		float change = (lastPressureAvg - pressureAvg) * CONVERSION_FACTOR;
		if (firstRound) // first time initial 3 hour
		{
			dP_dt = change * 2; // note this is for t = 0.5hour
		}
		else
		{
			dP_dt = change / 1.5; // divide by 1.5 as this is the difference in time from 0 value.
		}
	}
	else if (minuteCount == 65)
	{
		float lastPressureAvg = getLastPressureSamplesAverage();
		float change = (lastPressureAvg - pressureAvg) * CONVERSION_FACTOR;
		if (firstRound) //first time initial 3 hour
		{
			dP_dt = change; //note this is for t = 1 hour
		}
		else
		{
			dP_dt = change / 2; //divide by 2 as this is the difference in time from 0 value
		}
	}
	else if (minuteCount == 95)
	{
		float lastPressureAvg = getLastPressureSamplesAverage();
		float change = (lastPressureAvg - pressureAvg) * CONVERSION_FACTOR;
		if (firstRound) // first time initial 3 hour
		{
			dP_dt = change / 1.5; // note this is for t = 1.5 hour
		}
		else
		{
			dP_dt = change / 2.5; // divide by 2.5 as this is the difference in time from 0 value
		}
	}
	else if (minuteCount == 125)
	{
		float lastPressureAvg = getLastPressureSamplesAverage();
		pressureAvg2 = lastPressureAvg; // store for later use.
		float change = (lastPressureAvg - pressureAvg) * CONVERSION_FACTOR;
		if (firstRound) // first time initial 3 hour
		{
			dP_dt = change / 2; // note this is for t = 2 hour
		}
		else
		{
			dP_dt = change / 3; // divide by 3 as this is the difference in time from 0 value
		}
	}
	else if (minuteCount == 155)
	{
		float lastPressureAvg = getLastPressureSamplesAverage();
		float change = (lastPressureAvg - pressureAvg) * CONVERSION_FACTOR;
		if (firstRound) // first time initial 3 hour
		{
			dP_dt = change / 2.5; // note this is for t = 2.5 hour
		}
		else
		{
			dP_dt = change / 3.5; // divide by 3.5 as this is the difference in time from 0 value
		}
	}
	else if (minuteCount == 185)
	{
		float lastPressureAvg = getLastPressureSamplesAverage();
		float change = (lastPressureAvg - pressureAvg) * CONVERSION_FACTOR;
		if (firstRound) // first time initial 3 hour
		{
			dP_dt = change / 3; // note this is for t = 3 hour
		}
		else
		{
			dP_dt = change / 4; // divide by 4 as this is the difference in time from 0 value
		}
		pressureAvg = pressureAvg2; // Equating the pressure at 0 to the pressure at 2 hour after 3 hours have past.
		firstRound = false; // flag to let you know that this is on the past 3 hour mark. Initialized to 0 outside main loop.
	}

	int forecast = UNKNOWN;
	if (minuteCount < 35 && firstRound) //if time is less than 35 min on the first 3 hour interval.
	{
		forecast = UNKNOWN;
	}
	else if (dP_dt < (-0.25))
	{
		forecast = THUNDERSTORM;
	}
	else if (dP_dt > 0.25)
	{
		forecast = UNSTABLE;
	}
	else if ((dP_dt > (-0.25)) && (dP_dt < (-0.05)))
	{
		forecast = CLOUDY;
	}
	else if ((dP_dt > 0.05) && (dP_dt < 0.25))
	{
		forecast = SUNNY;
	}
	else if ((dP_dt >(-0.05)) && (dP_dt < 0.05))
	{
		forecast = STABLE;
	}
	else
	{
		forecast = UNKNOWN;
	}

	// uncomment when debugging
	//Serial.print(F("Forecast at minute "));
	//Serial.print(minuteCount);
	//Serial.print(F(" dP/dt = "));
	//Serial.print(dP_dt);
	//Serial.print(F("kPa/h --> "));
	//Serial.println(weather[forecast]);

	return forecast;
}



