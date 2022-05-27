/*
 * WPWM.cpp
 * Class to encapsulate Pulse width modulation of a pin via onboard timers.
 * Uses timer objects depending on desired pin. Timers are pre-instantiated at startup as Timer0, Timer1...Timer5
 * init() method must be called before resolution and prescalar as they act on the timer chosen.
 * an option to add an interrupt derived from CounterInterruptService, a simple counter of cycles, is provided.
 * Created: 4/7/2014 9:49:17 PM
 *  Author: jg
 */ 
#include "WPWM.h"
	/*
	* Constructor 
	*/
	PWM::PWM(uint8_t spin) {
		this->pin = spin;
		pinModeOut();
	}

	void PWM::init(uint8_t spin) {
	 this->pin = spin; 
	 pinModeOut();
	 switch (digitalPinToTimer(this->pin)) {
		case TIMER0A:
			channel = CHANNEL_A;
			timer = &Timer0;
			break;
		case TIMER0B:
			channel= CHANNEL_B;
			timer = &Timer0;
		break;
		case TIMER1A:
			channel = CHANNEL_A;
			timer = &Timer1;
			break;
		case TIMER1B:
			channel = CHANNEL_B;
			timer = &Timer1;
		break;
		case TIMER2A:
			channel = CHANNEL_A;
			timer = &Timer2;
			break;
		case TIMER2B:
			channel = CHANNEL_B;
			timer = &Timer2;
		break;
		case TIMER3A:
			channel = CHANNEL_A;
			timer = &Timer3;
			break;
		case TIMER3B:
			channel = CHANNEL_B;
			timer = &Timer3;
			break;
		case TIMER3C:
			channel = CHANNEL_C;
			timer = &Timer3;
		break;
		case TIMER4A:
			channel = CHANNEL_A;
			timer = &Timer4;
			break;
		case TIMER4B:
			channel = CHANNEL_B;
			timer = &Timer4;
			break;
		case TIMER4C:
			channel = CHANNEL_C;
			timer = &Timer4;
		break;
		case TIMER5A:
			channel = CHANNEL_A;
			timer = &Timer5;
			break;
		case TIMER5B:
			channel = CHANNEL_B;
			timer = &Timer5;
			break;
		case TIMER5C:
			channel = CHANNEL_C;
			timer = &Timer5;
		break;
		case NOT_ON_TIMER:
		default:
			timer=NULL;
	  }

	}
	/*
	* Sets up the PWM pin for interrupt service with a counter or its subclass.
	* Sets the counter to 0, attaches the interrupt to be serviced each cycle.
	* The channel is determined from the pin, however, if we want an overflow
	* interrupt set the overflow flag (default FALSE) to TRUE
	* ASSUMES OVERLOADED INIT FOR PIN ASSIGNMENT HAS BEEN CALLED!
	*/
	void PWM::attachInterrupt(InterruptService* cins, bool overflow) {
		this->interruptService = cins;
		if( overflow ) {
			if( timer )
				timer->attachInterrupt(INTERRUPT_OVERFLOW, this->interruptService);
		} else { // compare match
		   switch (digitalPinToTimer(this->pin)) {
			case TIMER0A:
			case TIMER1A:
			case TIMER2A:
			case TIMER3A:
			case TIMER4A:
			case TIMER5A:
				if( timer )
					timer->attachInterrupt(INTERRUPT_COMPARE_MATCH_A, this->interruptService);
				break;
			case TIMER0B:
			case TIMER1B:
			case TIMER2B:
			case TIMER3B:
			case TIMER4B:
			case TIMER5B:
				if( timer )
					timer->attachInterrupt(INTERRUPT_COMPARE_MATCH_B, this->interruptService);
				break;
			case TIMER3C:
			case TIMER4C:
			case TIMER5C:
				if( timer )
					timer->attachInterrupt(INTERRUPT_COMPARE_MATCH_C, this->interruptService);
				break;
			case NOT_ON_TIMER:
				default:
				break;
		   }
		}
	
	}
	
	void PWM::detachInterrupt(bool overflow) {
		if( overflow ) {
			if( timer )
				timer->detachInterrupt(INTERRUPT_OVERFLOW);
		} else { // compare match
		   switch (digitalPinToTimer(this->pin)) {
			case TIMER0A:
			case TIMER1A:
			case TIMER2A:
			case TIMER3A:
			case TIMER4A:
			case TIMER5A:
				if( timer )
					timer->detachInterrupt(INTERRUPT_COMPARE_MATCH_A);
				break;
			case TIMER0B:
			case TIMER1B:
			case TIMER2B:
			case TIMER3B:
			case TIMER4B:
			case TIMER5B:
				if( timer )
					timer->detachInterrupt(INTERRUPT_COMPARE_MATCH_B);
				break;
			case TIMER3C:
			case TIMER4C:
			case TIMER5C:
				if( timer )
					timer->detachInterrupt(INTERRUPT_COMPARE_MATCH_C);
				break;
			case NOT_ON_TIMER:
				default:
				break;
		   }
		}
		this->interruptService = NULL;
	}
			
	void PWM::pwmWrite(uint16_t val, uint8_t outputMode)
	{
		// If 0, turn off PWM (some OCRn registers, in fast PWM mode, will still generate spikes when its OCR is set to zero).
		if (val == 0)
		{
			outputMode = 0; // turn off PWM
			digitalWrite(LOW);
		}
		if( timer ) {
			(*timer).setOutputMode(channel, outputMode);
			(*timer).setOCR(channel, val);
		} else {
			if (val < 128)
			{
				digitalWrite(LOW);
			}
			else
			{
				digitalWrite(HIGH);
			}
		}
	}

	void PWM::setPWMResolution(uint8_t bitResolution)
	{
		uint8_t mode = 0b0101; // fast 8 default
		if( timer ) {
			if( (*timer).getTimerNumber() == 2 ) {
				mode = 0b0011; // fast 8 for timer 2
			} else {
				if (bitResolution == 9) // fast 9
					mode = 0b0110;
				else 
					if (bitResolution == 10) // fast 10
						mode = 0b0111;
			}
			(*timer).setMode(mode);
			(*timer).setOCR(channel,0);
		}
	}

	void PWM::setPWMPrescale(uint8_t prescalar)
	{
		if( timer ) (*timer).setClockSource(prescalar);
	}
	

	void PWM::digitalWrite(uint8_t val)
	{
		uint8_t timer = digitalPinToTimer(this->pin);
		uint8_t bit = digitalPinToBitMask(this->pin);
		uint8_t port = digitalPinToPort(this->pin);
		volatile uint16_t *out;

		if (port == NOT_A_PIN) return;

		// If PWM output, we need to turn pin off before doing a digital write.
		if (timer != NOT_ON_TIMER) Digital::turnOffPWM(timer);

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
	
	void PWM::pinModeOut(void) {
			uint8_t bit = digitalPinToBitMask(this->pin);
			uint8_t port = digitalPinToPort(this->pin);
			volatile uint16_t *reg, *out;

			if (port == NOT_A_PIN) return;

			reg = portModeRegister(port);
			out = portOutputRegister(port);
				
			uint8_t oldSREG = SREG;
			cli();
			*reg |= bit; // output mode
			SREG = oldSREG;
	}
