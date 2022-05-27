/* 
* HBridgeDriver.h
* Generic driver for a collection of H bridge driven brushed DC motor channels.
* Structure:
* 1) Top level, a master list of pins in either the PWM or digitals arrays. Slots to which physical pins are assigned.
* these are ppwms and pdigitals here, which hold pointers to these top level arrays.
* 2) In each class and subclass, a sublist of pins. Multidimensional arrays which hold the index to the top level pins.
* these are indexed by channel. motorDrive here. All PWM has a pin, a prescale, and a resolution. We standardize the resolution to 8 bits typically.
* 3) A PWM level indexed by channel. The current power or level value of the PWM timer. Standardized to reflect the control applied.
* for instance, if the PS3 controller delivers -1000 to 1000 thats the range. If the controller is using the button that has -1 off and a max of 1000
* then the range is 0 to 2000 to reflect the fact that no reverse operation is going to be in effect for something like a LED or pump.
* 4) Optionally, a duration and a minimum PWM level, or here a minimum motor power. The duration represents a maximum interval before the PWM timer
* is automatically shut down. In this case, the hall effect interrupts that are serviced by an interrupt handler that detect wheel rotation.
* The minimum PWM level is the bottom limit for the PWM value, or in this case the minimum motor power. It is indexed by channel and the value is
* the range that comes from the main controller, before any processing into a timer value.
* the top level abstract class AbstractMotorControl contains these values.
* Created: 10/2/2016 1:42:24 PM
* Author: jg
*/


#ifndef __HBRIDGEDRIVER_H__
#define __HBRIDGEDRIVER_H__
#include "AbstractMotorControl.h"
#include "AbstractPWMMotorControl.h"
#include "../pins.h"
#include <stdio.h>

class HBridgeDriver : public AbstractPWMMotorControl
{
//variables
public:
protected:
	PWM** ppwms;
	Digital** pdigitals;
	// 10 possible drive wheels, index is by channel-1. 
	// motorDrive[channel] [[PWM array index][dir pin][timer prescale][timer resolution]
	// PWM params array by channel:
	// 0-pin index to pwm array(default 255)
	// 1-direction pin
	// 2-timer prescale (1-none) 
	// 3-timer resolution (8 bit)
	uint8_t motorDrive[10][4]={{255,0,1,8},{255,0,1,8},{255,0,1,8},{255,0,1,8},{255,0,1,8},{255,0,1,8},{255,0,1,8},{255,0,1,8},{255,0,1,8},{255,0,1,8}};
	int status_flag = 0;
//functions
private:
public:
	HBridgeDriver(void) : AbstractPWMMotorControl(255){};
	~HBridgeDriver();
	void resetMaxMotorPower() { MAXMOTORPOWER = 255; }
	int commandMotorPower(uint8_t ch, int16_t p);
	int commandEmergencyStop(int status);
	int isConnected(void) { return true; }
	void setMotors(PWM** pwm) { ppwms = pwm; }
	void setDirectionPins(Digital** dpin) { pdigitals = dpin; }
	uint8_t getMotorPWMPin(uint8_t channel) { return motorDrive[channel-1][0]; }
	uint8_t getMotorEnablePin(uint8_t channel) {return motorDrive[channel-1][1]; }
	void createPWM(uint8_t channel, uint8_t pin_number, uint8_t dir_pin, uint8_t dir_default, int timer_pre, int timer_res);
	void getDriverInfo(uint8_t ch, char* outStr);
	int queryFaultFlag(void) { return fault_flag; }
    int queryStatusFlag(void) { return status_flag; }
protected:
private:
	HBridgeDriver( const HBridgeDriver &c );
	HBridgeDriver& operator=( const HBridgeDriver &c );

}; //HBridgeDriver
//extern HBridgeDriver hBridgeDriver;
#endif //__HBRIDGEDRIVER_H__
