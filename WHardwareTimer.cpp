/*
 * WHardwareTimer.cpp
 * Unified timer class. The approach is to isolate the target timer, of the 5 available on the 2560, and from there
 * the operations that take the individual channels on the timer (A,B or A,B,C for 16 bit) have that channel passed as
 * a method parameter
 * Created: 4/7/2014 10:17:59 PM
 * Author: jg
 */ 
#include "WHardwareTimer.h"

// Compare Output Mode bits
// For 16 bit timers
#define COMnA1 7
#define COMnA0 6
#define COMnB1 5
#define COMnB0 4
#define COMnC1 3
#define COMnC0 2
// For 8 bit timers with only single OCR
#define COMn1 5
#define COMn0 4


ISR(TIMER0_COMPA_vect)
{
  if (Timer0.compareMatchAFunction)
    (*Timer0.compareMatchAFunction).service();
}
ISR(TIMER0_COMPB_vect)
{
  if (Timer0.compareMatchBFunction)
    (*Timer0.compareMatchBFunction).service();
}
ISR(TIMER0_OVF_vect)
{
  if (Timer0.overflowFunction)
    (*Timer0.overflowFunction).service();
}

ISR(TIMER2_COMPA_vect)
{
  if (Timer2.compareMatchAFunction)
    (*Timer2.compareMatchAFunction).service();
}
ISR(TIMER2_COMPB_vect)
{
  if (Timer2.compareMatchBFunction)
   (*Timer2.compareMatchBFunction).service();
}
ISR(TIMER2_OVF_vect)
{
  if (Timer2.overflowFunction)
    (*Timer2.overflowFunction).service();
}

ISR(TIMER1_COMPA_vect)
{
  if (Timer1.compareMatchAFunction)
    (*Timer1.compareMatchAFunction).service();
}
ISR(TIMER1_COMPB_vect)
{
  if (Timer1.compareMatchBFunction)
    (*Timer1.compareMatchBFunction).service();
}

ISR(TIMER1_COMPC_vect)
{
  if (Timer1.compareMatchCFunction)
    (*Timer1.compareMatchCFunction).service();
}

ISR(TIMER1_OVF_vect)
{
  if (Timer1.overflowFunction)
    (*Timer1.overflowFunction).service();
}
ISR(TIMER1_CAPT_vect)
{
  if (Timer1.captureEventFunction)
    (*Timer1.captureEventFunction).service();
}

ISR(TIMER3_COMPA_vect)
{
  if (Timer3.compareMatchAFunction)
    (*Timer3.compareMatchAFunction).service();
}
ISR(TIMER3_COMPB_vect)
{
  if (Timer3.compareMatchBFunction)
    (*Timer3.compareMatchBFunction).service();
}
ISR(TIMER3_COMPC_vect)
{
  if (Timer3.compareMatchCFunction)
   (*Timer3.compareMatchCFunction).service();
}
ISR(TIMER3_OVF_vect)
{
  if (Timer3.overflowFunction)
    (*Timer3.overflowFunction).service();
}
ISR(TIMER3_CAPT_vect)
{
  if (Timer3.captureEventFunction)
    (*Timer3.captureEventFunction).service();
}

ISR(TIMER4_COMPA_vect)
{
  if (Timer4.compareMatchAFunction)
    (*Timer4.compareMatchAFunction).service();
}
ISR(TIMER4_COMPB_vect)
{
  if (Timer4.compareMatchBFunction)
    (*Timer4.compareMatchBFunction).service();
}
ISR(TIMER4_COMPC_vect)
{
  if (Timer4.compareMatchCFunction)
    (*Timer4.compareMatchCFunction).service();
}
ISR(TIMER4_OVF_vect)
{
  if (Timer4.overflowFunction)
    (*Timer4.overflowFunction).service();
}
ISR(TIMER4_CAPT_vect)
{
  if (Timer4.captureEventFunction)
    (*Timer4.captureEventFunction).service();
}

