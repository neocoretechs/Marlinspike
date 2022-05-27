/*
 * WPCInterrupts.h
 *
 * Created: 3/8/2014 12:27:33 AM
 *  Author: jg
 */ 

#ifndef WPCINTERRUPTS_H_
#define WPCINTERRUPTS_H_
#include "Arduino.h"
#include "pins_arduino.h"
#include "WInterruptService.h"
#include "WInterruptsBase.h"
/*
 * Since PCINTs are per port, not per pin, the ISR
 * must use some logic to actually implement a per-pin interrupt service.
 * look up PCMSK to determine ISR 0-3, port position and port. once we have those we can map the rest with macros
 * We make no attempt to ensure that the stated pin is set up as INPUT
 */

static int PCintMode[24];
static InterruptService* PCintFunc[24];
volatile static uint8_t PCintLast[3];
volatile static const uint8_t *PCmsk[3]={&PCMSK0,&PCMSK1,&PCMSK2};
volatile static const uint8_t *PCport[3] = {&PINB,&PINJ,&PINK};
	
class PCInterrupts: public InterruptsBase {
	private:
	public:
	uint8_t pin;
	PCInterrupts(void){}
	uint8_t attachInterrupt(InterruptService* userFunc, int mode);
	void attachInterrupt(uint8_t pin, InterruptService* userFunc, int mode);
	void detachInterrupt(uint8_t interruptNum);
};


#endif /* WPCINTERRUPTS_H_ */