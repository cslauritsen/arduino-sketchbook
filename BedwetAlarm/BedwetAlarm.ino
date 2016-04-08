#include <avr/sleep.h>
#include "pitches.h"

//                 &     4     &   1  &   2  &   3      &     4     &  1   &  2  &  3  &     4     &
//                 8     8     4      r8  r4    r8    as8   as8    f4     r8  r4    r4       eb4
int melody[] = {PAS7, PAS7, PDS8    , 0,  0,     0,  PAS7, PAS7,  PF8    , 0, 0   , 0   , PDS8      ,

//  1    &     2  &     3       &  4      &  1  &  2  &  3 ...
// c8   c8    c4       c8     eb4        f4    r8 r4    r8 ...
  PC8, PC8,  PC8   ,  PC8  , PD8    , PDS8    , 0, 0,    0
};

//             
int rhythm[] = {8, 8, 4   , 8, 4               , 8,   8,     8,     4   , 8,   4   , 4,        4,
     8,  8,    4,      8,      4,        4,     8, 4,     8 // ...
};

int moisturePin = 2;             // pin used for waking up
int alarmPin = 9;                // PWM pin to drive a piezo
int sleepStatus = 0;             // variable to store a request for sleep
int count = 0;                   // counter
bool doAlarm = false;

enum Defcon {
  Defcon0   // all clear
  , Defcon1 // play alarm less frequently
  , Defcon2 // play alarm loud/alot/etc
};

volatile Defcon defcon = Defcon0;
int defcon2Alerts = 0;

void onWake() {
    // execute code here after wake-up before returning to the loop() function
    // timers and code using timers (serial.print and more...) will not work here.

    defcon = Defcon2;
}

void playMelody() {  
       // iterate over the notes of the melody:
    for (int thisNote = 0; thisNote < sizeof(melody)/sizeof(melody[0]); thisNote++) {
  
      // to calculate the note duration, take one second
      // divided by the note type.
      //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
      int noteDuration = 1000 / rhythm[thisNote];
      tone(alarmPin, melody[thisNote], noteDuration);
  
      // to distinguish the notes, set a minimum time between them.
      int pauseBetweenNotes = noteDuration * 1.05;
      delay(pauseBetweenNotes);
      // stop the tone playing:
      noTone(alarmPin);
    }
}

void setup() {
  
    defcon = Defcon0;
    pinMode(moisturePin, INPUT_PULLUP);
    pinMode(13, OUTPUT);
    
    Serial.begin(19200); // The pro mini 3.3V worked when I used 19200 here and 9600 in the monitor
    Serial.println("begin setup...");

    attachInterrupt(0, onWake, LOW);
    
    Serial.println("end setup...");
}

void loop() {
    digitalWrite(13, HIGH);
    // display information about the counter
    Serial.println("loop() enter");
    // compute the serial input
    if (Serial.available()) {
        int val = Serial.read();
        if (val == 'S') {
            Serial.println("Serial: Entering Sleep mode");
            delay(100);     // this delay is needed, the sleep
            //function will provoke a Serial error otherwise!!
            count = 0;
            alarmStop();
            sleepNow();     // sleep function called here
        }
        if (val == 'A') {
            Serial.println("Hola Caracola"); // classic dummy message
        }
    }
    
    switch(defcon) {
      case Defcon2:
        for(int i=0; i < 100; i++) {
          playMelody();
        }
        defcon = Defcon1; // downgrade, everyone's asleep probably
        break;
        // Defcon1 not currently supported. Too lazy.
//      case Defcon1:
//        playMelody();
//        delay(60 * 60 * 1000); // wait 1 minute
//        break;
      default:
        sleepNow();
    }
}


void alarmStop() {
  defcon = Defcon0;
  analogWrite(alarmPin, 0);
}



void sleepNow() {
    /* Now is the time to set the sleep mode. In the Atmega8 datasheet
     * http://www.atmel.com/dyn/resources/prod_documents/doc2486.pdf on page 35
     * there is a list of sleep modes which explains which clocks and
     * wake up sources are available in which sleep mode.
     *
     * In the avr/sleep.h file, the call names of these sleep modes are to be found:
     *
     * The 5 different modes are:
     *     SLEEP_MODE_IDLE         -the least power savings
     *     SLEEP_MODE_ADC
     *     SLEEP_MODE_PWR_SAVE
     *     SLEEP_MODE_STANDBY
     *     SLEEP_MODE_PWR_DOWN     -the most power savings
     *
     * For now, we want as much power savings as possible, so we
     * choose the according
     * sleep mode: SLEEP_MODE_PWR_DOWN
     *
     */  
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);   // sleep mode is set here

    sleep_enable();          // enables the sleep bit in the mcucr register
    // so sleep is possible. just a safety pin

    /* Now it is time to enable an interrupt. We do it here so an
     * accidentally pushed interrupt button doesn't interrupt
     * our running program. if you want to be able to run
     * interrupt code besides the sleep function, place it in
     * setup() for example.
     *
     * In the function call attachInterrupt(A, B, C)
     * A   can be either 0 or 1 for interrupts on pin 2 or 3.  
     *
     * B   Name of a function you want to execute at interrupt for A.
     *
     * C   Trigger mode of the interrupt pin. can be:
     *             LOW        a low level triggers
     *             CHANGE     a change in level triggers
     *             RISING     a rising edge of a level triggers
     *             FALLING    a falling edge of a level triggers
     *
     * In all but the IDLE sleep modes only LOW can be used.
     */

    digitalWrite(13, LOW);
    attachInterrupt(digitalPinToInterrupt(moisturePin), onWake, LOW); 
    sleep_mode();

    // ***********************************************************
    // SKETCH CONTINUES FROM HERE AFTER WAKING UP
    // ***********************************************************

    sleep_disable();         // first thing after waking from sleep:
    // disable sleep...
    detachInterrupt(0);      // disables interrupt 0 on pin 2 so the
    // onWake code will not be executed
    // during normal running time.
}
