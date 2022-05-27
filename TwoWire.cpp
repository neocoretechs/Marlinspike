/*
 * TwoWire.cpp
 * Creates service instances TWIService by channel as needed to maintain transmit and receive buffers
 * for each channel in use.
 * Created: 3/17/2014 12:03:08 AM
 * Author: jg
 */ 

#include <math.h>
#include <stdlib.h>
#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <compat/twi.h>
#include "WString.h"
#include "TwoWire.h"
#include "new.h"

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif

#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

// Initialize Class Variables
TWIService* TwoWire::deviceService[255];
uint8_t TwoWire::numService = 0;

uint8_t TwoWire::twi_state;
uint8_t TwoWire::twi_slarw;
uint8_t TwoWire::twi_init = 0;

uint8_t TwoWire::twi_error = 0;
uint8_t TwoWire::transmitting = 0;
//void (*TwoWire::user_onRequest)(void);
//void (*TwoWire::user_onReceive)(int);

TwoWire::TwoWire()
{
	for(int i = 0; i < 255; i++) deviceService[i] = (TWIService*)NULL;
}

TWIService* TwoWire::getService(uint8_t tdevice) {
	numService = tdevice;
	if( deviceService[numService] != (TWIService*)NULL ) {// failsafe, sort of
			return deviceService[numService];
	}
	deviceService[numService] = new TWIService(numService);
	return deviceService[numService];
}

TWIService* TwoWire::getService() {
	return deviceService[numService];
}

void TwoWire::begin(void)
{
	twi_state = TWI_READY;
	if( !twi_init ) {
		//MCUCR |= (1<<PUD);      // Disable all pull-ups
		// activate internal pull-ups for twi
		// as per note from atmega128 manual pg204
		sbi(PORTD, 0);
		sbi(PORTD, 1);
		// initialize twi prescaler and bit rate
		cbi(TWSR, TWPS0);
		cbi(TWSR, TWPS1);
		TWBR = ((CPU_FREQ / TWI_FREQ) - 16) / 2;
		twi_init = 1;
	}
	/* twi bit rate formula from atmega128 manual pg 204
	SCL Frequency = CPU Clock Frequency / (16 + (2 * TWBR))
	note: TWBR should be 10 or higher for master mode
	It is 72 for a 16mhz Wiring board with 100kHz TWI
	*/
	// enable twi module, acks, and twi interrupt
	TWCR = _BV(TWEN) | _BV(TWINT) | _BV(TWSTA) | _BV(TWIE) ;
}

void TwoWire::begin(uint8_t address)
{
	// set twi slave address (skip over TWGCE bit)
	TWAR = address << 1;
	getService(address)->clearBuffers();
	//twi_attachSlaveTxEvent(onRequestService);
	//twi_attachSlaveRxEvent(onReceiveService);
	begin();
}


uint8_t TwoWire::requestFrom(uint8_t address, uint8_t quantity)
{
	// clamp to buffer length
	if(quantity > TWI_BUFFER_LENGTH){
		quantity = TWI_BUFFER_LENGTH;
	}
	// perform blocking read into buffer
	uint8_t read = twi_readFrom(address, quantity);
	return read;
}


void TwoWire::beginTransmission(uint8_t address)
{
	// indicate that we are transmitting
	transmitting = 1;
	// set address of targeted slave
	begin(address);
}

uint8_t TwoWire::endTransmission(void)
{
	// transmit buffer (blocking)
	int8_t ret = twi_writeTo(deviceService[numService]->device, deviceService[numService]->txBufferLength, true);
	// reset tx buffer iterator vars
	deviceService[numService]->clearTxBuffer();
	// indicate that we are done transmitting
	transmitting = 0;
	return ret;
}

// must be called in:
// slave tx event callback
// or after beginTransmission(address)
void TwoWire::send(uint8_t data)
{
	send(&data,1);
}

// must be called in:
// slave tx event callback
// or after beginTransmission(address)
void TwoWire::send(uint8_t* data, uint8_t quantity)
{
	if(transmitting){
		// in master transmitter mode
		// set length and copy data into tx buffer
		if( !deviceService[numService]->setTxBufferLength(quantity) ) {
			return;
		}
		deviceService[numService]->txBufferIndex = 0;
		for(int i = 0; i < quantity; ++i){
			deviceService[numService]->addTx(*(data+i));
		}
	} else {
		// in slave send mode
		// reply to master
		twi_transmit(data, quantity);
	}
}


// must be called in:
// slave rx event callback
// or after requestFrom(address, numBytes)
uint8_t TwoWire::available(void)
{
	uint8_t value = deviceService[numService]->remainingRcve();
	return value;
}

// must be called in:
// slave rx event callback
// or after requestFrom(address, numBytes)
uint8_t TwoWire::receive(void)
{
	uint8_t value = deviceService[numService]->getRx();
	return value;
}

/* 
 * Function twi_readFrom
 * Desc     attempts to become twi bus master and read a
 *          series of bytes from a device on the bus
 * Input    address: 7bit i2c device address
 *          data: pointer to byte array
 *          length: number of bytes to read into array
 * Output   number of bytes read
 */
