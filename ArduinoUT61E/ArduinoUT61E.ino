#include "UT61E.h"
#define PIN_DTR 32
UT61E ut61e = UT61E(&Serial2, PIN_DTR);

void setup() {
  Serial.begin(115200);                   // coms from arduino to mac
}

void loop() {
  int error = ut61e.measureResistance();
  if (error != UT61E_SUCCESS) {
    switch (error) {
      case UT61E_ERROR_TIMEOUT:
        Serial.println("TIMEOUT");
        return;
      case UT61E_ERROR_READING_PACKET:
        Serial.println("ERROR READING PACKET");
        return;
      case UT61E_ERROR_INVALID_MODE:
        Serial.println("INVALID METER MODE");
        return;
    }
  }
  float ohms = ut61e.getResistance();
  Serial.print("Ohms: " );
  if (ohms > 220000000) {
    Serial.println("OL.");
  } else {
    Serial.println(ohms);
  }
}
