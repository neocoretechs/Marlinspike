/*
 * Ultrasonic.h
 *
 * Created: 7/25/2014 11:44:28 PM
 *  Author: jg
 */ 
#ifndef ULTRASONIC_H_
#define ULTRASONIC_H_
#include "WDigital.h"

class Ultrasonic {
	private:
		unsigned long duration;
	public:
		Digital* pin;
		float getRange();
		Ultrasonic(int ipin) {
			pin = new Digital(ipin);
		}
		uint8_t getPin(void) { return pin->pin; }
		/*The measured distance from the range 0 to 400 Centimeters*/
		long microsecondsToCentimeters(void);
		/*The measured distance from the range 0 to 157 Inches*/
		long microsecondsToInches(void);
};
#endif /* ULTRASONIC_H_ */