ISR(TIMER5_COMPA_vect)
{
  if (Timer5.compareMatchAFunction)
    (*Timer5.compareMatchAFunction).service();
}
ISR(TIMER5_COMPB_vect)
{
  if (Timer5.compareMatchBFunction)
    (*Timer5.compareMatchBFunction).service();
}
ISR(TIMER5_COMPC_vect)
{
  if (Timer5.compareMatchCFunction)
    (*Timer5.compareMatchCFunction).service();
}
ISR(TIMER5_OVF_vect)
{
  if (Timer5.overflowFunction)
    (*Timer5.overflowFunction).service();
}
ISR(TIMER5_CAPT_vect)
{
  if (Timer5.captureEventFunction)
    (*Timer5.captureEventFunction).service();
}

// Constructor

HardwareTimer::HardwareTimer(uint8_t timerNumber)
{
  _timerNumber = timerNumber;

  switch (timerNumber)
  {
    case 0:
      _tcntnh = NULL;
      _tcntnl = &TCNT0;
      _timskn = &TIMSK0;
      _tccrna = &TCCR0A;
      _tccrnb = &TCCR0B;
	  
      _ocrnah = NULL;
      _ocrnal = &OCR0A;
      _ocrnbh = NULL;
      _ocrnbl = &OCR0B;
      _ocrnch = NULL;
      _ocrncl = NULL;
	  
	  _tifrn = &TIFR0;
	  _tovrn = TOV0;
	  _ocfna = OCF0A;
	  _ocfnb = OCF0B;
	  _ocfnc = NULL;
	  
	  _toie = TOIE0;
	  _ociea = OCIE0A;
	  _ocieb = OCIE0B;
	  _ociec = NULL;
	  _icie = NULL;
	  
      _channelCount = 2;
      break;

    case 1:
      _timskn = &TIMSK1;
      _tcntnh = &TCNT1H;
      _tcntnl = &TCNT1L;
      _tccrna = &TCCR1A;
      _tccrnb = &TCCR1B;

      _ocrnah = &OCR1AH;
      _ocrnal = &OCR1AL;
      _ocrnbh = &OCR1BH;
      _ocrnbl = &OCR1BL;
      _ocrnch = &OCR1CH;
      _ocrncl = &OCR1CL;
	  
	  _tifrn = &TIFR1;
	  _tovrn = TOV1;
	  _ocfna = OCF1A;
	  _ocfnb = OCF1B;
	  _ocfnc = OCF1C;
	  
	  _toie = TOIE1;
	  _ociea = OCIE1A;
	  _ocieb = OCIE1B;
	  _ociec = OCIE1C;
	  _icie = ICIE1;
	  
      _channelCount = 3;
      break;

    case 2:
      _tcntnh = NULL;
      _tcntnl = &TCNT2;
      _timskn = &TIMSK2;
      _tccrna = &TCCR2A;
      _tccrnb = &TCCR2B;

      _ocrnah = NULL;
      _ocrnal = &OCR2A;
      _ocrnbh = NULL;
      _ocrnbl = &OCR2B;
      _ocrnch = NULL;
      _ocrncl = NULL;
	  
	  _tifrn = &TIFR2;
	  _tovrn = TOV2;
	  _ocfna = OCF2A;
	  _ocfnb = OCF2B;
	  _ocfnc = NULL;
	   
	  _toie = TOIE2;
	  _ociea = OCIE2A;
	  _ocieb = OCIE2B;
	  _ociec = NULL;
	  _icie = NULL;
	  
      _channelCount = 2;
      break;

    case 3:
      _timskn = &TIMSK3;
      _tcntnh = &TCNT3H;
      _tcntnl = &TCNT3L;
      _tccrna = &TCCR3A;
      _tccrnb = &TCCR3B;

      _ocrnah = &OCR3AH;
      _ocrnal = &OCR3AL;
      _ocrnbh = &OCR3BH;
      _ocrnbl = &OCR3BL;
      _ocrnch = &OCR3CH;
      _ocrncl = &OCR3CL;
	  
	  _tifrn = &TIFR3;
	  _tovrn = TOV3;
	  _ocfna = OCF3A;
	  _ocfnb = OCF3B;
	  _ocfnc = OCF3C;
	  
	  _toie = TOIE3;
	  _ociea = OCIE3A;
	  _ocieb = OCIE3B;
	  _ociec = OCIE3C;
	  _icie = ICIE3;
	  
      _channelCount = 3;
      break;

    case 4:
      _timskn = &TIMSK4;
      _tcntnh = &TCNT4H;
      _tcntnl = &TCNT4L;
      _tccrna = &TCCR4A;
      _tccrnb = &TCCR4B;

      _ocrnah = &OCR4AH;
      _ocrnal = &OCR4AL;
      _ocrnbh = &OCR4BH;
      _ocrnbl = &OCR4BL;
      _ocrnch = &OCR4CH;
      _ocrncl = &OCR4CL;
	  
	  _tifrn = &TIFR4;
	  _tovrn = TOV4;
	  _ocfna = OCF4A;
	  _ocfnb = OCF4B;
	  _ocfnc = OCF4C;
	   
	  _toie = TOIE4;
	  _ociea = OCIE4A;
	  _ocieb = OCIE4B;
	  _ociec = OCIE4C;
	  _icie = ICIE4;
	  
      _channelCount = 3;
      break;

    case 5:
      _timskn = &TIMSK5;
      _tcntnh = &TCNT5H;
      _tcntnl = &TCNT5L;
      _tccrna = &TCCR5A;
      _tccrnb = &TCCR5B;

      _ocrnah = &OCR5AH;
      _ocrnal = &OCR5AL;
      _ocrnbh = &OCR5BH;
      _ocrnbl = &OCR5BL;
      _ocrnch = &OCR5CH;
      _ocrncl = &OCR5CL;
	  
	  _tifrn = &TIFR5;
	  _tovrn = TOV5;
	  _ocfna = OCF5A;
	  _ocfnb = OCF5B;
	  _ocfnc = OCF5C;
	  
	  _toie = TOIE5;
	  _ociea = OCIE5A;
	  _ocieb = OCIE5B;
	  _ociec = OCIE5C;
	  _icie = ICIE5;
	  
      _channelCount = 3;
      break;
  }

  setClockSource(CLOCK_STOP);
  
  if (_tcntnh != NULL)  // 16 bit timers
    *_tcntnh = 0;
  *_tcntnl = 0;
  
  overflowFunction = NULL;
  compareMatchAFunction = NULL;
  compareMatchBFunction = NULL;
  compareMatchCFunction = NULL;
  captureEventFunction = NULL;
}

