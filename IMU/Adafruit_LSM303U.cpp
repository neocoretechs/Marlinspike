
/***************************************************************************
  This is a library for the LSM303 Accelerometer and magnetometer/compass

  Designed specifically to work with the Adafruit LSM303DLHC Breakout

  These displays use I2C to communicate, 2 pins are required to interface.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit andopen-source hardware by purchasing products
  from Adafruit!

  Written by Kevin Townsend for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution
 ***************************************************************************/
#include "../Arduino.h"

#include "../TwoWire.h"
#include <limits.h>
#include "Adafruit_LSM303U.h"

static float _lsm303Accel_MG_LSB     = 0.001F;   // 1, 2, 4 or 12 mg per lsb
static float _lsm303Mag_Gauss_LSB_XY = 1100.0F;  // Varies with gain
static float _lsm303Mag_Gauss_LSB_Z  = 980.0F;   // Varies with gain

/***************************************************************************
 ACCELEROMETER
 ***************************************************************************/


/**************************************************************************/
/*!
    @brief  Reads the raw data from the sensor
*/
/**************************************************************************/
void Adafruit_LSM303_Accel_Unified::read()
{
  // Read the accelerometer
  
    uint8_t xlo = read8((byte)LSM303_ADDRESS_ACCEL,(byte)LSM303_REGISTER_ACCEL_OUT_X_L_A) | 0x80; //s two compliment
    uint8_t xhi = read8((byte)LSM303_ADDRESS_ACCEL,(byte)LSM303_REGISTER_ACCEL_OUT_X_H_A) | 0x80;
    uint8_t ylo = read8((byte)LSM303_ADDRESS_ACCEL,(byte)LSM303_REGISTER_ACCEL_OUT_Y_L_A) | 0x80;
    uint8_t yhi = read8((byte)LSM303_ADDRESS_ACCEL,(byte)LSM303_REGISTER_ACCEL_OUT_Y_H_A) | 0x80;
    uint8_t zlo = read8((byte)LSM303_ADDRESS_ACCEL,(byte)LSM303_REGISTER_ACCEL_OUT_Z_L_A) | 0x80;
    uint8_t zhi = read8((byte)LSM303_ADDRESS_ACCEL,(byte)LSM303_REGISTER_ACCEL_OUT_Z_H_A) | 0x80; 
	 
  // Shift values to create properly formed integer (low byte first)
  _accelData.x = (int16_t)(xlo | (xhi << 8)) >> 4;
  _accelData.y = (int16_t)(ylo | (yhi << 8)) >> 4;
  _accelData.z = (int16_t)(zlo | (zhi << 8)) >> 4;
}

 
/**************************************************************************/
/*!
    @brief  Instantiates a new Adafruit_LSM303 class
*/
/**************************************************************************/
Adafruit_LSM303_Accel_Unified::Adafruit_LSM303_Accel_Unified(int32_t sensorID) {
  _sensorID = sensorID;
}


/**************************************************************************/
/*!
    @brief  Setups the HW
*/
/**************************************************************************/
bool Adafruit_LSM303_Accel_Unified::begin()
{
  // Enable I2C
  Wire.begin();

  // Enable the accelerometer
  // 0x57 = 0b01010111
  // AODR = 0101 (100 Hz ODR); AZEN = AYEN = AXEN = 1 (all axes enabled)
  // 0x23 = 1hz
  // 0x39 = 10hz
  // 0x47 = 50hz
  write8(LSM303_ADDRESS_ACCEL, LSM303_REGISTER_ACCEL_CTRL_REG1_A, 0x39);
  //write8(LSM303_ADDRESS_ACCEL, LSM303_REGISTER_ACCEL_CTRL_REG3_A, 0xFE); // enable all interrupts
  // set gauss scale
  write8(LSM303_ADDRESS_ACCEL, LSM303_REGISTER_ACCEL_CTRL_REG6_A, 0xE0);
  return true;
}

/**************************************************************************/
/*! 
    @brief  Gets the most recent sensor event
*/
/**************************************************************************/
void Adafruit_LSM303_Accel_Unified::getEvent(sensors_event_t *event) {
  /* Clear the event */
  memset(event, 0, sizeof(sensors_event_t));
  
  /* Read new data */
  read();

  event->version   = sizeof(sensors_event_t);
  event->sensor_id = _sensorID;
  event->type      = SENSOR_TYPE_ACCELEROMETER;
  event->timestamp = 0;//millis();
  event->acceleration.x = _accelData.x * _lsm303Accel_MG_LSB * SENSORS_GRAVITY_STANDARD;
  event->acceleration.y = _accelData.y * _lsm303Accel_MG_LSB * SENSORS_GRAVITY_STANDARD;
  event->acceleration.z = _accelData.z * _lsm303Accel_MG_LSB * SENSORS_GRAVITY_STANDARD;
}

