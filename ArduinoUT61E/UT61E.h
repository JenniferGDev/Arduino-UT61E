#ifndef UT61E_HEADER_FILE
#define UT61E_HEADER_FILE

#include <arduino.h>

#define UT61E_DEBUG 1                  // Set to one to enable printPacket() & printErrorMessage().
                                       // This option to disable is to preserve precious space on
                                       // the small arduino devices.

#define UT61E_AC 1
#define UT61E_DC 2  

#define UT61E_SUCCESS                  1
#define UT61E_ERROR_TIMEOUT           -1
#define UT61E_ERROR_READING_PACKET    -2
#define UT61E_ERROR_INVALID_MODE      -3
#define UT61E_ERROR_VOLTAGE_NOT_DC    -4
#define UT61E_ERROR_VOLTAGE_NOT_AC    -5
#define UT61E_ERROR_OVERLOAD          -6

struct UT61E_Packet {
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
  UT61E(HardwareSerial* serialObj);
  int readPacket(void);
  int measureResistance(void);
  float getResistance(void);
  int measureVolts(byte type);
  int measureMillivolts(byte type);
  float getVolts(void);
  void getVoltsStr(char *fifteenByteBuf);
  float getMillivolts(void);
  float getMillivoltsStr(char *fifteenByteBuf);
  int measureMicroamps(byte type);
  int measureMilliamps(byte type);
  int measureAmps(byte type);
  float getAmps(void);
  void getAmpsStr(char *fifteenByteBuf);
  float getMilliAmps(void);
  void getMilliampsStr(char *fifteenByteBuf);
  float getMicroAmps(void);
  void getMicroampsStr(char *fifteenByteBuf);
  #if UT61E_DEBUG == 1
    void printPacket(void);
    void printErrorMessage(HardwareSerial* SerialObj, int error);
  #endif
private:
  HardwareSerial* _Serial;
  int _dtrPin;
  struct UT61E_Packet _packet;
  float _resistance;
  float _volts;
  float _amps;
  void setup(HardwareSerial* serialObj);
  void massagePacket(void);
  float calculate(byte magnitude);
  int readPacketCheckMode(byte mode);
  int readPacketCheckModeType(byte mode, byte type);
  long lpow(byte base, byte exponent);
  void ttrim(char* buf);
};

#endif