void HardwareTimer::clearInterrupt(uint8_t tchannel) {
	    //TIFR3 = _BV(OCF3A);     // clear any pending interrupts;
		//uint8_t oldSREG = SREG;
		//cli();
		if (tchannel == CHANNEL_A) {
				*_tifrn = _BV(_ocfna);
		} else
			if (tchannel == CHANNEL_B) {
				*_tifrn = _BV(_ocfnb);
			 } else 
				if (tchannel == CHANNEL_C) {
				 if( _ocfnc != NULL )
					*_tifrn = _BV(_ocfnc);
				} else
					if( tchannel == STATE_OVFL ) {
						*_tifrn = _BV(_tovrn);
					}
		//SREG = oldSREG;
}

void HardwareTimer::clearInterrupt() {
	//TIFR3 = _BV(OCF3A);     // clear any pending interrupts;
	//uint8_t oldSREG = SREG;
	//cli();
	*_tifrn = _BV(_ocfna) | _BV(_ocfnb) |  _BV(_ocfnc) | _BV(_tovrn);
	//SREG = oldSREG;
}

void HardwareTimer::stopChannel(uint8_t channel) {
	setOutputMode(channel,0);
}

/*
* Return the previous setting for clock source
*/
uint8_t HardwareTimer::setClockSource(uint8_t clockSource)
{
  uint8_t bits = 0;
  uint8_t oldSource;
  // Select our clock source based on timer type and clockSource.
  // Timer 2 is the only RTC clock on the majority of Atmel AVR 8 bit controllers.
  // Timer 2 has a different set of clock prescalers

  if (_timerNumber == 2)
    switch (clockSource)
    {
      case CLOCK_NO_PRESCALE:
        bits = 0b001;
        break;
      case CLOCK_PRESCALE_8:
        bits = 0b010;
        break;
      case CLOCK_PRESCALE_32:
        bits = 0b011;
        break;
      case CLOCK_PRESCALE_64:
        bits = 0b100;
        break;
      case CLOCK_PRESCALE_128:
        bits = 0b101;
        break;
      case CLOCK_PRESCALE_256:
        bits = 0b110;
        break;
      case CLOCK_PRESCALE_1024:
        bits = 0b111;
        break;

      //case CLOCK_STOP:
      //case CLOCK_EXTERNAL_FALLING:
      //case CLOCK_EXTERNAL_RISING:
      default:
        bits = 0;
        break;
    } else
    // all the rest use same prescale divisors
    switch (clockSource)
    {
      case CLOCK_NO_PRESCALE:
        bits = 0b001;
        break;
      case CLOCK_PRESCALE_8:
        bits = 0b010;
        break;
      case CLOCK_PRESCALE_64:
        bits = 0b011;
        break;
      case CLOCK_PRESCALE_256:
        bits = 0b100;
        break;
      case CLOCK_PRESCALE_1024:
        bits = 0b101;
        break;
      case CLOCK_EXTERNAL_FALLING:
        bits = 0b110;
        break;
      case CLOCK_EXTERNAL_RISING:
        bits = 0b111;
        break;

      //case CLOCK_STOP:
      //case CLOCK_PRESCALE_32:
      //case CLOCK_PRESCALE_128:
      default:
        bits = 0;
        break;
    }
  oldSource = (*_tccrnb & 0b00000111);
  *_tccrnb = (*_tccrnb & 0b11111000) | bits;
  return oldSource;
 
}

