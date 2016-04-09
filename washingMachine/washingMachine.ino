#include <gitcommit.h>

// NODE CONFIGUATION *******************************************
#define SKETCH_NAME "Washing Machine"
#define SKETCH_MAJOR_VER "1"
#define SKETCH_MINOR_VER GIT_COMMIT_ID

#define NODEID 0x03
#define PARENT_NODEID AUTO
#define REPEATER_NODE true

#define WET_PIN 2
#define SOUND_PIN 3 // Arduino Digital I/O pin for button/reed switch
#define SOUND_CHILD_ID 7
#define WET_CHILD_ID 8

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

void onSound() {
  soundPresent = digitalRead(SOUND_PIN) == LOW;
}
void onWet() {
  wetPresent = digitalRead(WET_PIN) == LOW;
}

void setup()  
{
 

  sensor_node.begin(NULL, NODEID, REPEATER_NODE, PARENT_NODEID);

  pinMode(SOUND_PIN, INPUT); 
  pinMode(WET_PIN, INPUT);
  // Activate internal pull-ups
  digitalWrite(SOUND_PIN, HIGH);
  digitalWrite(WET_PIN, HIGH);
  
  // Send the sketch version information to the gateway and Controller
  sensor_node.sendSketchInfo(SKETCH_NAME, SKETCH_MAJOR_VER "." SKETCH_MINOR_VER);

  // Register binary input sensor to sensor_node (they will be created as child devices)
  // You can use S_DOOR, S_MOTION or S_LIGHT here depending on your usage. 
  // If S_LIGHT is used, remember to update variable type you send in. See "msg" above.
  sensor_node.present(SOUND_CHILD_ID, S_SOUND, "Washer Sound", true);  
  sensor_node.present(WET_CHILD_ID, S_WATER_LEAK, "Washer Leak", true);
  
  attachInterrupt(digitalPinToInterrupt(SOUND_PIN), onSound, CHANGE); 
  attachInterrupt(digitalPinToInterrupt(WET_PIN), onWet, CHANGE); 
}

boolean lastWet = wetPresent;
boolean lastSound = soundPresent;
long elapsed = 0;
void loop() {
  sensor_node.process();
  if (lastWet != wetPresent) {
    sensor_node.send(msgWet.set(wetPresent));
    lastWet = wetPresent;
  }
  if (lastSound != soundPresent) {
    sensor_node.send(msgSound.set(soundPresent));
    lastSound = soundPresent;
  }
  
  if (elapsed > 10L * 60 * 1000) {
    elapsed = 0;
    sensor_node.send(msgSound.set(soundPresent));
    sensor_node.send(msgWet.set(wetPresent));
  }
  elapsed = millis();
}
