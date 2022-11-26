
// Example code for reading resistance measurement from meter
// Using the Arduino Mini Mega 2560, which has four serial ports.
// Pins 16 & 17 on the Mega are Serial2.

#include "UT61E.h"
#define PIN_DTR 32
UT61E ut61e = UT61E(&Serial2, PIN_DTR);

void setup() {
  Serial.begin(115200);                   // coms from arduino to mac
}

void loop() {
  int error = ut61e.measureResistance();
  if (error != UT61E_SUCCESS) {
    #if UT61E_DEBUG == 1
      ut61e.printErrorMessage(&Serial, error);
      return;
    #endif
  } else {
    #if UT61E_DEBUG == 1
      ut61e.printPacket();
    #endif
    float ohms = ut61e.getResistance();
    Serial.print("Ohms: " );
    if (ohms > 220000000) {
      Serial.println("OL.");
    } else {
      Serial.println(ohms);
    }
  }
}
