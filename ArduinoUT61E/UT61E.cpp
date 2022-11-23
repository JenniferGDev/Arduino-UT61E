#include "UT61E.h"

/****************************************************************************/
// Constructors
/****************************************************************************/
UT61E::UT61E(HardwareSerial* serialObj) {
  this->setup(serialObj);
}

UT61E::UT61E(HardwareSerial* serialObj, int dtrPin) {
  this->setup(serialObj);
  
  _dtrPin = dtrPin;                         // DTR needs to be high as it powers the adapter
  pinMode(_dtrPin, OUTPUT);                 // RTS needs to be grounded as well to power it
  digitalWrite(_dtrPin, HIGH);              // RX Line needs inversion (e.g. 74HC14N) before Arduino                               
}

void UT61E::setup(HardwareSerial* serialObj) {
  _Serial = serialObj;
  _Serial->begin(19200, SERIAL_7O1);        // seven bit word length, odd parity, one stop bit.
  _resistance = 0.0;
}

/****************************************************************************/
// readPacket()
/****************************************************************************/
bool UT61E::readPacket(void) {
  // read the packet -- 3 retries
  for (byte i = 0; i < 4; i++) {
    do {                                    // clear the input buffer                                 
      _Serial->read();
    } while (_Serial->available());
                                            // read the packet
    byte size = _Serial->readBytesUntil(10, (char *)&_packet, 14);
    if (size == 13) {
        return true;
    }
  }
  return false;
}

/****************************************************************************/
// massagePacket()
/****************************************************************************/
void UT61E::massagePacket(void) {
    // strip out the junk
    _packet.range  = _packet.range  & B00000111;  // bits 7 through 3 are always 00110
    _packet.digit1 = _packet.digit1 & B00001111;  // bits 7 through 4 are always  0011
    _packet.digit2 = _packet.digit2 & B00001111;  // bits 7 through 4 are always  0011
    _packet.digit3 = _packet.digit3 & B00001111;  // bits 7 through 4 are always  0011
    _packet.digit4 = _packet.digit4 & B00001111;  // bits 7 through 4 are always  0011
    _packet.digit5 = _packet.digit5 & B00001111;  // bits 7 through 4 are always  0011
    _packet.mode   = _packet.mode   & B00001111;  // bits 7 through 4 are always  0011
}

/****************************************************************************/
// measureResistance()
/****************************************************************************/
int UT61E::measureResistance(void) {
  unsigned long start = millis();
  for (;;) {
    if (_Serial->available() == 0) {
      if (millis() - start > 1500) {
        return UT61E_ERROR_TIMEOUT;
      }
    } else {
      // read the packet
      if (!this->readPacket()) {
        return UT61E_ERROR_READING_PACKET;
      }

      // strip out the junk
      this->massagePacket();

      // check meter mode
      if (_packet.mode !=  3) {             // Mode 3 = Resistance Measurement
        return UT61E_ERROR_INVALID_MODE;
      }

      // do the maths
      if (_packet.range == 0) {
        // floating point math
        _resistance = (100.0 * _packet.digit1) + (10.0 * _packet.digit2) + (1.0 * _packet.digit3)
                + (0.1 * _packet.digit4) + (0.01 * _packet.digit5);
      } else if (_packet.range == 1) { 
        // floating point math
        _resistance = (1000.0 * _packet.digit1) + (100.0 * _packet.digit2) + (10.0 * _packet.digit3)
                + (1.0 * _packet.digit4) + (0.1 * _packet.digit5);
      } else {
        // long integer math - convert to floating point at end (to preserve accuracy)
        long ulResistance = (10000 * _packet.digit1) + (1000 * _packet.digit2) + (100 * _packet.digit3)
                + (10 * _packet.digit4) + _packet.digit5;
                
        _resistance = (float) (ulResistance * this->lpow(10, _packet.range - 2));

      }
      return UT61E_SUCCESS;
    }
  }
}

/****************************************************************************/
// getResistance()
/****************************************************************************/
float UT61E::getResistance(void) {
  return _resistance;
}

/****************************************************************************/
// Utility methods
/****************************************************************************/
long UT61E::lpow(byte base, byte exponent) {
  long result = 1;

  for (byte i = 0; i < exponent; i++) {
      result = result * base;
  }
  return result;
}