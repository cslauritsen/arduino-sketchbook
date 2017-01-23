#define MY_DEBUG
#include <MySensor.h>  
#include <SPI.h>

#include "EmonLib.h"                   // Include Emon Library
EnergyMonitor emon1;                   // Create an instance
EnergyMonitor emon2;                   // Create an instance

#define LINE1 1
#define LINE2 2

MySensor gw;
float Vrms = 120.0;

MyMessage irms1Msg(LINE1, V_CURRENT); // current in mA (integer)
MyMessage irms2Msg(LINE2, V_CURRENT); // current in mA (integer)

#define NODEID AUTO // 0x4
#define PARENT_NODEID AUTO
#define REPEATER_NODE false
#define MY_RADIO_NRF24

#define MAX_READING 250.0  // sometimes it gets a spike in the reading. don't send. it can really throw things off

void setup()  
{  
  gw.begin(NULL, NODEID, REPEATER_NODE, PARENT_NODEID);
  
  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Whole House Power", "1.0");
  // Register all sensors to gw (they will be created as child devices)
  gw.present(LINE1, S_MULTIMETER);  
  gw.present(LINE2, S_MULTIMETER);  
  
  #ifndef MY_DEBUG
  Serial.begin(115200);
  #endif
  
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  emon1.current(A1, 55.5);             // Current: input pin, calibration.
  emon2.current(A2, 55.5);             // Current: input pin, calibration.
  analogReference(DEFAULT);            // 5V
}

float Irms = 0.0;
float power = 0.0;
int i=0;
void loop()     
{ 
  
  Serial.print("mA[1]: ");
  Irms = emon1.calcIrms(1480);  // Calculate Irms only  
  Serial.print(Irms);
  Serial.print(" power: ");
  power = Irms * Vrms;
  Serial.print(power);	       // Apparent power
  Serial.println(" ");  
  if (i % 10 == 0 && Irms > -MAX_READING && Irms < MAX_READING) {
    gw.send(irms1Msg.set(Irms, 3));
    Serial.print(">>SENT ");
    Serial.println(Irms, 3);
  }
  
  gw.sleep(100); // Sleep to prevent analog jitter, per https://www.arduino.cc/en/Tutorial/AnalogInputPins
  
  Serial.print("mA[2]: ");
  Irms = emon2.calcIrms(1480);  // Calculate Irms only  
  Serial.print(Irms);
  Serial.print(" power: ");
  power = Irms * Vrms;
  Serial.print(power);	       // Apparent power
  Serial.println(" ");  
  
  if (i % 10 == 0 && Irms > -MAX_READING && Irms < MAX_READING) {
    gw.send(irms2Msg.set(Irms, 3));
    Serial.print(">>SENT ");
    Serial.println(Irms, 3);
  }
  gw.sleep(100);
  
  i++;
}
