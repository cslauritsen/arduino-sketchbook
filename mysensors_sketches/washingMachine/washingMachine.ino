#include <MsTimer2.h>

#define ABS(x) ((x) < 0 ? -(x) : (x))

// NODE CONFIGUATION *******************************************
#define SKETCH_NAME "Washing Machine"
#define SKETCH_MAJOR_VER "3"
#define SKETCH_MINOR_VER "14"

#define NODEID 0x03
#define PARENT_NODEID AUTO
#define REPEATER_NODE true

#define WET_PIN 2
#define SOUND_PIN 3 // Arduino Digital I/O pin for button/reed switch
#define SOUND_CHILD_ID 7
#define WET_CHILD_ID 8

#define STATUS_LED 8
#define SOUND_LED 7

// ****************************************************************
#define CSL_DEBUG 1

#include <MySensor.h>
#include <SPI.h>

#if (SOUND_PIN < 2 || SOUND_PIN > 3)
#error SOUND_PIN must be either 2 or 3 for interrupts to work
#endif
#if (WET_PIN < 2 || WET_PIN > 3)
#error WET_PIN must be either 2 or 3 for interrupts to work
#endif

MySensor sensor_node;

// Change to V_LIGHT if you use S_LIGHT in presentation below
MyMessage msgSound(SOUND_CHILD_ID, V_TRIPPED);
MyMessage msgWet(WET_CHILD_ID, V_TRIPPED);


boolean soundPresent = false;
boolean wetPresent = false;

void flash() {
  static boolean output = HIGH;

  digitalWrite(STATUS_LED, output);
  output = !output;
}

long soundReset = 0L;
void onSound() {
  soundPresent = true;  
}
void onWet() {
  wetPresent = true;
}

void setup()  
{
  pinMode(SOUND_PIN, INPUT); 
  pinMode(WET_PIN, INPUT);
 pinMode(STATUS_LED, OUTPUT);
 pinMode(SOUND_LED, OUTPUT);
 
  // Activate internal pull-ups
  digitalWrite(SOUND_PIN, HIGH);
  digitalWrite(WET_PIN, HIGH);  
  
 digitalWrite(SOUND_LED, LOW);
 
 MsTimer2::set(300, flash);

  MsTimer2::start();
  sensor_node.begin(NULL, NODEID, REPEATER_NODE, PARENT_NODEID);
  
  // Send the sketch version information to the gateway and Controller
  sensor_node.sendSketchInfo(SKETCH_NAME, SKETCH_MAJOR_VER "." SKETCH_MINOR_VER);

  // Register binary input sensor to sensor_node (they will be created as child devices)
  // You can use S_DOOR, S_MOTION or S_LIGHT here depending on your usage. 
  // If S_LIGHT is used, remember to update variable type you send in. See "msg" above.
  sensor_node.present(SOUND_CHILD_ID, S_MOTION, "Washer Sound", true);  
  sensor_node.present(WET_CHILD_ID, S_MOTION, "Washer Leak", true);
  
  attachInterrupt(digitalPinToInterrupt(SOUND_PIN), onSound, FALLING); 
  attachInterrupt(digitalPinToInterrupt(WET_PIN), onWet, FALLING); 
  MsTimer2::stop();
}

boolean lastWet = wetPresent;
boolean lastSound = soundPresent;
long elapsed = 0;

long soundResetMillis = 0;
long wetResetMillis = 0;
void loop() {
  
  elapsed = millis();
  sensor_node.process();
  
  digitalWrite(SOUND_LED, soundPresent);
  digitalWrite(STATUS_LED, wetPresent);
  
  if (soundPresent && ABS(elapsed - soundResetMillis) > 15000) {
     soundPresent = digitalRead(SOUND_PIN) == LOW;
     soundResetMillis = elapsed;
  }  
  
  if (wetPresent && ABS(elapsed - wetResetMillis) > 5000) {
     wetPresent = digitalRead(WET_PIN) == LOW;
     wetResetMillis = elapsed;
  }  
  
  if (lastWet != wetPresent) {
    sensor_node.send(msgWet.set(wetPresent));
  }
  if (lastSound != soundPresent) {
    sensor_node.send(msgSound.set(soundPresent));
  }  
  
  lastSound = soundPresent;
  lastWet = wetPresent;
}
