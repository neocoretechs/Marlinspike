/*
* Analog functions, subclass of digital. Analog input on the Mega 2560 is P54 thru P69.
* Corresponding to analog 0 thru 15.
* Modified 3/2014 Groff
*/
#ifndef Analog_h
#define Analog_h

#include "Arduino.h"
#include "pins_arduino.h"
#include "WDigital.h"

class Analog : public virtual Digital {
	private:
	uint8_t analog_channel = 0;
	public:
		Analog(uint8_t spin) : Digital(spin) { 	sbi(ADCSRA, ADEN); }
		uint8_t analog_reference = DEFAULT;

void analogReference(uint8_t mode)
{
	// can't actually set the register here because the default setting
	// will connect AVCC and the AREF pin, which would cause a short if
	// there's something connected to AREF.
	analog_reference = mode;
}
/*
* D54 thru D69 are the analog input pins on the Mega. A0-A15.
* Compute the analog channel from the given pin, but maintain the pin number as its digital designation.
*/
int analogRead()
{
	uint8_t low, high;
	if (pin >= 54 && pin <= 69) 
		analog_channel = pin - 54; // allow for pin numbers
	else
		return -1; // can analog read a digital pin

	// the MUX5 bit of ADCSRB selects whether we're reading from channels
	// 0 to 7 (MUX5 low) or 8 to 15 (MUX5 high).
	ADCSRB = (ADCSRB & ~(1 << MUX5)) | (((analog_channel >> 3) & 0x01) << MUX5);
  
	// set the analog reference (high two bits of ADMUX) and select the
	// channel (low 4 bits).  this also sets ADLAR (left-adjust result)
	// to 0 (the default).
	ADMUX = (analog_reference << 6) | (analog_channel & 0x07);

	// without a delay, we seem to read from the wrong channel
	//_delay_ms(1);

	// start the conversion
	sbi(ADCSRA, ADSC);

	// ADSC is cleared when the conversion finishes
	while (bit_is_set(ADCSRA, ADSC));

	// we have to read ADCL first; doing so locks both ADCL
	// and ADCH until ADCH is read.  reading ADCL second would
	// cause the results of each conversion to be discarded,
	// as ADCL and ADCH would be locked when it completed.
	low  = ADCL;
	high = ADCH;

	// combine the two bytes
	return (high << 8) | low;
}


};
#endif
