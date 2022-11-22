#ifndef UT61E_HEADER_FILE
#define UT61E_HEADER_FILE

#include <arduino.h>

#define UT61E_SUCCESS                   1
#define UT61E_ERROR_TIMEOUT             -1
#define UT61E_ERROR_READING_PACKET      -2
#define UT61E_ERROR_INVALID_MODE        -3 

struct Packet {
    byte range;
    byte digit1;
    byte digit2;
    byte digit3;
    byte digit4;
    byte digit5;
    byte mode;
    byte info_flags;
    byte relative_mode_flags;
    byte limit_flags;
    byte voltage_and_autorange_flags;
    byte hold;
    byte cr;
    byte lf;
  };

class UT61E {
public:
  UT61E(HardwareSerial* serialObj, int dtrPin);
  int measureResistance(void);
  float getResistance(void);
private:
  HardwareSerial* _Serial;
  int _dtrPin;
  struct Packet _packet;
  float _resistance;
  bool readPacket(void);
  void massagePacket(void);
};

#endif