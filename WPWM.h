/*
 * WPWM.h
 *
 * Created: 4/7/2014 8:47:26 PM
 *  Author: jg
 */ 
#ifndef WPWM_H_
#define WPWM_H_
#include "pins_arduino.h"
#include "WDigital.h"
#include "WHardwareTimer.h"
#include "WInterruptService.h"
class PWM {
	public:
	uint8_t pin;
	uint8_t mode = OUTPUT;
	HardwareTimer* timer;
	uint8_t channel = 0;
	InterruptService* interruptService=NULL;
	PWM(uint8_t spin);
	void init(uint8_t spin);
	void pwmWrite(uint16_t val, uint8_t outputMode = 0b10);
	inline void pwmOff() { pwmWrite(0, 0); };
	void setPWMResolution(uint8_t bitResolution);
	void setPWMPrescale(uint8_t prescalar);
	void setCounter(int cntx);
	int getCounter(void);
	void digitalWrite(uint8_t val);
	void pinModeOut(void);
	void attachInterrupt(InterruptService* cins, bool overflow = false);
	void detachInterrupt(bool overflow = false);
};


#endif /* WPWM_H_ */