/**************************************************************************/
/*! 
    @brief  Gets the sensor_t data
*/
/**************************************************************************/
void Adafruit_LSM303_Accel_Unified::getSensor(sensor_t *sensor) {
  /* Clear the sensor_t object */
  memset(sensor, 0, sizeof(sensor_t));

  /* Insert the sensor name in the fixed length char array */
  strncpy (sensor->name, "LSM303", sizeof(sensor->name) - 1);
  sensor->name[sizeof(sensor->name)- 1] = 0;
  sensor->version     = 1;
  sensor->sensor_id   = _sensorID;
  sensor->type        = SENSOR_TYPE_ACCELEROMETER;
  sensor->min_delay   = 0;
  sensor->max_value   = 0.0F; // TBD
  sensor->min_value   = 0.0F; // TBD
  sensor->resolution  = 0.0F; // TBD
}

/***************************************************************************
 MAGNETOMETER
 ***************************************************************************/
/**************************************************************************/
/*!
    @brief  Reads the raw data from the sensor
*/
/**************************************************************************/
void Adafruit_LSM303_Mag_Unified::read()
{
  // Read the magnetometer  
    uint8_t xlo = read8((byte)LSM303_ADDRESS_MAG,(byte)LSM303_REGISTER_MAG_OUT_X_L_M) | 0x80; //s two compliment
    uint8_t xhi = read8((byte)LSM303_ADDRESS_MAG,(byte)LSM303_REGISTER_MAG_OUT_X_H_M) | 0x80;
    uint8_t ylo = read8((byte)LSM303_ADDRESS_MAG,(byte)LSM303_REGISTER_MAG_OUT_Y_L_M) | 0x80;
    uint8_t yhi = read8((byte)LSM303_ADDRESS_MAG,(byte)LSM303_REGISTER_MAG_OUT_Y_H_M) | 0x80;
    uint8_t zlo = read8((byte)LSM303_ADDRESS_MAG,(byte)LSM303_REGISTER_MAG_OUT_Z_L_M) | 0x80;
    uint8_t zhi = read8((byte)LSM303_ADDRESS_MAG,(byte)LSM303_REGISTER_MAG_OUT_Z_H_M) | 0x80; 
	  
  // Shift values to create properly formed integer (low byte first)
  _magData.x = (int16_t)(xlo | ((int16_t)xhi << 8));
  _magData.y = (int16_t)(ylo | ((int16_t)yhi << 8));
  _magData.z = (int16_t)(zlo | ((int16_t)zhi << 8));
  
  // ToDo: Calculate orientation
  _magData.orientation = 0.0;
}

/***************************************************************************
 CONSTRUCTOR
 ***************************************************************************/
 
/**************************************************************************/
/*!
    @brief  Instantiates a new Adafruit_LSM303 class
*/
/**************************************************************************/
Adafruit_LSM303_Mag_Unified::Adafruit_LSM303_Mag_Unified(int32_t sensorID) {
  _sensorID = sensorID;
  _autoRangeEnabled = false;
}

/***************************************************************************
 PUBLIC FUNCTIONS
 ***************************************************************************/
 
/**************************************************************************/
/*!
    @brief  Setups the HW
*/
/**************************************************************************/
bool Adafruit_LSM303_Mag_Unified::begin()
{
  // Enable I2C
  Wire.begin(LSM303_ADDRESS_MAG);

  // Enable the magnetometer
  write8(LSM303_ADDRESS_MAG, LSM303_REGISTER_MAG_MR_REG_M, 0x00);
  
  // Set the gain to a known level
  setMagGain(LSM303_MAGGAIN_1_3);

  return true;
}

/**************************************************************************/
/*! 
    @brief  Enables or disables auto-ranging
*/
/**************************************************************************/
void Adafruit_LSM303_Mag_Unified::enableAutoRange(bool enabled)
{
  _autoRangeEnabled = enabled;
}

/**************************************************************************/
/*!
    @brief  Sets the magnetometer's gain
*/
/**************************************************************************/
void Adafruit_LSM303_Mag_Unified::setMagGain(lsm303MagGain gain)
{
  write8(LSM303_ADDRESS_MAG, LSM303_REGISTER_MAG_CRB_REG_M, (byte)gain);
  
  _magGain = gain;
 
  switch(gain)
  {
    case LSM303_MAGGAIN_1_3:
      _lsm303Mag_Gauss_LSB_XY = 1100;
      _lsm303Mag_Gauss_LSB_Z  = 980;
      break;
    case LSM303_MAGGAIN_1_9:
      _lsm303Mag_Gauss_LSB_XY = 855;
      _lsm303Mag_Gauss_LSB_Z  = 760;
      break;
    case LSM303_MAGGAIN_2_5:
      _lsm303Mag_Gauss_LSB_XY = 670;
      _lsm303Mag_Gauss_LSB_Z  = 600;
      break;
    case LSM303_MAGGAIN_4_0:
      _lsm303Mag_Gauss_LSB_XY = 450;
      _lsm303Mag_Gauss_LSB_Z  = 400;
      break;
    case LSM303_MAGGAIN_4_7:
      _lsm303Mag_Gauss_LSB_XY = 400;
      _lsm303Mag_Gauss_LSB_Z  = 255;
      break;
    case LSM303_MAGGAIN_5_6:
      _lsm303Mag_Gauss_LSB_XY = 330;
      _lsm303Mag_Gauss_LSB_Z  = 295;
      break;
    case LSM303_MAGGAIN_8_1:
      _lsm303Mag_Gauss_LSB_XY = 230;
      _lsm303Mag_Gauss_LSB_Z  = 205;
      break;
  } 
}

