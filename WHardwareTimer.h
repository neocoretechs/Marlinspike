/*
 * WHardwareTimer.h
 *
 * Created: 4/7/2014 10:16:20 PM
 *  Author: jg
 */ 


#ifndef WHARDWARETIMER_H_
#define WHARDWARETIMER_H_

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stddef.h>
#include "WInterruptService.h"
#include "WInterruptsBase.h"

#define CLOCK_STOP                      0
#define CLOCK_NO_PRESCALE               1
#define CLOCK_PRESCALE_8                2
#define CLOCK_PRESCALE_32               3
#define CLOCK_PRESCALE_64               4
#define CLOCK_PRESCALE_128              5
#define CLOCK_PRESCALE_256              6
#define CLOCK_PRESCALE_1024             7
#define CLOCK_EXTERNAL_FALLING          8
#define CLOCK_EXTERNAL_RISING           9

#define INTERRUPT_OVERFLOW              0
#define INTERRUPT_COMPARE_MATCH_A       1
#define INTERRUPT_COMPARE_MATCH_B       2
#define INTERRUPT_COMPARE_MATCH_C       3
#define INTERRUPT_CAPTURE_EVENT         4

#define CHANNEL_A                       0
#define CHANNEL_B                       1
#define CHANNEL_C                       2
#define STATE_OVFL						3

ISR(TIMER0_COMPA_vect);
ISR(TIMER0_COMPB_vect);
ISR(TIMER0_OVF_vect);

ISR(TIMER2_COMPA_vect);
ISR(TIMER2_COMPB_vect);
ISR(TIMER2_OVF_vect);

ISR(TIMER1_COMPA_vect);
ISR(TIMER1_COMPB_vect);
ISR(TIMER1_COMPC_vect);
ISR(TIMER1_OVF_vect);
ISR(TIMER1_CAPT_vect);

ISR(TIMER3_COMPA_vect);
ISR(TIMER3_COMPB_vect);
ISR(TIMER3_COMPC_vect);
ISR(TIMER3_OVF_vect);
ISR(TIMER3_CAPT_vect);

ISR(TIMER4_COMPA_vect);
ISR(TIMER4_COMPB_vect);
ISR(TIMER4_COMPC_vect);
ISR(TIMER4_OVF_vect);
ISR(TIMER4_CAPT_vect);

ISR(TIMER5_COMPA_vect);
ISR(TIMER5_COMPB_vect);
ISR(TIMER5_COMPC_vect);
ISR(TIMER5_OVF_vect);
ISR(TIMER5_CAPT_vect);


class HardwareTimer : public virtual InterruptsBase
{
	friend void TIMER0_COMPA_vect();
	friend void TIMER0_COMPB_vect();
	friend void TIMER0_OVF_vect();

	friend void TIMER2_COMPA_vect();
	friend void TIMER2_COMPB_vect();
	friend void TIMER2_OVF_vect();

	friend void TIMER1_COMPA_vect();
	friend void TIMER1_COMPB_vect();
	friend void TIMER1_COMPC_vect();
	friend void TIMER1_OVF_vect();
	friend void TIMER1_CAPT_vect();

	friend void TIMER3_COMPA_vect();
	friend void TIMER3_COMPB_vect();
	friend void TIMER3_COMPC_vect();
	friend void TIMER3_OVF_vect();
	friend void TIMER3_CAPT_vect();

	friend void TIMER4_COMPA_vect();
	friend void TIMER4_COMPB_vect();
	friend void TIMER4_COMPC_vect();
	friend void TIMER4_OVF_vect();
	friend void TIMER4_CAPT_vect();

	friend void TIMER5_COMPA_vect();
	friend void TIMER5_COMPB_vect();
	friend void TIMER5_COMPC_vect();
	friend void TIMER5_OVF_vect();
	friend void TIMER5_CAPT_vect();

	private:
	uint8_t _timerNumber;
	uint8_t _channelCount;
	// Control registers
	volatile uint8_t *_tcntnh;
	volatile uint8_t *_tcntnl;
	volatile uint8_t *_timskn;
	volatile uint8_t *_tccrna;
	volatile uint8_t *_tccrnb;
	volatile uint8_t *_ocrnah;
	volatile uint8_t *_ocrnal;
	volatile uint8_t *_ocrnbh;
	volatile uint8_t *_ocrnbl;
	volatile uint8_t *_ocrnch;
	volatile uint8_t *_ocrncl;
	volatile uint8_t *_tifrn;
	uint8_t _tovrn;
	uint8_t _ocfna;
	uint8_t _ocfnb;
	uint8_t _ocfnc;
	uint8_t _toie;
	uint8_t _ociea;
	uint8_t _ocieb;
	uint8_t _ociec;
	uint8_t _icie;
	// User interrupt handlers
	InterruptService* overflowFunction;
	InterruptService* compareMatchAFunction;
	InterruptService* compareMatchBFunction;
	InterruptService* compareMatchCFunction;
	InterruptService* captureEventFunction;
	
	public:
	HardwareTimer(uint8_t timerNumber);
	inline uint8_t getTimerNumber(void) { return _timerNumber; }
	inline uint8_t stop(void) { return setClockSource(CLOCK_STOP); };
	void stopChannel(uint8_t channel);
	uint8_t setClockSource(uint8_t clockSource);
	void setInterrupt(uint8_t interrupt, uint8_t value);
	inline void enableInterrupt(uint8_t interrupt) { setInterrupt(interrupt, 1); };
	inline void disableInterrupt(uint8_t interrupt) { setInterrupt(interrupt, 0); };
	virtual uint8_t attachInterrupt(InterruptService* userFunc, int enable = 1);
	void attachInterrupt(uint8_t interrupt, InterruptService* userFunc, int enable = 1);
	void detachInterrupt(uint8_t interrupt);
	void setMode(uint8_t mode);
	// uint16_t getClockSource();
	void setOutputMode(uint8_t channel, uint8_t outputMode);
	void setOCR(uint8_t channel, uint16_t value);
	void setCounter(uint16_t value);
	uint16_t getCounter(void);
	void clearInterrupt(uint8_t channel);
	void clearInterrupt(void);
	void waitForOverflow(void);
};

extern HardwareTimer Timer0;
extern HardwareTimer Timer1;
extern HardwareTimer Timer2;
extern HardwareTimer Timer3;
extern HardwareTimer Timer4;
extern HardwareTimer Timer5;


#endif /* WHARDWARETIMER_H_ */