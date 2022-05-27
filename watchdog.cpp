/*
* Watchdog timer
*/

#include "watchdog.h"
extern InterruptService* WatchdogTimer::dogservice;
/// initialize watch dog with an interrupt time
//  Possible timeout values are: 15 ms, 30 ms, 60 ms, 120 ms, 250 ms,
//  500 ms, 1 s, 2 s.  (Some devices also allow for 4 s and 8 s.)
void WatchdogTimer::watchdog_init(uint16_t timeout)
{
	switch(timeout) {
		case 15:
			wdt_enable(WDTO_15MS);
			break;
		case 30:
			wdt_enable(WDTO_30MS);
			break;
		case 60:
			wdt_enable(WDTO_60MS);
			break;
		case 120:
			wdt_enable(WDTO_120MS);
			break;
		case 250:
			wdt_enable(WDTO_250MS);
			break;
		case 500:
			wdt_enable(WDTO_500MS);
			break;
		case 1000:
			wdt_enable(WDTO_1S);
			break;
		case 2000:
			wdt_enable(WDTO_2S);
			break;
		case 4000:
			wdt_enable(WDTO_4S);
			break;
		case 8000:
			wdt_enable(WDTO_8S);
			break;
		default:
			wdt_enable(WDTO_4S);
			break;
	}
}

/// initialize watch dog with an interrupt time and an interrupt service handler
//  Possible timeout values are: 15 ms, 30 ms, 60 ms, 120 ms, 250 ms,
//  500 ms, 1 s, 2 s.  (Some devices also allow for 4 s and 8 s.)
void WatchdogTimer::watchdog_init(uint16_t timeout, InterruptService* servix)
{
	WatchdogTimer::dogservice = servix;
	//We enable the watchdog timer, but only for the interrupt.
	//Take care, as this requires the correct order of operation, with interrupts disabled. 
	//See the datasheet of any AVR chip for details.
	wdt_reset();
	_WD_CONTROL_REG = _BV(_WD_CHANGE_BIT) | _BV(WDE);
	switch(timeout) {
		case 15:
			_WD_CONTROL_REG = _BV(WDIE) | WDTO_15MS;
			break;
		case 30:
			_WD_CONTROL_REG = _BV(WDIE) | WDTO_30MS;
			break;
		case 60:
			_WD_CONTROL_REG = _BV(WDIE) | WDTO_60MS;
			break;
		case 120:
			_WD_CONTROL_REG = _BV(WDIE) | WDTO_120MS;
			break;
		case 250:
			_WD_CONTROL_REG = _BV(WDIE) | WDTO_250MS;
			break;
		case 500:
			_WD_CONTROL_REG = _BV(WDIE) | WDTO_500MS;
			break;
		case 1000:
			_WD_CONTROL_REG = _BV(WDIE) | WDTO_1S;
			break;
		case 2000:
			_WD_CONTROL_REG = _BV(WDIE) | WDTO_2S;
			break;
		case 4000:
			_WD_CONTROL_REG = _BV(WDIE) | WDTO_4S;
			break;
		case 8000:
			_WD_CONTROL_REG = _BV(WDIE) | WDTO_8S;
			break;
		default:
			_WD_CONTROL_REG = _BV(WDIE) | WDTO_4S;
			break;
	}
	
}

/// reset watchdog. MUST be called every 1s after init or avr will reset.
void WatchdogTimer::watchdog_reset() 
{
    wdt_reset();
}

//Watchdog timer interrupt, called if main program blocks >1sec and manual reset is enabled.

ISR(WDT_vect)
{ 
  WatchdogTimer::dogservice->service();  
}

