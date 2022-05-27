/*
 * WPCInterrupts.cpp
 *
 * Created: 3/12/2014 3:17:27 AM
 *  Author: jg
 */
#include "WPCInterrupts.h" 
uint8_t PCInterrupts::attachInterrupt(InterruptService* userFunc, int mode) { return 0; }
/*
 * attach an interrupt to a specific pin using pin change interrupts.
 * mode is CHANGE, RISING, FALLING
 */
 void PCInterrupts::attachInterrupt(uint8_t pin, InterruptService* userFunc, int mode) {
  this->pin = pin;
  uint8_t bit = digitalPinToBitMask(pin);
  uint8_t port = digitalPinToPort(pin);
  uint8_t slot;
  volatile uint16_t *pcmask = digitalPinToPCMSK(pin);
  uint8_t pcslot = digitalPinToPCMSKbit(pin);
    // map pin to PCIR register
  if (pcmask == ((uint16_t*)0)) { //not a valid pin for PCINT
    return;
  } 
  uint8_t sport;
  if( pcmask == &PCMSK0 ) {// PCInt 7:0
	sport = 1;
	slot = pcslot;
	PCintLast[0] = *portInputRegister(port);
  } else {
	if( pcmask == &PCMSK1 ) {// PCInt 15:8
		sport = 2;
		slot = 8 + pcslot;
		PCintLast[1] = *portInputRegister(port);
	} else {
		sport = 4; // PCInt 23:16
		slot = 16 + pcslot;
		PCintLast[2] = *portInputRegister(port);
	}
  }

  //for(uint8_t j = 0; j < 3; j++)
	//if( pcmask == PCmsk[j] ) { 
	//	slot = (j * 8) + pcslot; // 3 registers, 8 bits each, more or less...Arduino doesnt pin them all out, grr
	//	PCintLast[j] = *PCport[j];
	//}

  PCintMode[slot] = mode;
  PCintFunc[slot] = userFunc;
  // set the mask
  *pcmask |= bit;
  // enable the interrupt, port bits are 0,1,2 (1,2,4) for PCMSKn
 
  sei();
  PCICR |= sport;
}

void PCInterrupts::detachInterrupt(uint8_t pin) {
  uint8_t bit = digitalPinToBitMask(pin);
  uint8_t port = digitalPinToPort(pin);
  volatile uint16_t *pcmask = digitalPinToPCMSK(pin);

  // map pin to PCIR register
  if (port == NOT_A_PORT) {
    return;
  } 

  // disable the mask.
  *pcmask &= ~bit;
  // if that's the last one, disable the interrupt.
  if (*pcmask == 0) {
    PCICR &= ~(0x01 << port);
  }
}

// common code for isr handler. "posi" is the PCINT number.
static void PCint(uint8_t posi) {
  uint8_t bit;
  uint8_t curr;
  uint8_t mask;
  uint8_t pin;

  // get the pin states for the indicated port.
  curr = *(PCport[posi]);
  //uint8_t port = digitalPinToPort(pin);
  //curr = *portInputRegister(port);
  mask = curr ^ PCintLast[posi];
  PCintLast[posi] = curr;
  // mask is pins that have changed. screen out non pcint pins.
  if ((mask &= *PCmsk[posi]) == 0) {
    return;
  }
  // mask is pcint pins that have changed.
  for (uint8_t i=0; i < 8; i++) {
    bit = 0x01 << i;
    if (bit & mask) {
      pin = (posi * 8) + i;
      // Trigger interrupt if mode is CHANGE, or if mode is RISING and
      // the bit is currently high, or if mode is FALLING and bit is low.
      if ((PCintMode[pin] == CHANGE || ((PCintMode[pin] == RISING) && (curr & bit)) || ((PCintMode[pin] == FALLING) && !(curr & bit)))) {
        PCintFunc[pin]->service();
      }
    }
  }
}

ISR(PCINT0_vect) {
  PCint(0);
}
ISR(PCINT1_vect) {
  PCint(1);
}
ISR(PCINT2_vect) {
  PCint(2);
}