Arduino C++ Library for getting measurements results from UT61E using the DB9 RS-232 cable.

The UT61E class will evolve over time adding ability to measure more than just resistance.

This project wouldn't of been possible without the packet information from the following github project:

https://github.com/4x1md/ut61e_py

Special thanks to synthiq on Modwigglers.com for informing me that the RX signal from the DB9 of the UT61E needs to be inverted before going into the Arduino RX pin.  I am using a 74HC14N to accomplish this.

Settings:  19200 baud, 7 word length, odd parity, one stop bit.

The DTR pin must be set to high (+5V) -- pin 4 on the DB9.  Also RTS pin -- pin 7 on DB9 -- must be tied to ground.  DTR and RTS together provide the power to the optical RS-232 interface.  Pin 5 on the DB9 needs to be tied to ground.  Pin 2 on the DB9 is the RX signal -- again it must be inverted before sending to the Arduino serial RX pin.

2022-11-26 UDPATE:<br>
Added the following public methods for measuring voltage:<br>
&nbsp;&nbsp;int measureVoltageDC(void);<br>
&nbsp;&nbsp;int measureVoltageAC(void);<br>
&nbsp;&nbsp;float getVolts(void);<br>
&nbsp;&nbsp;void getVoltsStr(char* buf);<br>
Added the following debugging methods, which can be disabled with #define UT61E_DEBUG defined as 0.<br>
&nbsp;&nbsp;void printPacket(void);<br>
&nbsp;&nbsp;void printErrorMessage(HardwareSerial* SerialObj, int error);<br>

