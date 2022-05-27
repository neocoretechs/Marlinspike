/***********************************************************************
* RoboCore robotic controller platform.
*
* Processes a variation of M and G code from CNC and 3D printing to control a range of motor controllers and drivers
* and GPIO pins for smart machine and robotics hardware functionality.
*
* Geared toward the Mega2560 microcontroller chip, this code unifies the microcontroller platform and allows it to be easily accessed through
* the standard CNC-derived 'language'.
*
* As an example, it unifies the timers to deliver consistent PWM functions across all available pins, and allows
* various motor controllers like H-bridge, half bridge, and smart controllers to be controlled through UART, PWM, and GPIO functions.
*
* It contains a main processing loop that receives M and G code commands through the UART via USB or TTL, like a CNC machine or 3D printer,
* then uses a pure C++, rewritten, unified version of the Wiring library for arduino to handle the functions provided in Wiring through the
* main CNC-derived M and G code processing loop.
*
* Servos, ADCs, Timers, Pin change interrupts, Analog and Digital IO pins, SPI, I2C, PWM, UARTs, and motor controllers are all unified and
* accessible through the main processing loop. In the primary loop, M and G code commands are processed such that this generic, multifunction,
* realtime, robotic smart controller can be attached to a single board computer (SBC), or used standalone and accessed through a UART,
* or potentially be attached via SPI, I2C or TWI to function as a realtime hardware controller.
*
* Another example is the way in which ultrasonic sensors can be attached to any motor driver through main loop processing commands and used to
* set up a minimum distance to detect an object before issuing a command to shut down the motor driver.
*
* To continue the examples, through the processing loop, hall effect sensors can be attached to any motor driver through pins designated in the main
* processing loop M and G code commands and used to detect wheel rotations, then set up pin change interrupts for those sensors through other
* M and G code commands, then perform actions based on the wheel rotations and the interrupts generated and processed through other M and G code commands.
*
* A microcontroller running this code can be used to easily construct a robot, a 3D printer, a CNC machine, or any number of devices that need
* a generic smart controller capable of low level access to GPIO pins and hardware timers and ADCs etc.
*
* Copyright NeoCoreTechs 2020
* Author: Jonathan Neville Groff
***********************************************************************/

#ifndef ROBOCORE_H
#define ROBOCORE_H

#define  FORCE_INLINE __attribute__((always_inline)) inline

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>

#include "fastio.h"
#include "Configuration.h"
#include "pins.h"
#include "Arduino.h"
#include "HardwareSerial/HardwareSerial.h"
#include "Ultrasonic.h"
#include "WAnalog.h"
#include "WDigital.h"

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif


#define SERIAL_PORT Serial

void setup();
void loop();
void get_command();
void process_commands();
void processGCode(int cval);
void processMCode(int cval);
void manage_inactivity();
void FlushSerialRequestResend();
void ClearToSend();
void kill();
void Stop();
bool IsStopped();
void refresh_cmd_timeout(void);
void publishMotorFaultCode(int fault);
void publishMotorStatCode(int stat);
void publishBatteryVolts(int volts);
void printUltrasonic(Ultrasonic* upin, int index); // index -> ultrasonic array
void printAnalog(Analog* apin, int index); // index -> analog array
void printDigital(Digital* dpin, int target); //'target' represents the EXCLUDED value, other than this we get a reading
void checkMotorInterval(void);
void checkSmartController(void);
 
#ifndef CRITICAL_SECTION_START
  #define CRITICAL_SECTION_START  unsigned char _sreg = SREG; cli();
  #define CRITICAL_SECTION_END    SREG = _sreg;
#endif

#define SERIAL_PGM(x) (serialprintPGM(PSTR(x)))
#define SERIAL_PGMLN(x) (serialprintPGM(PSTR(x)),SERIAL_PORT.println())

FORCE_INLINE void serialprintPGM(const char *str)
{
	char ch=pgm_read_byte(str);
	while(ch)
	{
		SERIAL_PORT.write(ch);
		ch=pgm_read_byte(++str);
	}
}

extern int fanSpeed;
extern unsigned long starttime;
extern unsigned long stoptime;


#endif
