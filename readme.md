Arduino C++ Library for getting measurements results from UT61E using the DB9 RS-232 cable.

The UT61E class will evolve over time adding ability to measure more than just resistance.

This project wouldn't of been possible without the packet information from the following github project:

https://github.com/4x1md/ut61e_py

Special thanks to synthiq on Modwigglers.com for informing me that the RX signal from the DB9 of the UT61E needs to be inverted before going into the Arduino RX pin.  I am using a 74HC14N to accomplish this.
