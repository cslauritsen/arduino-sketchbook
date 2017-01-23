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
 * DESCRIPTION
 *
 * Interrupt driven binary switch example with dual interrupts
 * Author: Patrick 'Anticimex' Fallberg
 * Connect one button or door/window reed switch between 
 * digitial I/O pin 3 (BUTTON_PIN below) and GND and the other
 * one in similar fashion on digital I/O pin 2.
 * This example is designed to fit Arduino Nano/Pro Mini
 * 
 */
 
// NODE CONFIGUATION *******************************************
#define SKETCH_NAME "Backdoor Sensor"
#define SKETCH_MAJOR_VER "1"
#define SKETCH_MINOR_VER "1"

#define NODEID 0x10
#define PARENT_NODEID AUTO
#define REPEATER_NODE false

#define MOTION_PIN 2   // Arduino Digital I/O pin for button/reed switch
#define DOOR_PIN 3 // Arduino Digital I/O pin for button/reed switch
#define MOTION_CHILD_ID 3
#define DOOR_CHILD_ID 4

#define BATTERY_SENSE_PIN  A1  // select the input pin for the battery sense point
// ****************************************************************

#include <MySensor.h>
#include <SPI.h>

#if (MOTION_PIN < 2 || MOTION_PIN > 3)
#error MOTION_PIN must be either 2 or 3 for interrupts to work
#endif



#if (DOOR_PIN < 2 || DOOR_PIN > 3)
#error DOOR_PIN must be either 2 or 3 for interrupts to work
#endif
#if (MOTION_PIN == DOOR_PIN)
#error MOTION_PIN and BUTTON_PIN2 cannot be the same
#endif
#if (MOTION_CHILD_ID == DOOR_CHILD_ID)
#error MOTION_CHILD_ID and DOOR_CHILD_ID cannot be the same
#endif

MySensor sensor_node;

// Change to V_LIGHT if you use S_LIGHT in presentation below
MyMessage msgMotion(MOTION_CHILD_ID, V_TRIPPED);
MyMessage msgDoor(DOOR_CHILD_ID, V_TRIPPED);

void setup()  
{
  
   // use the 1.1 V internal reference
#if defined(__AVR_ATmega2560__)
   analogReference(INTERNAL1V1);
#else
   analogReference(INTERNAL);
#endif

  sensor_node.begin(NULL, NODEID, REPEATER_NODE, PARENT_NODEID);

  // Setup the buttons
  pinMode(MOTION_PIN, INPUT);
  pinMode(DOOR_PIN, INPUT);

  // Activate internal pull-ups
  digitalWrite(MOTION_PIN, HIGH);
  digitalWrite(DOOR_PIN, HIGH);
  
  // Send the sketch version information to the gateway and Controller
  sensor_node.sendSketchInfo(SKETCH_NAME, SKETCH_MAJOR_VER "." SKETCH_MINOR_VER);

  // Register binary input sensor to sensor_node (they will be created as child devices)
  // You can use S_DOOR, S_MOTION or S_LIGHT here depending on your usage. 
  // If S_LIGHT is used, remember to update variable type you send in. See "msg" above.
  sensor_node.present(MOTION_CHILD_ID, S_MOTION);  
  sensor_node.present(DOOR_CHILD_ID, S_DOOR);  
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
     sensor_node.sendBatteryLevel(batteryPcnt);
     oldBatteryPcnt = batteryPcnt;
   }
}

// Loop will iterate on changes on the BUTTON_PINs
void loop() {
  
  static uint8_t value;
  static uint8_t sentValue=2;
  static uint8_t sentValue2=2;

  // Short delay to allow buttons to properly settle
  sensor_node.sleep(5);
  
  value = digitalRead(MOTION_PIN);
  if (value != sentValue) {
     // Value has changed from last transmission, send the updated value
     sensor_node.send(msgMotion.set(value==HIGH ? 1 : 0));
     sentValue = value;
  }

  value = digitalRead(DOOR_PIN);  
  if (value != sentValue2) {
     // Value has changed from last transmission, send the updated value
     sensor_node.send(msgDoor.set(value==HIGH ? 1 : 0));
     sentValue2 = value;
  }
  
  sendBatteryVoltage();
  
  // Sleep until something happens with the sensor
  sensor_node.sleep(MOTION_PIN-2, CHANGE, DOOR_PIN-2, CHANGE, 0);
} 
