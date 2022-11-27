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
  _amps = 0.0;
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
// Read Packet and Check Mode/Type
/****************************************************************************/
int UT61E::readPacketCheckMode(byte mode) {
  int error = this->readPacket();
  if (error !=  UT61E_SUCCESS) { return error; }
  if (_packet.mode != mode) {
    return UT61E_ERROR_INVALID_MODE;
  }
  return UT61E_SUCCESS;
}

int UT61E::readPacketCheckModeType(byte mode, byte type) {
  int error = this->readPacketCheckMode(mode);
  if (error !=  UT61E_SUCCESS) { return error; }
  if ((_packet.info_flags & B00000001) != 0) {
    return UT61E_ERROR_OVERLOAD;
  }
  if (type == UT61E_DC && (_packet.voltage_and_autorange_flags & B00001000) != 8) {
    return UT61E_ERROR_VOLTAGE_NOT_DC;
  } else if (type == UT61E_AC && (_packet.voltage_and_autorange_flags & B00000100) != 4) {
    return UT61E_ERROR_VOLTAGE_NOT_AC;
  }
  return UT61E_SUCCESS;
}

/****************************************************************************/
// Calculate combined digits based on magnitude parameter
/****************************************************************************/
float UT61E::calculate(byte magnitude) {
  unsigned long ulCombinedDigits = (10000 * _packet.digit1) + (1000 * _packet.digit2)
                   + (100 * _packet.digit3) + (10 * _packet.digit4) + _packet.digit5;
  float result;
  if (_packet.range < magnitude) {
    result = ((float) ulCombinedDigits) * pow(10, _packet.range - magnitude);
  } else {
    // lpow is built in utility method to preerve accuracy of whole number readings
    result = (float) (ulCombinedDigits * this->lpow(10, _packet.range - magnitude));
  }
  
  if (_packet.info_flags & B00000100) {         // check for negative value & set
    result = result * -1.0;
  }
  return result;
}

/****************************************************************************/
// Resistance Measuring Methods
/****************************************************************************/
int UT61E::measureResistance(void) {
  int error = this->readPacketCheckMode(3);               // 3 = Resist. Mode
  if (error !=  UT61E_SUCCESS) { return error; }
  _resistance = this->calculate(2);
  return UT61E_SUCCESS;
}

float UT61E::getResistance(void) {
  return _resistance;
}

/****************************************************************************/
// Voltage Measuring Methods
/****************************************************************************/
int UT61E::measureVolts(byte type) {
  int error = this->readPacketCheckModeType(11, type);    // 11 = Volts Mode
  if (error !=  UT61E_SUCCESS) { return error; }
  if ((_packet.voltage_and_autorange_flags & B00000010) != 2) {
    // meter not set to autorange
    return UT61E_ERROR_INVALID_MODE;
  }
  _volts = this->calculate(4);  
  return UT61E_SUCCESS;
}
int UT61E::measureMillivolts(byte type) {
  int error = this->readPacketCheckModeType(11, type);    // 11 = Volts Mode
  if (error !=  UT61E_SUCCESS) { return error; }
  if ((_packet.voltage_and_autorange_flags & B00000010) == 2) {
    // meter not set to manual
    return UT61E_ERROR_INVALID_MODE;
  }
  _volts = this->calculate(9);
  return UT61E_SUCCESS;
}

float UT61E::getVolts(void) {
  return _volts;
}

void UT61E::getVoltsStr(char *fifteenByteBuf) {
  dtostrf(_volts, -15, 5, fifteenByteBuf);
  this->ttrim(fifteenByteBuf);
}

float UT61E::getMillivolts(void) {
  return _volts * 1000;
}

float UT61E::getMillivoltsStr(char *fifteenByteBuf) {
  dtostrf(_volts * 1000, -15, 2, fifteenByteBuf);
  this->ttrim(fifteenByteBuf);
}

/****************************************************************************/
// AMPERAGE
/****************************************************************************/
int UT61E::measureMicroamps(byte type) {
  int error = this->readPacketCheckModeType(13, type);      // 13 = uA Mode
  if (error !=  UT61E_SUCCESS) { return error; }
  _amps = this->calculate(8);
  return UT61E_SUCCESS;
}

int UT61E::measureMilliamps(byte type) {
  int error = this->readPacketCheckModeType(15, type);      // 15 = mA Mode
  if (error !=  UT61E_SUCCESS) { return error; }
  _amps = this->calculate(6);
  return UT61E_SUCCESS;
}

int UT61E::measureAmps(byte type) {
   int error = this->readPacketCheckModeType(0, type);      // 0 = Amps Mode
  if (error !=  UT61E_SUCCESS) { return error; }
  _amps = this->calculate(2);
  return UT61E_SUCCESS;
}

float UT61E::getAmps(void) {
  return _amps;
}

void UT61E::getAmpsStr(char *fifteenByteBuf) {
  if (_packet.mode == 13) {                     //13 = uA Mode
    dtostrf(_amps, -15, 7, fifteenByteBuf);
  } else if (_packet.mode == 15) {              //15 = mA Mode
    dtostrf(_amps, -15, 6, fifteenByteBuf);
  } else {
    dtostrf(_amps, -15, 3, fifteenByteBuf);
  }
  this->ttrim(fifteenByteBuf);
}

float UT61E::getMilliAmps(void) {
  return _amps * 1000;
}

void UT61E::getMilliampsStr(char *fifteenByteBuf) {
  dtostrf(_amps * 1000, -15, 3, fifteenByteBuf);
  this->ttrim(fifteenByteBuf);
}

float UT61E::getMicroAmps(void) {
  return _amps * 1000000;
}

void UT61E::getMicroampsStr(char *fifteenByteBuf) {
  dtostrf(_amps * 1000000, -15, 1, fifteenByteBuf);
  this->ttrim(fifteenByteBuf);
}

/****************************************************************************/
// Debugging
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
    Serial.print("| ");
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
        SerialObj->println("ERROR: VOLTAGE NOT SET TO DC");
        return;
      case UT61E_ERROR_VOLTAGE_NOT_AC:
        SerialObj->println("ERROR: VOLTAGE NOT SET TO AC");
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

void UT61E::ttrim(char* buf) {
  size_t size = strlen(buf);
  for (byte i = 0; i < size; i++) {
    if (buf[i] == ' ') {
      buf[i] = 0;
    }
  }
}