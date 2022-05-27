/*
 * WInterrupts.h
 *
 * Created: 3/4/2014 11:56:39 PM
 *  Author: jg
 */ 


#ifndef WINTERRUPTS_H_
#define WINTERRUPTS_H_
#include "WInterruptsBase.h"
#include "WInterruptService.h"

	static uint8_t highInt = 0; 
class Interrupts: public InterruptsBase {
	private:
	public:
	uint8_t attachInterrupt(InterruptService* userFunc, int mode);
	void attachInterrupt(uint8_t interruptNum, InterruptService* userFunc, int mode);
	void detachInterrupt(uint8_t interruptNum);
};

#endif /* WINTERRUPTS_H_ */