uint8_t TwoWire::twi_readFrom(uint8_t address, uint8_t length)
{
  uint8_t i;

  // ensure data will fit into buffer
  if(TWI_BUFFER_LENGTH < length){
    return 0;
  }

  // wait until twi is ready, become master receiver
  while(TWI_READY != twi_state){
    continue;
  }
  //twi_state = TWI_MRX;
  // reset error state (0xFF.. no error occurred)
  twi_error = 0xFF;

  // initialize buffer iteration vars
  TWIService* service = getService(address);
  service->rxBufferIndex = 0;
  service->rxBufferPointer = 0;
  service->rxBufferLength = length;  
  //service->rxBufferLength = length-1;  // This is not intuitive, read on...
  // On receive, the previously configured ACK/NACK setting is transmitted in
  // response to the received byte before the interrupt is signaled. 
  // Therefor we must actually set NACK when the _next_ to last byte is
  // received, causing that NACK to be sent in response to receiving the last
  // expected byte of data.

  // build sla+w, slave device address + w bit
  twi_slarw = TW_READ;
  twi_slarw |= address << 1;

  // send start condition
  TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA) | _BV(TWINT) | _BV(TWSTA);

  // wait for read operation to complete via ISR
  while(TWI_MRX == twi_state){
    continue;
  }  
  return service->rxBufferIndex;
}

/* 
 * Function twi_writeTo
 * Desc     attempts to become twi bus master and write a
 *          series of bytes to a device on the bus
 * Input    address: 7bit i2c device address
 *          data: pointer to byte array
 *          length: number of bytes in array
 *          wait: boolean indicating to wait for write or not
 * Output   0 .. success
 *          1 .. length too long for buffer
 *          2 .. address sent, NACK received
 *          3 .. data sent, NACK received
 *          4 .. other twi error (lost bus arbitration, bus error, ..)
 */
uint8_t TwoWire::twi_writeTo(uint8_t address, uint8_t length, uint8_t wait)
{
  uint8_t i;

  // ensure data will fit into buffer
  if(TWI_BUFFER_LENGTH < length){
    return 1;
  }

  // wait until twi is ready, become master transmitter
  while(TWI_READY != twi_state){
    continue;
  }
 
  // reset error state (0xFF.. no error occured)
  twi_error = 0xFF;
  
  TWIService* service = getService(address);
  // initialize buffer iteration vars
  service->txBufferIndex = 0;
  service->setTxBufferLength(length);
  // build sla+w, slave device address + w bit
  twi_slarw = TW_WRITE;
  twi_slarw |= address << 1;
  
  // send start condition, this should generate interrupt with 'start' status
  TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA) | _BV(TWINT) | _BV(TWSTA);
  // wait for write operation to complete
  while(wait && (TWI_MTX == twi_state)){
    continue;
  }
  
  if (twi_error == 0xFF)
    return 0;   // success
  else if (twi_error == TW_MT_SLA_NACK)
    return 2;   // error: address send, nack received
  else if (twi_error == TW_MT_DATA_NACK)
    return 3;   // error: data send, nack received
  else
    return 4;   // other twi error
}

/* 
 * Function twi_transmit
 * Desc     fills tx buffer with data
 *          designed to operate in 'slave send' mode from 'send' method
 * Input    data: pointer to byte array
 *          length: number of bytes in array
 * Output   1 length too long for buffer
 *          2 not slave transmitter
 *          0 ok
 */
uint8_t TwoWire::twi_transmit(uint8_t* data, uint8_t length)
{
  uint8_t i;

  // ensure data will fit into buffer
  if(TWI_BUFFER_LENGTH < length){
    return 1;
  }
  
  // ensure we are currently a slave transmitter
  if(TWI_STX != twi_state){
    return 2;
  }
  
  // set length and copy data into tx buffer
  deviceService[numService]->setTxBufferLength(length);
  deviceService[numService]->txBufferIndex = 0;
  
  for(i = 0; i < length; ++i){
    deviceService[numService]->addTx(*(data+i));
  }
  
  return 0;
}


/* 
 * Function twi_reply
 * Desc     sends byte or readies receive line
 * Input    ack: byte indicating to ack or to nack
 * Output   none
 */
void TwoWire::twi_reply(uint8_t ack)
{
  // transmit master read ready signal, with or without ack
  if(ack) {
    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWINT) | _BV(TWEA);
  } else {
          TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWINT);
  }
}

/* 
 * Function twi_stop
 * Desc     relinquishes bus master status
 * Input    none
 * Output   none
 */
void TwoWire::twi_stop(void)
{
  // send stop condition
  TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA) | _BV(TWINT) | _BV(TWSTO);

  // wait for stop condition to be executed on bus
  // TWINT is not set after a stop condition!
  while(TWCR & _BV(TWSTO)) {
    continue;
  }

  // update twi state
  twi_state = TWI_READY;
}

/* 
 * Function twi_releaseBus
 * Desc     releases bus control
 * Input    none
 * Output   none
 */
void TwoWire::twi_releaseBus(void)
{
  // release bus
  TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA) | _BV(TWINT);

  // update twi state
  twi_state = TWI_READY;
}