/**************************************************************************/
/*! 
    @brief  Gets the most recent sensor event
*/
/**************************************************************************/
void Adafruit_LSM303_Mag_Unified::getEvent(sensors_event_t *event) {
  bool readingValid = false;
  
  /* Clear the event */
  memset(event, 0, sizeof(sensors_event_t));
  
  while(!readingValid)
  {
    /* Read new data */
    read();
    
    /* Make sure the sensor isn't saturating if auto-ranging is enabled */
    if (!_autoRangeEnabled)
    {
      readingValid = true;
    }
    else
    {
      //Serial.print(_magData.x); Serial.print(" ");
      //Serial.print(_magData.y); Serial.print(" ");
      //Serial.print(_magData.z); Serial.println(" ");
      /* Check if the sensor is saturating or not */
      if ( (_magData.x >= 4090) | (_magData.x <= -4090) | 
           (_magData.y >= 4090) | (_magData.y <= -4090) | 
           (_magData.z >= 4090) | (_magData.z <= -4090) )
      {
        /* Saturating .... increase the range if we can */
        switch(_magGain)
        {
          case LSM303_MAGGAIN_5_6:
            setMagGain(LSM303_MAGGAIN_8_1);
            readingValid = false;
            //Serial.println("Changing range to +/- 8.1");
            break;
          case LSM303_MAGGAIN_4_7:
            setMagGain(LSM303_MAGGAIN_5_6);
            readingValid = false;
            //Serial.println("Changing range to +/- 5.6");
            break;
          case LSM303_MAGGAIN_4_0:
            setMagGain(LSM303_MAGGAIN_4_7);
            readingValid = false;
            //Serial.println("Changing range to +/- 4.7");
            break;
          case LSM303_MAGGAIN_2_5:
            setMagGain(LSM303_MAGGAIN_4_0);
            readingValid = false;
            //Serial.println("Changing range to +/- 4.0");
            break;
          case LSM303_MAGGAIN_1_9:
            setMagGain(LSM303_MAGGAIN_2_5);
            readingValid = false;
            //Serial.println("Changing range to +/- 2.5");
            break;
          case LSM303_MAGGAIN_1_3:
            setMagGain(LSM303_MAGGAIN_1_9);
            readingValid = false;
            //Serial.println("Changing range to +/- 1.9");
            break;
          default:
            readingValid = true;
            break;  
        }
      }
      else
      {
        /* All values are withing range */
        readingValid = true;
      }
    }
  }
  
  event->version   = sizeof(sensors_event_t);
  event->sensor_id = _sensorID;
  event->type      = SENSOR_TYPE_MAGNETIC_FIELD;
  event->timestamp = 0;//millis();
  event->magnetic.x = _magData.x / _lsm303Mag_Gauss_LSB_XY * SENSORS_GAUSS_TO_MICROTESLA;
  event->magnetic.y = _magData.y / _lsm303Mag_Gauss_LSB_XY * SENSORS_GAUSS_TO_MICROTESLA;
  event->magnetic.z = _magData.z / _lsm303Mag_Gauss_LSB_Z * SENSORS_GAUSS_TO_MICROTESLA;
}

/**************************************************************************/
/*! 
    @brief  Gets the sensor_t data
*/
/**************************************************************************/
void Adafruit_LSM303_Mag_Unified::getSensor(sensor_t *sensor) {
  /* Clear the sensor_t object */
  memset(sensor, 0, sizeof(sensor_t));

  /* Insert the sensor name in the fixed length char array */
  strncpy (sensor->name, "LSM303", sizeof(sensor->name) - 1);
  sensor->name[sizeof(sensor->name)- 1] = 0;
  sensor->version     = 1;
  sensor->sensor_id   = _sensorID;
  sensor->type        = SENSOR_TYPE_MAGNETIC_FIELD;
  sensor->min_delay   = 0;
  sensor->max_value   = 0.0F; // TBD
  sensor->min_value   = 0.0F; // TBD
  sensor->resolution  = 0.0F; // TBD
}