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
  _volts = 0.0;
}

/****************************************************************************/
// readPacket()
/****************************************************************************/
int UT61E::readPacket(void) {
  unsigned long start = millis();
  for (;;) {
    if (_Serial->available() == 0) {
      if (millis() - start > 1500) {
        return UT61E_ERROR_TIMEOUT;
      }
    } else {
      // read the packet -- 3 retries
      for (byte i = 0; i < 4; i++) {
        while (_Serial->available()) {          // clear the input buffer                                 
          _Serial->read();
        }
                                                  // read the packet
        byte size = _Serial->readBytesUntil(10, (char *)&_packet, 14);
        if (size == 13) {
            _packet.lf = 10;
            this->massagePacket();
            return UT61E_SUCCESS;
        }
      }
      return UT61E_ERROR_READING_PACKET;
    }
  }
}

/****************************************************************************/
// massagePacket()
/****************************************************************************/
void UT61E::massagePacket(void) {
    // strip out the junk
    _packet.range                       &= B00000111;  // bits 7 through 3 are always 00110
    _packet.digit1                      &= B00001111;  // bits 7 through 4 are always  0011
    _packet.digit2                      &= B00001111;  // bits 7 through 4 are always  0011
    _packet.digit3                      &= B00001111;  // bits 7 through 4 are always  0011
    _packet.digit4                      &= B00001111;  // bits 7 through 4 are always  0011
    _packet.digit5                      &= B00001111;  // bits 7 through 4 are always  0011
    _packet.mode                        &= B00001111;  // bits 7 through 4 are always  0011
    _packet.info_flags                  &= B00001111;  // bits 7 through 4 are always  0011
    _packet.relative_mode_flags         &= B00001111;  // bits 7 through 4 are always  0011
    _packet.limit_flags                 &= B00001111;  // bits 7 through 4 are always  0011
    _packet.voltage_and_autorange_flags &= B00001111;  // bits 7 through 4 are always  0011
    _packet.hold                        &= B00001111;  // bits 7 through 4 are always  0011
}

/****************************************************************************/
// Resistance Measuring Methods
/****************************************************************************/
int UT61E::measureResistance(void) {
  int error = this->readPacket();
  if (error !=  UT61E_SUCCESS) {
    return error;
  }
  // check meter mode
  if (_packet.mode !=  3) {             // Mode 3 = Resistance Measurement
    return UT61E_ERROR_INVALID_MODE;
  }   
  // do the maths and set resistance member variable
  unsigned long ulResistance = (10000 * _packet.digit1) + (1000 * _packet.digit2)
                   + (100 * _packet.digit3) + (10 * _packet.digit4) + _packet.digit5;
  if (_packet.range == 0) {
    _resistance = ((float) ulResistance) / 100.0;
  } else if (_packet.range == 1) {
    _resistance = ((float) ulResistance) / 10.0;
  } else {
    _resistance = (float) (ulResistance * this->lpow(10, _packet.range - 2));
  }
  return UT61E_SUCCESS;
}

float UT61E::getResistance(void) {
  return _resistance;
}

/****************************************************************************/
// Voltage Measuring Methods
/****************************************************************************/
int UT61E::measureVoltage(byte type) {
  int error = this->readPacket();

  if (error !=  UT61E_SUCCESS) {
    return error;
  }

  if (_packet.mode !=  11) {             // Mode 11 = Voltage Measurement
    return UT61E_ERROR_INVALID_MODE;
  }

  if ((_packet.info_flags & B00000001) != 0) {
    return UT61E_ERROR_OVERLOAD;
  }

  if (type == UT61E_DC && (_packet.voltage_and_autorange_flags & B00001000) != 8) {
    return UT61E_ERROR_VOLTAGE_NOT_DC;
  } else if (type == UT61E_AC && (_packet.voltage_and_autorange_flags & B00000100) != 4) {
    return UT61E_ERROR_VOLTAGE_NOT_AC;
  }
  
  
  // do the maths and set volts member variable
  unsigned long ulVolts = (10000 * _packet.digit1) + (1000 * _packet.digit2)
            + (100 * _packet.digit3) + (10 * _packet.digit4) + _packet.digit5;
  if (_packet.range == 0) {
    _volts = ((float) ulVolts) / 10000.0;
  } else if (_packet.range == 1) {
    _volts = ((float) ulVolts) / 1000.0;
  } else if (_packet.range = 2) {
    _volts = ((float) ulVolts) / 100.0;
  } else if (_packet.range = 3) {
    _volts = ((float) ulVolts) / 10.0;
  } else {
    _volts = (float) (ulVolts * this->lpow(10, _packet.range - 4));
  }
  // read info flags to see if the measured voltage is a negative value
  if (_packet.info_flags & B00000100) {
    _volts = _volts * -1.0;
  }

  return UT61E_SUCCESS;
}

int UT61E::measureVoltageDC(void) {
  int error = this->measureVoltage(UT61E_DC);
  if (error != UT61E_SUCCESS) {
    return error;
  }
  return UT61E_SUCCESS;
}

int UT61E::measureVoltageAC(void) {
  int error = this->measureVoltage(UT61E_AC);
  if (error != UT61E_SUCCESS) {
    return error;
  }
  return UT61E_SUCCESS;
}

float UT61E::getVolts(void) {
  return _volts;
}

void UT61E::getVoltsStr(char *buf) {
  dtostrf(_volts, 10, 5, buf);
}

/****************************************************************************/
// Debugging / Sniffing
/****************************************************************************/
#if UT61E_DEBUG == 1
  void UT61E::printPacket(void) {
    Serial.print("|Range: ");
    Serial.print(_packet.range);
    Serial.print("|Digits: ");
    Serial.print(_packet.digit1);
    Serial.print(" ");
    Serial.print(_packet.digit2);
    Serial.print(" ");
    Serial.print(_packet.digit3);
    Serial.print(" ");
    Serial.print(_packet.digit4);
    Serial.print(" ");
    Serial.print(_packet.digit5);
    Serial.print("|Mode: ");
    Serial.print(_packet.mode);
    Serial.print("|Info: ");
    Serial.print(_packet.info_flags);
    Serial.print("|Rel Mode: ");
    Serial.print(_packet.relative_mode_flags);
    Serial.print("|Limit: ");
    Serial.print(_packet.limit_flags);
    Serial.print("|Vltg & AR Flags: ");
    Serial.print(_packet.voltage_and_autorange_flags);
    Serial.print("|Hold: ");
    Serial.print(_packet.hold);
    Serial.print("|EOP: ");
    Serial.print(_packet.cr);
    Serial.print(" ");
    Serial.print(_packet.lf);
    Serial.print(" ");
  }

  void UT61E::printErrorMessage(HardwareSerial* SerialObj, int error) {
    switch (error) {
      case UT61E_ERROR_TIMEOUT:
        SerialObj->println("TIMEOUT");
        return;
      case UT61E_ERROR_READING_PACKET:
        SerialObj->println("ERROR READING PACKET");
        return;
      case UT61E_ERROR_INVALID_MODE:
        SerialObj->println("INVALID METER MODE");
        return;      
      case UT61E_ERROR_VOLTAGE_NOT_DC:
        SerialObj->println("ERROR: VOLTAGE NOT DC");
        return;
      case UT61E_ERROR_VOLTAGE_NOT_AC:
        SerialObj->println("ERROR: VOLTAGE NOT AC");
        return;  
      case UT61E_ERROR_OVERLOAD:
        SerialObj->println("OVERLOAD");
        return;  
    }
  }
#endif  // if UT61E_DEBUG == 1

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