/*
* Interrupt is one of overflow, compare match/channel, capture I.E. INTERRUPT_OVERFLOW, INTERRUPT_COMPARE_MATCH_A
* If value is zero the interrupt is disabled.
*/
void HardwareTimer::setInterrupt(uint8_t interrupt, uint8_t value)
{
  //uint8_t oldSREG = SREG;
  //cli();
  switch (interrupt)
  {
    case INTERRUPT_OVERFLOW:
	  clearInterrupt(STATE_OVFL);
      *_timskn = (*_timskn & ~(1 << _toie)) | (value << _toie);
      break;
    case INTERRUPT_COMPARE_MATCH_A:
	  clearInterrupt(CHANNEL_A);
      *_timskn = (*_timskn & ~(1 << _ociea)) | (value << _ociea);
      break;
    case INTERRUPT_COMPARE_MATCH_B:
	  clearInterrupt(CHANNEL_B);
      *_timskn = (*_timskn & ~(1 << _ocieb)) | (value << _ocieb);
      break;
    case INTERRUPT_COMPARE_MATCH_C:
	  clearInterrupt(CHANNEL_C);
	  *_timskn = (*_timskn & ~(1 << _ociec)) | (value << _ociec);
      break;
    case INTERRUPT_CAPTURE_EVENT:
     *_timskn = (*_timskn & ~(1 << _icie)) | (value << _icie);
      break;
  }
  //SREG = oldSREG;
  // Only set if the flagBit has been set
  //if (flagBit >= 0)
  //  *_timskn = (*_timskn & ~(1 << flagBit)) | (bitValue << flagBit);

}
uint8_t HardwareTimer::attachInterrupt(InterruptService* userFunc, int enable) { return 0; }

