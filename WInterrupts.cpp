
/*
* Groff 4/2014
* General interrupt handler
*/

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>

#include "WInterrupts.h"
#include "WInterruptService.h"

InterruptService* services[EXTERNAL_NUM_INTERRUPTS];
ISR(INT0_vect) {
	services[EXTERNAL_INT_0]->service();
}

ISR(INT1_vect) {
	services[EXTERNAL_INT_1]->service();
}

ISR(INT2_vect) {
	services[EXTERNAL_INT_2]->service();
}

ISR(INT3_vect) {
	services[EXTERNAL_INT_3]->service();
}

ISR(INT4_vect) {
	services[EXTERNAL_INT_4]->service();
}

ISR(INT5_vect) {
	services[EXTERNAL_INT_5]->service();
}

ISR(INT6_vect) {
	services[EXTERNAL_INT_6]->service();
}

ISR(INT7_vect) {
	services[EXTERNAL_INT_7]->service();
}

	uint8_t Interrupts::attachInterrupt(InterruptService* userFunc, int mode) {
		attachInterrupt(highInt++, userFunc, mode);
	}

	void Interrupts::attachInterrupt(uint8_t interruptNum, InterruptService* userFunc, int mode) {
		if(interruptNum < EXTERNAL_NUM_INTERRUPTS) {
		services[interruptNum] = userFunc;
    
		// Configure the interrupt mode (trigger on low input, any change, rising
		// edge, or falling edge).  The mode constants were chosen to correspond
		// to the configuration bits in the hardware register, so we simply shift
		// the mode into place.
      
		// Enable the interrupt.
      
		switch (interruptNum) {
			case EXTERNAL_INT_4:
			EICRB = (EICRB & ~((1 << ISC40) | (1 << ISC41))) | (mode << ISC40);
			EIMSK |= (1 << INT4);
			break;
			case EXTERNAL_INT_5:
			EICRB = (EICRB & ~((1 << ISC50) | (1 << ISC51))) | (mode << ISC50);
			EIMSK |= (1 << INT5);
			break;
			case EXTERNAL_INT_0:
			EICRA = (EICRA & ~((1 << ISC00) | (1 << ISC01))) | (mode << ISC00);
			EIMSK |= (1 << INT0);
			break;
			case EXTERNAL_INT_1:
			EICRA = (EICRA & ~((1 << ISC10) | (1 << ISC11))) | (mode << ISC10);
			EIMSK |= (1 << INT1);
			break;
			case EXTERNAL_INT_2:
			EICRA = (EICRA & ~((1 << ISC20) | (1 << ISC21))) | (mode << ISC20);
			EIMSK |= (1 << INT2);
			break;
			case EXTERNAL_INT_3:
			EICRA = (EICRA & ~((1 << ISC30) | (1 << ISC31))) | (mode << ISC30);
			EIMSK |= (1 << INT3);
			break;
			case EXTERNAL_INT_6:
			EICRB = (EICRB & ~((1 << ISC60) | (1 << ISC61))) | (mode << ISC60);
			EIMSK |= (1 << INT6);
			break;
			case EXTERNAL_INT_7:
			EICRB = (EICRB & ~((1 << ISC70) | (1 << ISC71))) | (mode << ISC70);
			EIMSK |= (1 << INT7);
			break;
		}
		}
	}


	void Interrupts::detachInterrupt(uint8_t interruptNum) {
	if(interruptNum < EXTERNAL_NUM_INTERRUPTS) {
    // Disable the interrupt. 
    switch (interruptNum) {
	case EXTERNAL_INT_4:
		EIMSK &= ~(1 << INT4);
		break;
	case EXTERNAL_INT_5:
		EIMSK &= ~(1 << INT5);
		break;
	case EXTERNAL_INT_0:
		EIMSK &= ~(1 << INT0);
      break;
    case EXTERNAL_INT_1:
      EIMSK &= ~(1 << INT1);
      break;
    case EXTERNAL_INT_2:
      EIMSK &= ~(1 << INT2);
      break;
    case EXTERNAL_INT_3:
      EIMSK &= ~(1 << INT3);
      break;
    case EXTERNAL_INT_6:
      EIMSK &= ~(1 << INT6);
      break;
    case EXTERNAL_INT_7:
      EIMSK &= ~(1 << INT7);
      break;
    }
  }
}