ISR(TWI_vect)
{
  //TWIService* service = Wire.getService(TWAR);
  TWIService* service = Wire.getService();
  switch(TW_STATUS){
    // All Master
    case TW_START:     // sent start condition
    case TW_REP_START: // sent repeated start condition
      // copy device address and r/w bit to output register and ack
      TWDR = Wire.twi_slarw;
      Wire.twi_reply(1);
      break; 
	   
    // Master Transmitter
    case TW_MT_SLA_ACK:  // slave receiver acked address
	  Wire.twi_state = TWI_MTX;
    case TW_MT_DATA_ACK: // slave receiver acked data
      // if there is data to send, send it, otherwise stop 
      if(service->txBufferIndex < service->txBufferLength) {
        // copy data to output register and ack
        TWDR = service->getTx();
        Wire.twi_reply(1);
      } else {
        Wire.twi_stop();
      }
      break;
    case TW_MT_SLA_NACK:  // address sent, nack received
      Wire.twi_error = TW_MT_SLA_NACK;
      Wire.twi_stop();
      break;
    case TW_MT_DATA_NACK: // data sent, nack received
      Wire.twi_error = TW_MT_DATA_NACK;
      Wire.twi_stop();
      break;
    case TW_MT_ARB_LOST: // lost bus arbitration
      Wire.twi_error = TW_MT_ARB_LOST;
      Wire.twi_releaseBus();
      break;
	  
    // Master Receiver
	case TW_MR_SLA_ACK:  // address sent, ack received
	  Wire.twi_state = TWI_MRX;
    case TW_MR_DATA_ACK: // data received, ack sent
      // put byte into buffer
      service->addRx(TWDR);
      // ack if more bytes are expected, otherwise nack
      if(service->rxBufferIndex < service->rxBufferLength){
        Wire.twi_reply(1);
      } else {
        Wire.twi_reply(0);
      }
      break;
    case TW_MR_DATA_NACK: // data received, nack sent
      // put final byte into buffer
      service->addRx(TWDR);
    case TW_MR_SLA_NACK: // address sent, nack received
      Wire.twi_stop();
      break;
	  
    // TW_MR_ARB_LOST handled by TW_MT_ARB_LOST case
    // Slave Receiver
    case TW_SR_SLA_ACK:   // addressed, returned ack
    case TW_SR_GCALL_ACK: // addressed generally, returned ack
    case TW_SR_ARB_LOST_SLA_ACK:   // lost arbitration, returned ack
    case TW_SR_ARB_LOST_GCALL_ACK: // lost arbitration, returned ack
      // enter slave receiver mode
      Wire.twi_state = TWI_SRX;
      // indicate that rx buffer can be overwritten and ack
      service->clearRxBuffer();
      Wire.twi_reply(1);
      break;
    case TW_SR_DATA_ACK:       // data received, returned ack
    case TW_SR_GCALL_DATA_ACK: // data received generally, returned ack
      // if there is still room in the rx buffer
      if(service->rxBufferIndex < TWI_BUFFER_LENGTH){
        // put byte in buffer and ack
        service->addRx(TWDR);
        Wire.twi_reply(1);
      } else {
        // otherwise nack
        Wire.twi_reply(0);
      }
      break;
    case TW_SR_STOP: // stop or repeated start condition received
      // sends ack and stops interface for clock stretching
      Wire.twi_stop();
      // callback to user defined service
      service->onReceive();
      //
      //service->clearRxBuffer();
      // ack future responses and leave slave receiver state
      Wire.twi_releaseBus();
      break;
    case TW_SR_DATA_NACK:       // data received, returned nack
    case TW_SR_GCALL_DATA_NACK: // data received generally, returned nack
      // nack back at master
      Wire.twi_reply(0);
      break;
	  
    // Slave Transmitter
    case TW_ST_SLA_ACK:          // addressed, returned ack
    case TW_ST_ARB_LOST_SLA_ACK: // arbitration lost, returned ack
      // enter slave transmitter mode
      Wire.twi_state = TWI_STX;
      // transmit first byte from buffer, fall
    case TW_ST_DATA_ACK: // byte sent, ack returned
      service->onTransmit();
	  // copy data to output register
      TWDR = service->getTx();
      // if there is more to send, ack, otherwise nack
      if(service->txBufferIndex < service->txBufferLength){
        Wire.twi_reply(1);
      } else {
        Wire.twi_reply(0);
      }
      break;
    case TW_ST_DATA_NACK: // received nack, we are done 
    case TW_ST_LAST_DATA: // received ack, but we are done already!
      // ack future responses
      Wire.twi_reply(1);
      // leave slave receiver state
      Wire.twi_state = TWI_READY;
      break;
    // All
    case TW_NO_INFO:   // no state information
      break;
    case TW_BUS_ERROR: // bus error, illegal stop/start
      Wire.twi_error = TW_BUS_ERROR;
      Wire.twi_stop();
      break;
  }
}
// Preinstantiate Objects //////////////////////////////////////////////////////

TwoWire Wire = TwoWire();