// Attach an interrupt function, and set the appropriate interrupt flag
void HardwareTimer::attachInterrupt(uint8_t interrupt, InterruptService* userFunc, int enable)
{
  switch (interrupt)
  {
    case INTERRUPT_OVERFLOW:
      overflowFunction = userFunc;
      break;
    case INTERRUPT_COMPARE_MATCH_A:
      compareMatchAFunction = userFunc;
      break;
    case INTERRUPT_COMPARE_MATCH_B:
      compareMatchBFunction = userFunc;
      break;
    case INTERRUPT_COMPARE_MATCH_C:
      compareMatchCFunction = userFunc;
      break;
    case INTERRUPT_CAPTURE_EVENT:
      captureEventFunction = userFunc;
      break;
  }

  if (userFunc == NULL)
    setInterrupt(interrupt, 0);
  else
    setInterrupt(interrupt, enable);

}

void HardwareTimer::detachInterrupt(uint8_t interrupt)
{
  setInterrupt(interrupt, 0);
  // clean up, so they wont be serviced
  switch (interrupt) {
    case INTERRUPT_OVERFLOW:
      overflowFunction = NULL;
      break;
    case INTERRUPT_COMPARE_MATCH_A:
      compareMatchAFunction = NULL;
      break;
    case INTERRUPT_COMPARE_MATCH_B:
      compareMatchBFunction = NULL;
      break;
    case INTERRUPT_COMPARE_MATCH_C:
      compareMatchCFunction = NULL;
      break;
    case INTERRUPT_CAPTURE_EVENT:
      captureEventFunction = NULL;
      break;
  }

  //disable use of the given timer
  //*_timskn =  (*_timskn & ~_BV(flagBit)) ;  // disable timer output

}

/*
uint16_t HardwareTimer::getPrescaler()
{
  return (pgm_read_word(timerPrescaleFactor+(*_tccrnb & TIMER_PRESCALE_MASK)));
}
*/
/*
Mod WGMn3 | WGMn2(CTCn) | WGMn1(PWMn1) | WGMn0(PWMn0) | Timer/CounterMode of Operation TOP | Update of OCRnx at | TOVn Flag Set on
0)  0 0 0 0 Normal 0xFFFF Immediate MAX
1)  0 0 0 1 PWM, Phase Correct, 8-bit 0x00FF TOP BOTTOM
2)  0 0 1 0 PWM, Phase Correct, 9-bit 0x01FF TOP BOTTOM
3)  0 0 1 1 PWM, Phase Correct, 10-bit 0x03FF TOP BOTTOM
4)  0 1 0 0 CTC, OCRnA Immediate MAX
5)  0 1 0 1 Fast PWM, 8-bit 0x00FF BOTTOM TOP
6)  0 1 1 0 Fast PWM, 9-bit 0x01FF BOTTOM TOP
7)  0 1 1 1 Fast PWM, 10-bit 0x03FF BOTTOM TOP
8)  1 0 0 0 PWM, Phase and Frequency Correct ICRn BOTTOM BOTTOM
9)  1 0 0 1 PWM, Phase and FrequencyCorrect OCRnA BOTTOM BOTTOM
10) 1 0 1 0 PWM, Phase Correct ICRn TOP BOTTOM
11) 1 0 1 1 PWM, Phase Correct OCRnA TOP BOTTOM
12) 1 1 0 0 CTC, ICRn Immediate MAX
13) 1 1 0 1 (Reserved) – – –
14) 1 1 1 0 Fast PWM, ICRn BOTTOM TOP
15) 1 1 1 1 Fast PWM, OCRnA BOTTOM TOP

EXCEPT for timer 2, whos modes are as follows:
Mo|WGM2|WGM1|WGM0|Timer/Counter Mode of Operation|TOP|Update ofOCRx at|TOV FlagSet on MAX ff BOTTOM 0
0) 0 0 0 Normal 0xFF Immediate MAX
1) 0 0 1 PWM, Phase Correct 0xFF TOP BOTTOM
2) 0 1 0 CTC OCRA Immediate MAX
3) 0 1 1 Fast PWM 0xFF BOTTOM MAX
4) 1 0 0 Reserved – – –
5) 1 0 1 PWM, Phase Correct OCRA TOP BOTTOM
6) 1 1 0 Reserved – – –
7) 1 1 1 Fast PWM OCRA BOTTOM TOP
*/
void HardwareTimer::setMode(uint8_t mode)
{
  // WGMn3 and WGMn2 are in positions 4 and 3, respectively, in TCCRnB
  // WGMn1 and WGNn0 are in positions 1 and 0, respectively, in TCCRnA
  uint8_t oldSREG = SREG;
  cli();
  *_tccrna = (*_tccrna & 0b11111100) | (mode & 0b00000011);
  *_tccrnb = (*_tccrnb & 0b11100111) | ((mode & 0b00001100) << 1);
  SREG = oldSREG;
}

