/* 
* AbstractPWMMotorControl.h
*
* Created: 10/31/2020 12:57:21 PM
* Author: Groff
*/


#ifndef __ABSTRACTPWMMOTORCONTROL_H__
#define __ABSTRACTPWMMOTORCONTROL_H__


class AbstractPWMMotorControl : public AbstractMotorControl
{
//functions
public:
	AbstractPWMMotorControl(uint32_t maxPower) {MAXMOTORPOWER = maxPower;}
	~AbstractPWMMotorControl(){};
	void setMinMotorPower(uint8_t ch, uint32_t mpow) { minMotorPower[ch-1] = abs(mpow)/4; }
	void setMaxMotorPower(int p) { MAXMOTORPOWER = abs(p)/4; }
	void setMotorPowerScale(int p) { MOTORPOWERSCALE = abs(p)/4;}
	virtual void resetMaxMotorPower()=0;//set back to the maximum power, subclass sets

}; //AbstractPWMMotorControl

#endif //__ABSTRACTPWMMOTORCONTROL_H__
