#include "Arduino.h"
#include "RoboCore.h"
#include "new.h"

/*
* Main entry point for the Marlinspike RoboCore real time robotics control platform.
* The function enters the main processing loop, from which the serial data
* is read and processed to invoke the proper M and G codes to activate robotic control
* functions.
* Author: Jonathan N. Groff. Copyright NeoCoreTechs 2012,2013,2014,2015,2016,2020
*/
int main(void)
{
	// Clear the reset bit
	MCUSR &= ~_BV(WDRF);
	// Disable the WDT
	WDTCSR |= _BV(WDCE) | _BV(WDE);
	WDTCSR = 0;
	// Enter the main processing setup
	setup();
	for (;;) {
		loop();
	}

}

