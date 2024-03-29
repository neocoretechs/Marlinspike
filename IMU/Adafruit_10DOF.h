/*
 * Adafruit_10DOF.h
 *
 * Created: 4/20/2014 8:27:37 PM
 *  Author: jg
 
  This is a library for the Adafruit 10DOF Breakout

  Designed specifically to work with the Adafruit 10DOF Breakout

  These displays use I2C to communicate, 2 pins are required to interface.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit andopen-source hardware by purchasing products
  from Adafruit!

  Written by Kevin Townsend for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution
 ***************************************************************************/
#ifndef __ADAFRUIT_10DOF_H__
#define __ADAFRUIT_10DOF_H__

#include "../Arduino.h"

#include "Adafruit_Sensor.h"
#include "Adafruit_LSM303U.h"
#include "Adafruit_BMP085U.h"
#include "Adafruit_L3GD20U.h"
#include "../TwoWire.h"

/** Sensor axis */
typedef enum
{
  SENSOR_AXIS_X  = (1),
  SENSOR_AXIS_Y  = (2),
  SENSOR_AXIS_Z  = (3)
} sensors_axis_t;

/* Driver for the the 10DOF breakout sensors */
class Adafruit_10DOF
{
  public:
    Adafruit_10DOF(void);
    bool begin(void);
    
    bool  accelGetOrientation  ( sensors_event_t *event, sensors_vec_t *orientation );
    bool  magTiltCompensation  ( sensors_axis_t axis, sensors_event_t *mag_event, sensors_event_t *accel_event );
    bool  magGetOrientation    ( sensors_axis_t axis, sensors_event_t *event, sensors_vec_t *mag_orientation );
    bool  fusionGetOrientation ( sensors_event_t *accel_event, sensors_event_t *mag_event, sensors_vec_t *orientation );

  private:
};

#endif /* ADAFRUIT_10DOF.H_H_ */