/*
Compare Output Mode, Fast PWM 
COMnA1/COMnB1/COMnC1 | COMnA0/COMnB0/COMnC0 | Description
0 0 Normal port operation, OCnA/OCnB/OCnC disconnected.
0 1 WGM13:0 = 14 or 15: Toggle OC1A on Compare Match, OC1B and OC1C disconnected (normal port operation). For all other WGM1 settings, normal port operation, OC1A/OC1B/OC1C disconnected.
1 0 Clear OCnA/OCnB/OCnC on compare match, set OCnA/OCnB/OCnC at BOTTOM (non-inverting mode).
1 1 Set OCnA/OCnB/OCnC on compare match, clear OCnA/OCnB/OCnC at BOTTOM (inverting mode).
*/
void HardwareTimer::setOutputMode(uint8_t channel, uint8_t outputMode)
{
  uint8_t COMbitMask = 0b11;
  uint8_t shiftCount;

  outputMode &= 0b11;  // only 4 modes

  if (channel == CHANNEL_A)
        shiftCount = COMnA0;
  else if (channel == CHANNEL_B)
        shiftCount = COMnB0;
  else if (channel == CHANNEL_C)
        shiftCount = COMnC0;

  COMbitMask <<= shiftCount;
  outputMode <<= shiftCount;
  
  uint8_t oldSREG = SREG;
  cli();
  *_tccrna = (*_tccrna & ~(COMbitMask)) | outputMode;
  SREG = oldSREG;
}

void HardwareTimer::setOCR(uint8_t channel, uint16_t val)
{
  uint8_t oldSREG = SREG;
  cli();
  uint8_t highByte = val >> 8;
  uint8_t lowByte = val & 0x00ff;
	switch(channel) {
		case CHANNEL_A:
			if (_ocrnah != NULL) {
				*_ocrnah = highByte;
			}
			*_ocrnal = lowByte;
			break;  
		case CHANNEL_B:
			if (_ocrnbh != NULL) {
				*_ocrnbh = highByte;
			}
			*_ocrnbl = lowByte;
			break;
		case CHANNEL_C:
			if (_ocrnch != NULL) {
				*_ocrnch = highByte;
			}
			*_ocrncl = lowByte;
			break;
	}
	SREG = oldSREG;
}

void HardwareTimer::setCounter(uint16_t val)
{
  uint8_t highByte = val >> 8;
  uint8_t lowByte = val & 0x00ff;

  uint8_t oldSREG = SREG;
  cli();
  if (_tcntnh != NULL)  // 16 bit timers
    *_tcntnh = highByte;

  *_tcntnl = lowByte;
  SREG = oldSREG;
}

uint16_t HardwareTimer::getCounter(void)
{
  uint16_t value = 0;
  uint8_t oldSREG = SREG;
  cli();
  if (_tcntnh != NULL)
    value = *_tcntnh << 8;
  value += *_tcntnl;
  SREG = oldSREG;

  return value;
}

void HardwareTimer::waitForOverflow(void) { while( !(*_tifrn & (1<<_tovrn))); }

// Preinstantiate Objects
HardwareTimer Timer0(0);
HardwareTimer Timer1(1);
HardwareTimer Timer2(2);
HardwareTimer Timer3(3);
HardwareTimer Timer4(4);
HardwareTimer Timer5(5);

