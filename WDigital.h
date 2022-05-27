/*
* Defines digital pin functions. base class for more defined functionality
*/
#ifndef Digital_h
#define Digital_h

#include "Arduino.h"
#include "pins_arduino.h"
#include "WHardwareTimer.h"

class Digital {
	public:
	uint8_t pin;
	uint8_t mode = INPUT; // default
	Digital(uint8_t spin) {
		this->pin = spin;
	}
	
void setPin(uint8_t spin) { this->pin = spin; }
	
void pinMode(uint8_t pmode) {
	this->mode = pmode;
	uint8_t bit = digitalPinToBitMask(this->pin);
	uint8_t port = digitalPinToPort(this->pin);
	volatile uint16_t *reg, *out;

	if (port == NOT_A_PIN) return;

	reg = portModeRegister(port);
	out = portOutputRegister(port);

	if (this->mode == INPUT) { 
		uint8_t oldSREG = SREG;
                cli();
		*reg &= ~bit;
		*out &= ~bit;
		SREG = oldSREG;
	} else if (this->mode == INPUT_PULLUP) {
		uint8_t oldSREG = SREG;
                cli();
		*reg &= ~bit;
		*out |= bit;
		SREG = oldSREG;
	} else {
		uint8_t oldSREG = SREG;
                cli();
		*reg |= bit;
		SREG = oldSREG;
	}
}

// If we detect the pin is on a timer, we may need to defuse PWM output, 
// we need to turn pin off before doing a digital write.
static void turnOffPWM(uint8_t timer)
{
	switch (timer)
	{
		case TIMER0A: Timer0.stopChannel(CHANNEL_A); break;
		case TIMER0B: Timer0.stopChannel(CHANNEL_B); break;
	
		case TIMER1A: Timer1.stopChannel(CHANNEL_A); break;
		case TIMER1B: Timer1.stopChannel(CHANNEL_B); break;
		
		case TIMER2A: Timer2.stopChannel(CHANNEL_A); break;
		case TIMER2B: Timer2.stopChannel(CHANNEL_B); break;
	
		case TIMER3A: Timer3.stopChannel(CHANNEL_A); break;
		case TIMER3B: Timer3.stopChannel(CHANNEL_B); break; 
		case TIMER3C: Timer3.stopChannel(CHANNEL_C); break;
		
		case TIMER4A: Timer4.stopChannel(CHANNEL_A); break;
		case TIMER4B: Timer4.stopChannel(CHANNEL_B); break; 
		case TIMER4C: Timer4.stopChannel(CHANNEL_C); break;
		
		case TIMER5A: Timer5.stopChannel(CHANNEL_A); break;
		case TIMER5B: Timer5.stopChannel(CHANNEL_B); break; 
		case TIMER5C: Timer5.stopChannel(CHANNEL_C); break;
	}
}

void digitalWrite(uint8_t val)
{
	uint8_t timer = digitalPinToTimer(this->pin);
	uint8_t bit = digitalPinToBitMask(this->pin);
	uint8_t port = digitalPinToPort(this->pin);
	volatile uint16_t *out;

	if (port == NOT_A_PIN) return;

	// If PWM output, we need to turn pin off before doing a digital write.
	if (timer != NOT_ON_TIMER) turnOffPWM(timer);

	out = portOutputRegister(port);

	uint8_t oldSREG = SREG;
	cli();

	if (val == LOW) {
		*out &= ~bit;
	} else {
		*out |= bit;
	}

	SREG = oldSREG;
}

int digitalRead()
{
	uint8_t timer = digitalPinToTimer(this->pin);
	uint8_t bit = digitalPinToBitMask(this->pin);
	uint8_t port = digitalPinToPort(this->pin);

	if (port == NOT_A_PIN) return LOW;

	// If the pin that support PWM output, we need to turn it off
	// before getting a digital reading.
	if (timer != NOT_ON_TIMER) turnOffPWM(timer);

	if (*portInputRegister(port) & bit) return HIGH;
	return LOW;
}
/* Measures the length (in microseconds) of a pulse on the pin; state is HIGH
 * or LOW, the type of pulse to measure.  Works on pulses from 2-3 microseconds
 * to 3 minutes in length, but must be called at least a few dozen microseconds
 * before the start of the pulse. */
unsigned long pulseIn(uint8_t state, unsigned long timeout=1000000L) {
		// cache the port and bit of the pin in order to speed up the
		// pulse width measuring loop and achieve finer resolution.  calling
		// digitalRead() instead yields much coarser resolution.
		uint8_t bit = digitalPinToBitMask(this->pin);
		uint8_t port = digitalPinToPort(this->pin);
		uint8_t stateMask = (state ? bit : 0);
		unsigned long width = 0; // keep initialization out of time critical area
		// convert the timeout from microseconds to a number of times through
		// the initial loop; it takes 16 clock cycles per iteration.
		unsigned long numloops = 0;
		unsigned long maxloops = microsecondsToClockCycles(timeout) / 16;
		// wait for any previous pulse to end
		while ((*portInputRegister(port) & bit) == stateMask)
			if (numloops++ == maxloops)
				return 0;	
		// wait for the pulse to start
		while ((*portInputRegister(port) & bit) != stateMask)
			if (numloops++ == maxloops)
				return 0;
	
		// wait for the pulse to stop
		while ((*portInputRegister(port) & bit) == stateMask) {
			if (numloops++ == maxloops)
				return 0;
			width++;
		}
		// convert the reading to microseconds. The loop has been determined
		// to be 20 clock cycles long and have about 16 clocks between the edge
		// and the start of the loop. There will be some error introduced by
		// the interrupt handlers.
		return clockCyclesToMicroseconds(width * 21 + 16); 
}


};
#endif
