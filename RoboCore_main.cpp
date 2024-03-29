
/*
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
 */

#include "Arduino.h"
#include "watchdog.h"
#include "ConfigurationStore.h"
#include "language.h"
#include "RoboCore.h"
#include "pins.h"
#include "pins_arduino.h"
#include "new.h"
#include "math.h"
#include "Servo.h"
#include "Propulsion/AbstractMotorControl.h"
#include "Propulsion/RoboteqDevice.h"
#include "Propulsion/HBridgeDriver.h"
#include "Propulsion/SplitBridgeDriver.h"
#include "Propulsion/SwitchBridgeDriver.h"
#include "CounterInterruptService.h"
#include "AbstractPWMControl.h"
#include "VariablePWMDriver.h"
#include "AccelStepper.h"

// look here for descriptions of gcodes: http://linuxcnc.org/handbook/gcode/g-code.html, protocol here is different but similar
// When 'stopped' is true the Gcodes G0-G5 are ignored as a safety interlock.

//===========================================================================
//=============================private variables=============================
//===========================================================================
static long gcode_N, gcode_LastN, Stopped_gcode_LastN = 0;

static uint8_t realtime_output = 1; // Determines whether real time data from inactive period is streamed

static char cmdbuffer[MAX_CMD_SIZE];
static char outbuffer[OUT_BUFFER_SIZE];

static char serial_char;
static int serial_read;
static int serial_count = 0;
static boolean comment_mode = false;
static char *strchr_pointer; // just a pointer to find chars in the cmd string like X, Y, Z, E, etc

//Inactivity shutdown variables
static unsigned long previous_millis_cmd = 0;
static unsigned long max_inactive_time = 0;

unsigned long starttime = 0;
unsigned long stoptime = 0;

bool Stopped=false;

bool target_direction;

byte mcu;
Analog* apin;
Digital* dpin;
PWM* ppin;
int nread = 0;
uint32_t micros = 0;
int* values;
String motorCntrlResp;
int status;
int fault = 0;
// Dynamically defined ultrasonic rangers
Ultrasonic* psonics[10]={0,0,0,0,0,0,0,0,0,0};
// Last distance published per sensor
float sonicDist[10]={0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f};
// Dynamically defined analog pins
int analogRanges[2][16];
Analog* panalogs[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
// Dynamically defined digital pins
int digitalTarget[32];
Digital* pdigitals[32]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
// PWM control block
PWM* ppwms[12]={0,0,0,0,0,0,0,0,0,0,0,0};

uint8_t channel;
  
int digitarg;
static int uspin = 0;
unsigned long t;
int pin_number, pin_numberB;
int dir_pin, dir_default, enable_pin;
int encode_pin = 0;
uint8_t dir_face;
uint32_t dist;
int timer_res = 8; // resolution in bits
int timer_pre = 1; // 1 is no prescale

WatchdogTimer* watchdog_timer=NULL;

// &roboteqDevice, new HBridgeDriver, new SplitBridgeDriver...
AbstractMotorControl* motorControl[10]={0,0,0,0,0,0,0,0,0,0};
AbstractPWMControl* pwmControl[10]={0,0,0,0,0,0,0,0,0,0};
	
#define STEPS_PER_TURN 2048 // number of steps in 360deg;	
AccelStepper* accelStepper;
//===========================================================================
//=============================ROUTINES=============================
//===========================================================================


extern "C"{
  extern unsigned int __bss_end;
  extern unsigned int __heap_start;
  extern void *__brkval;

  int freeMemory() {
    int free_memory;

    if((int)__brkval == 0)
      free_memory = ((int)&free_memory) - ((int)&__bss_end);
    else
      free_memory = ((int)&free_memory) - ((int)__brkval);

    return free_memory;
  }
}

#define POLY 0x8408
int crc_ok = 0x470F;
unsigned short crc16(char *data_p, unsigned short length) {
	unsigned char i;
	unsigned int data;
	unsigned int crc;
	
	crc = 0xffff;
	if (length == 0)
	return (~crc);
	do {
		for (i = 0 ,data = (unsigned int)0xff & *data_p++; i < 8; i++, data >>= 1) {
			if ((crc & 0x0001) ^ (data & 0x0001))
			crc = (crc >> 1) ^ POLY;
			else
			crc >>= 1;
		}
	} while (--length);
	
	crc = ~crc;
	data = crc;
	crc = (crc << 8) | (data >> 8 & 0xFF);
	return (crc);
}
void setup_killpin()
{
  #if defined(KILL_PIN) && KILL_PIN > -1
	Digital kp = new Digital(KILL_PIN);
    kp.pinMode(INPUT);
    kp.digitalWrite(HIGH);
  #endif
}

void setup_photpin()
{
  #if defined(PHOTOGRAPH_PIN) && PHOTOGRAPH_PIN > -1
    SET_OUTPUT(PHOTOGRAPH_PIN);
    WRITE(PHOTOGRAPH_PIN, LOW);
  #endif
}

void setup_powerhold()
{
  #if defined(SUICIDE_PIN) && SUICIDE_PIN > -1
    SET_OUTPUT(SUICIDE_PIN);
    WRITE(SUICIDE_PIN, HIGH);
  #endif
  #if defined(PS_ON_PIN) && PS_ON_PIN > -1
    SET_OUTPUT(PS_ON_PIN);
	#if defined(PS_DEFAULT_OFF)
	  WRITE(PS_ON_PIN, PS_ON_ASLEEP);
    #else
	  WRITE(PS_ON_PIN, PS_ON_AWAKE);
	#endif
  #endif
}

void suicide()
{
  #if defined(SUICIDE_PIN) && SUICIDE_PIN > -1
    SET_OUTPUT(SUICIDE_PIN);
    WRITE(SUICIDE_PIN, LOW);
  #endif
}

/*--------------------------------------------------
* Main setup a la wiring
*/
void setup()
{
  //setup_killpin();
  //setup_powerhold();
  sei();
  SERIAL_PORT.begin(BAUDRATE);
  Serial2.begin(BAUDRATE);
  // loads data from EEPROM if available else uses defaults (and resets step acceleration rate)
  Config_RetrieveSettings();
  //Config_PrintSettings();
  //watchdog_init();
}

/*-------------------------------------------------
* And the primary command parser loop
*/
void loop()
{
  get_command();
  if(!comment_mode)
  {
    process_commands();
  }
  manage_inactivity();
}
  
void get_command()
{
  comment_mode = true;
  while( SERIAL_PORT.available() > 0 ) {
    serial_read = SERIAL_PORT.read();
	if( serial_read == -1 )
		continue;
	serial_char = (char)serial_read;
    if(serial_char == '\n' || serial_char == '\r' || serial_count >= (MAX_CMD_SIZE - 1) ) {
      if(!serial_count) { //if empty line
        comment_mode = true; //for new command
        return;
      }
	  comment_mode = false;
      cmdbuffer[serial_count] = 0; //terminate string
      if(strchr(cmdbuffer, 'N') != NULL) {
          strchr_pointer = strchr(cmdbuffer, 'N');
          gcode_N = (strtol(&cmdbuffer[strchr_pointer - cmdbuffer + 1], NULL, 10));
          if(gcode_N != gcode_LastN+1 && (strstr_P(cmdbuffer, PSTR("M110")) == NULL) ) {
			SERIAL_PGM(MSG_BEGIN);
            SERIAL_PGM(MSG_ERR_LINE_NO);
            SERIAL_PORT.print(gcode_LastN);
			SERIAL_PGMLN(MSG_TERMINATE);
            //Serial.println(gcode_N);
            FlushSerialRequestResend();
            serial_count = 0;
			comment_mode = true;
            return;
          }
          if(strchr(cmdbuffer, '*') != NULL) {
            unsigned short checksum = 0;
            byte count = 0;
            //while(cmdbuffer[bufindw][count] != '*') checksum = checksum^cmdbuffer[bufindw][count++];
            strchr_pointer = strchr(cmdbuffer, '*');
			checksum = crc16(&cmdbuffer[strchr_pointer - cmdbuffer + 1],(strchr_pointer - cmdbuffer + 1) );
            if( (int)(strtod(&cmdbuffer[strchr_pointer - cmdbuffer + 1], NULL)) != checksum) {
			  SERIAL_PGM(MSG_BEGIN);
              SERIAL_PGM(MSG_ERR_CHECKSUM_MISMATCH);
              SERIAL_PORT.print(gcode_LastN);
			  SERIAL_PGMLN(MSG_TERMINATE);
              FlushSerialRequestResend();
              serial_count = 0;
			  comment_mode = true;
              return;
            }
            //if no errors, continue parsing
          } else {
			SERIAL_PGM(MSG_BEGIN);
            SERIAL_PGM(MSG_ERR_NO_CHECKSUM);
            SERIAL_PORT.print(gcode_LastN);
			SERIAL_PGMLN(MSG_TERMINATE);
            FlushSerialRequestResend();
            serial_count = 0;
			comment_mode = true;
            return;
          }
          gcode_LastN = gcode_N;
          //if no errors, continue parsing
      } else { // if we don't receive 'N' but still see '*'
          if((strchr(cmdbuffer, '*') != NULL)) {
			SERIAL_PGM(MSG_BEGIN);
            SERIAL_PGM(MSG_ERR_NO_LINENUMBER_WITH_CHECKSUM);
            SERIAL_PORT.print(gcode_LastN);
			SERIAL_PGMLN(MSG_TERMINATE);
			SERIAL_PORT.flush();
            serial_count = 0;
			comment_mode = true;
            return;
          }
      }
      if(strchr(cmdbuffer, ';') != NULL) {
		  comment_mode = true;
          serial_count = 0; //clear buffer
		  return;
	  }
	  // Determine if an outstanding error caused safety shutdown. If so respond with header
      if((strchr(cmdbuffer, 'G') != NULL)){
          strchr_pointer = strchr(cmdbuffer, 'G');
          switch((int)((strtod(&cmdbuffer[strchr_pointer - cmdbuffer + 1], NULL)))) {
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
				if(Stopped) { // If robot is stopped by an error the G[0-5] codes are ignored.
					SERIAL_PGM(MSG_BEGIN);
					SERIAL_PGM(MSG_ERR_STOPPED);
					SERIAL_PGMLN(MSG_TERMINATE);
					SERIAL_PORT.flush();
					comment_mode = true;
					serial_count = 0;
					return;
				}
				break;
			default:
				break;
          }
        }
		// finished processing c/r terminated cmdl
		serial_count = 0;
		return;
      } // if c/r l/f
      cmdbuffer[serial_count++] = serial_char;
  } // while avail

}

float code_value()
{
  return (strtod(&cmdbuffer[strchr_pointer - cmdbuffer + 1], NULL));
}

long code_value_long()
{
  return (strtol(&cmdbuffer[strchr_pointer - cmdbuffer + 1], NULL, 10));
}

bool code_seen(char code)
{
  strchr_pointer = strchr(cmdbuffer, code);
  return (strchr_pointer != NULL);  //Return True if a character was found
}

#define DEFINE_PGM_READ_ANY(type, reader)       \
    static inline type pgm_read_any(const type *p)  \
    { return pgm_read_##reader##_near(p); }

DEFINE_PGM_READ_ANY(float,       float);
DEFINE_PGM_READ_ANY(signed char, byte);

#define XYZ_CONSTS_FROM_CONFIG(type, array, CONFIG) \
static const PROGMEM type array##_P[3] =        \
    { X_##CONFIG, Y_##CONFIG, Z_##CONFIG };     \
static inline type array(int axis)          \
    { return pgm_read_any(&array##_P[axis]); }


/*----------------------------------------
*
* Process the command sequence
*-----------------------------------------
*/
void process_commands() { 
  if(code_seen('G')) {
	  int cval = (int)code_value();
	  processGCode(cval);
  } else {
	  if(code_seen('M') ) {
		  int cval = (int)code_value();
		  processMCode(cval);
	  } else { // if neither G nor M code
		   int ibuf = 0;
		   SERIAL_PGM(MSG_BEGIN);
		   SERIAL_PGM(MSG_UNKNOWN_COMMAND);
		   while(cmdbuffer[ibuf]) SERIAL_PORT.print(cmdbuffer[ibuf++]);
		   SERIAL_PGMLN(MSG_TERMINATE);
		   SERIAL_PORT.flush();
	  }
  }
}
/*--------------------------
* Process the Gcode command sequence
*---------------------------
*/
void processGCode(int cval) {
	  int result;
	  int motorController, PWMDriver, motorChannel, motorPower, PWMLevel;
	  unsigned long codenum;
	  char *starpos = NULL;
    switch(cval)
    {
	    
    case 4: // G4 dwell
      //LCD_MESSAGEPGM(MSG_DWELL);
      codenum = 0;
      if(code_seen('P')) codenum = code_value(); // milliseconds to wait
      if(code_seen('S')) codenum = code_value() * 1000; // seconds to wait

      //codenum += millis();  // keep track of when we started waiting
      previous_millis_cmd = 0;//millis();
      while(++previous_millis_cmd  < codenum ){
        manage_inactivity();
		_delay_ms(1);
      }
	  SERIAL_PGM(MSG_BEGIN);
	  SERIAL_PGM("G4");
	  SERIAL_PGMLN(MSG_TERMINATE);
	  SERIAL_PORT.flush();
      break;
	  
	case 5: // G5 - Absolute command motor [Z<controller>] C<Channel> [P<motor power -1000 to 1000>] [X<PWM power -1000 to 1000>(scaled 0-2000)]
	     if(!Stopped) {
			 if(code_seen('Z')) {
				 motorController = code_value();
			 }
		     if(code_seen('C')) {
				motorChannel = code_value(); // channel 1,2
				if(code_seen('P')) {
					motorPower = code_value(); // motor power -1000,1000
					fault = 0; // clear fault flag
					if( (status=motorControl[motorController]->commandMotorPower(motorChannel, motorPower)) ) {
							SERIAL_PGM(MSG_BEGIN);
							SERIAL_PGM(MSG_BAD_MOTOR);
							SERIAL_PORT.print(status);
							SERIAL_PORT.print(' ');
							SERIAL_PORT.print(motorChannel);
							SERIAL_PORT.print(' ');
							SERIAL_PORT.print(motorPower);
							SERIAL_PGMLN(MSG_TERMINATE);
							SERIAL_PORT.flush();
					} else {
							SERIAL_PGM(MSG_BEGIN);
							SERIAL_PGM("G5");
							SERIAL_PGMLN(MSG_TERMINATE);
							SERIAL_PORT.flush();
					}
				} else {// code P or X
					if(code_seen('X')) {
						PWMLevel = code_value(); // PWM level -1000,1000, scaled to 0-2000 in PWM controller, as no reverse
						fault = 0; // clear fault flag
						// use motor related index and value, as we have them
						if( (status=pwmControl[motorController]->commandPWMLevel(motorChannel, PWMLevel)) ) {
							SERIAL_PGM(MSG_BEGIN);
							SERIAL_PGM(MSG_BAD_PWM);
							SERIAL_PORT.print(status);
							SERIAL_PORT.print(' ');
							SERIAL_PORT.print(motorChannel);
							SERIAL_PORT.print(' ');
							SERIAL_PORT.print(PWMLevel);
							SERIAL_PGMLN(MSG_TERMINATE);
							SERIAL_PORT.flush();
						} else {
							SERIAL_PGM(MSG_BEGIN);
							SERIAL_PGM("G5");
							SERIAL_PGMLN(MSG_TERMINATE);
							SERIAL_PORT.flush();
						}
					} // code X
				}
			 } // code C
	     } // stopped
	     break;
	  
	case 99: // G99 start watchdog timer. G99 T<time_in_millis> values are 15,30,60,120,250,500,1000,4000,8000 default 4000
		if( code_seen('T') ) {
			int time_val = code_value();
			watchdog_timer = new WatchdogTimer();
			SERIAL_PGM(MSG_BEGIN);
			SERIAL_PGM("G99");
			SERIAL_PGMLN(MSG_TERMINATE);
			SERIAL_PORT.flush();
			watchdog_timer->watchdog_init(time_val);
		}
		break;
		
	case 100: // G100 reset watchog timer before time interval is expired, otherwise a reset occurs
		if( watchdog_timer != NULL ) {
			watchdog_timer->watchdog_reset();
		}
		SERIAL_PGM(MSG_BEGIN);
		SERIAL_PGM("G100");
		SERIAL_PGMLN(MSG_TERMINATE);
		SERIAL_PORT.flush();
		break;
	
	case 200: // G200 set up stepper. G200 W<wires else default 4> P<pin 1 default 22> Q<pin 2 default 24> R<pin 3 default 26> S<pin 4 defualt 28> F<pulse width default 20> M<motor speed default 500> A<motor accel def 400>
		int swire;
		swire = code_seen('W') ? code_value() : 4;
		int p1,q2,r3,s4;
		p1 = code_seen('P') ? code_value() : 22;
		q2 = code_seen('Q') ? code_value() : 24;
		r3 = code_seen('R') ? code_value() : 26;
		s4 = code_seen('S') ? code_value() : 28;
		accelStepper = new AccelStepper(swire,p1,q2,r3,s4,true);
		int pulseWidth;
		int motorSpeed;
		int motorAccel;
		pulseWidth = code_seen('F') ? code_value() : 20;
		motorSpeed = code_seen('M') ? code_value() : 500;
		motorAccel = code_seen('A') ? code_value() : 400;
		accelStepper->setMinPulseWidth(pulseWidth); // 20 prevents pulses too quick to be decoded
		accelStepper->setMaxSpeed(motorSpeed);
		accelStepper->setSpeed(motorSpeed);
		accelStepper->setAcceleration(motorAccel);
		// the following two lines reset "step zero" to the current position
		accelStepper->setCurrentPosition(accelStepper->currentPosition());
		accelStepper->runToPosition();
		SERIAL_PGM(MSG_BEGIN);
		SERIAL_PGM("G200");
		SERIAL_PGMLN(MSG_TERMINATE);
		SERIAL_PORT.flush();
		break;
		
	case 201: // G201 stepper stop
		if(accelStepper) {
			accelStepper->stop();
			accelStepper->runToPosition();
			accelStepper->run();
			SERIAL_PGM(MSG_BEGIN);
			SERIAL_PGM("G201");
			SERIAL_PGMLN(MSG_TERMINATE);
			SERIAL_PORT.flush();
		}
		break;
	
	case 202: // G202 S<steps>  clockwise
		int steps;
		if(accelStepper) {
			if(code_seen('S'))
				steps = code_value();
	        // The two lines that follow allow to send commands in any sequence:
	        // before execution, a quick stop is performed
	        //accelStepper->stop(); // Stop as fast as possible: sets new target
	        //accelStepper->runToPosition(); // Now stopped after quickstop
	        accelStepper->setCurrentPosition(accelStepper->currentPosition()); // Set step 0 "here"
	        accelStepper->setSpeed(motorSpeed); // Previous commands have reset the speed
	        // Since we step forward 2047 steps, current position is starting point of the rotation
			accelStepper->moveTo(accelStepper->currentPosition()+steps); // 1 turn = 2048 step
			while (accelStepper->currentPosition() != accelStepper->targetPosition()) {
				accelStepper->runSpeedToPosition();
			}
			//accelStepper->run();
			SERIAL_PGM(MSG_BEGIN);
			SERIAL_PGM("G202");
			SERIAL_PGMLN(MSG_TERMINATE);
			SERIAL_PORT.flush();
		}
		break;
		
	case 203: // G203 S<steps>  anti-clockwise
		if(accelStepper) {
			if(code_seen('S'))
				steps = code_value();
			accelStepper->setCurrentPosition(accelStepper->currentPosition()); // Set step 0 "here"
			accelStepper->setSpeed(-motorSpeed); // Previous commands have reset the speed
			// Since we step backward 2047 steps, current position is starting point of the rotation
			accelStepper->moveTo(accelStepper->currentPosition()-steps); // 1 turn = 2048 step
			while (accelStepper->currentPosition() != accelStepper->targetPosition()) {
					accelStepper->runSpeedToPosition();
			}
			SERIAL_PGM(MSG_BEGIN);
			SERIAL_PGM("G203");
			SERIAL_PGMLN(MSG_TERMINATE);
			SERIAL_PORT.flush();
		}
		break;	
		
	default:
		int ibuf = 0;
		SERIAL_PGM(MSG_BEGIN);
		SERIAL_PGM(MSG_UNKNOWN_GCODE);
		while(cmdbuffer[ibuf]) SERIAL_PORT.print(cmdbuffer[ibuf++]);
		SERIAL_PGMLN(MSG_TERMINATE);
		SERIAL_PORT.flush();
		break;
		
    } // switch
  } // if code
  
  /*-------------------------------------
  * M Code processing
  *--------------------------------------
  */
  void processMCode(int cval) {
	  int motorController = 0; 
	  int PWMDriver = 0;
	  
    switch( cval ) {
	case 0: // M0 - Set real time output off
		realtime_output = 0;
		SERIAL_PGM(MSG_BEGIN);
		SERIAL_PGM("M0");
		SERIAL_PGMLN(MSG_TERMINATE);
		SERIAL_PORT.flush();
		break;
		
	case 1: // M1 - Set real time output on
		realtime_output = 1;
		SERIAL_PGM(MSG_BEGIN);
		SERIAL_PGM("M1");
		SERIAL_PGMLN(MSG_TERMINATE);
		SERIAL_PORT.flush();
		break;
		
	//CHANNEL 1-10, NO CHANNEL ZERO!	
	case 2: // M2 [Z<slot>] [C<channel> W<encoder pin> E<default dir>] - set smart controller (default) with optional encoder pin per channel, can be issued multiple times
		 if(code_seen('Z')) {
			 motorController = code_value();
		 }
		if(code_seen('C')) {
			channel = code_value();
			if(channel <= 0) {
				break;
			}
			if(code_seen('W')) {
				pin_number = code_value();
				motorControl[motorController]->createEncoder(channel, pin_number);
			}
			if(code_seen('E')) {
				motorControl[motorController]->setDefaultDirection(channel, code_value());
			}
			SERIAL_PGM(MSG_BEGIN);
			SERIAL_PGM("M2");
			SERIAL_PGMLN(MSG_TERMINATE);
			SERIAL_PORT.flush();
		}
		break;
		
	// Set HBridge PWM motor driver, map pin to channel, this will check to prevent free running motors during inactivity
	// For a PWM motor control subsequent G5 commands are affected here.
	// and then D a direction pin that determines forward/backward , then E, the default value of the direction pin.
	// The D and E pins determine whether a HIGH or LOW determines FORWARD/BACK on the given controller channel. This is
	// to compensate for 'backward' motors mounted left/right, or differences in controller design. Using a combination of
	// these 2 parameters you can tune any controller/motor setup properly for forward/back.
	// Finally, W<encoder pin>  to receive hall wheel sensor signals and
	// optionally PWM timer setup [R<resolution 8,9,10 bits>] [X<prescale 0-7>].
	// The Timer mode (0-3) is preset to 2 in the individual driver. Page 129 in datasheet. Technically we are using a 'non PWM'
	// where the 'compare output mode' is defined by 3 operating modes. Since we are unifying all the timers to use all available PWM
	// pins, the common mode among them all is the 'non PWM', within which the 3 available operating modes can be chosen from.
	// There are essentially three main operating modes:
	// 0 - Stop
	// 1 - Toggle on compare match
	// 2 - Clear on match
	// 3 - Set on match
	// For motor operation and general purpose PWM, mode 2 the most universally applicable.
	case 3: // M3 [Z<slot>] P<pin> C<channel> D<direction pin> E<default dir> W<encoder pin> [R<resolution 8,9,10 bits>] [X<prescale 0-7>]
		timer_res = 8; // resolution in bits
		timer_pre = 1; // 1 is no prescale
		pin_number = -1;
		encode_pin = 0;
		if(code_seen('Z')) {
			motorController = code_value();
		}
	 // motorControl = (AbstractMotorControl*)&hBridgeDriver;
	 if(motorControl[motorController]) {
	  ((HBridgeDriver*)motorControl[motorController])->setMotors((PWM**)&ppwms);
	  ((HBridgeDriver*)motorControl[motorController])->setDirectionPins((Digital**)&pdigitals);
	  if(code_seen('P')) {
          pin_number = code_value();
	  } else {
		 break;
	  }
      if(code_seen('C')) {
        channel = code_value();
		if(channel <= 0) {
			break;
		}
		if( code_seen('D')) {
			dir_pin = code_value();
		} else {
			break;
		}
		if( code_seen('E')) {
			dir_default = code_value();
		} else {
			break;
		}
		if( code_seen('W')) {
			encode_pin = code_value();
		}
		if(code_seen('X')) {
			timer_pre = code_value();
		}
		if( code_seen('R')) {
			timer_res = code_value();
		}
		((HBridgeDriver*)motorControl[motorController])->createPWM(channel, pin_number, dir_pin, dir_default, timer_pre, timer_res);
		if(encode_pin) {
			motorControl[motorController]->createEncoder(channel, encode_pin);
		}
		SERIAL_PGM(MSG_BEGIN);
		SERIAL_PGM("M3");
		SERIAL_PGMLN(MSG_TERMINATE);
		SERIAL_PORT.flush();
	   } // code_seen['C']
      } // if motorControl[motorController]
	  break;
	  
	// Split bridge or 2 half bridge motor controller. Takes 2 inputs: one for forward,called P, one for backward,called Q, then motor channel, 
	// and then D, an enable pin. Finally, W<encoder pin>  to receive hall wheel sensor signals and 
	// optionally PWM timer setup [R<resolution 8,9,10 bits>] [X<prescale 0-7>].
	// Everything derived from HBridgeDriver can be done here.
	case 4:// M4 [Z<slot>] P<pin> Q<pin> C<channel> D<enable pin> E<default dir> [W<encoder pin>] [R<resolution 8,9,10 bits>] [X<prescale 0-7>]
	  timer_res = 8; // resolution in bits
	  timer_pre = 1; // 1 is no prescale
	  pin_number = -1;
	  pin_numberB = -1;
	  encode_pin = 0;
	  if(code_seen('Z')) {
		motorController = code_value();
	  }
	  if(motorControl[motorController]) {
	  //motorControl = (AbstractMotorControl*)&splitBridgeDriver;
	  ((SplitBridgeDriver*)motorControl[motorController])->setMotors((PWM**)&ppwms);
	  ((SplitBridgeDriver*)motorControl[motorController])->setDirectionPins((Digital**)&pdigitals);
	  if(code_seen('P')) {
		pin_number = code_value();
	  } else {
		 break;
	  }
	  if(code_seen('Q')) {
		pin_numberB = code_value();
	 } else {
		break;
	 }
	  if(code_seen('C')) {
		  channel = code_value();
		  if(channel <= 0) {
			break;
		  }
		  if( code_seen('D')) {
			dir_pin = code_value();
		  } else {
			break;
		  }
		  if( code_seen('E')) {
			dir_default = code_value();
		  } else {
			break;
		  }
		  if( code_seen('W')) {
			encode_pin = code_value();
		  }
		  if(code_seen('X')) {
				timer_pre = code_value();
		  }
		  if( code_seen('R')) {
				timer_res = code_value();
		  }
		  ((SplitBridgeDriver*)motorControl[motorController])->createPWM(channel, pin_number, pin_numberB, dir_pin, dir_default, timer_pre, timer_res);
		  if(encode_pin) {
			motorControl[motorController]->createEncoder(channel, encode_pin);
		  }
		  SERIAL_PGM(MSG_BEGIN);
		  SERIAL_PGM("M4");
		  SERIAL_PGMLN(MSG_TERMINATE);
		  SERIAL_PORT.flush();
		} // code C
		} //motorcontrol[motorcontroller]
		break;
		
	// Switch bridge or 2 digital motor controller. Takes 2 inputs: one digital pin for forward,called P, one for backward,called Q, then motor channel,
	// and then D, an enable pin, and E default dir, with optional encoder
	case 5: //M5 Z<slot> P<pin> Q<pin> C<channel> D<enable pin> E<default dir> [W<encoder>]- Create switch bridge Z slot, P forward pin, Q reverse pin, D enable, E default state of enable for dir
		pin_number = -1;
		pin_numberB = -1;
		encode_pin = 0;
		  if(code_seen('Z')) {
			  motorController = code_value();
		  }
		  if(motorControl[motorController]) {
			  ((SwitchBridgeDriver*)motorControl[motorController])->setPins((Digital**)&pdigitals);
			  if(code_seen('P')) {
				  pin_number = code_value();
			  } else {
				  break;
			  }
			  if(code_seen('Q')) {
				  pin_numberB = code_value();
			  } else {
				  break;
			  }
			  if(code_seen('C')) {
				  channel = code_value();
				  if(channel <= 0) {
					  break;
				  }
				  if( code_seen('D')) {
					  dir_pin = code_value();
				  } else {
					  break;
				  }
				  if( code_seen('E')) {
					  dir_default = code_value();
				  } else {
					  break;
				  }
				  if( code_seen('W')) {
					 encode_pin = code_value();
				  }
				  ((SwitchBridgeDriver*)motorControl[motorController])->createDigital(channel, pin_number, pin_numberB, dir_pin, dir_default);
				  if(encode_pin) {
					  motorControl[motorController]->createEncoder(channel, encode_pin);
				  }
				  SERIAL_PGM(MSG_BEGIN);
				  SERIAL_PGM("M5");
				  SERIAL_PGMLN(MSG_TERMINATE);
				  SERIAL_PORT.flush();
			  } // code C
		  } //motorcontrol[motorcontroller]
		break;
		
	case 6: //M6 [Z<slot>] [S<scale>] [X<scale>] - Set motor or PWM scaling, divisor for final power to limit speed or level, set to 0 to cancel. If X, slot is PWM
		if(code_seen('Z')) {
			motorController = code_value();
		}
		if( code_seen('S') ) {
			if(motorControl[motorController]) {
				motorControl[motorController]->setMotorPowerScale(code_value());
				SERIAL_PGM(MSG_BEGIN);
				SERIAL_PGM("M6");
				SERIAL_PGMLN(MSG_TERMINATE);
				SERIAL_PORT.flush();
			}
		} else {
			if(code_seen('X')) {
				if(pwmControl[motorController]) {
					pwmControl[motorController]->setPWMPowerScale(code_value());
					SERIAL_PGM(MSG_BEGIN);
					SERIAL_PGM("M6");
					SERIAL_PGMLN(MSG_TERMINATE);
					SERIAL_PORT.flush();
				}
			}
		}
		break;
		
	case 7: // M7 [Z<slot>] [X]- Set motor override to stop motor operation, or optionally PWM operation, if X, slot is PWM
		if(code_seen('Z')) {
			motorController = code_value();
		}
		if(code_seen('X')) {
			if(pwmControl[motorController]) {
				pwmControl[motorController]->setPWMShutdown();
				SERIAL_PGM(MSG_BEGIN);
				SERIAL_PGM("M7");
				SERIAL_PGMLN(MSG_TERMINATE);
				SERIAL_PORT.flush();
			}
		} else {
			if(motorControl[motorController]) {
				motorControl[motorController]->setMotorShutdown();
				SERIAL_PGM(MSG_BEGIN);
				SERIAL_PGM("M7");
				SERIAL_PGMLN(MSG_TERMINATE);
				SERIAL_PORT.flush();
			}
		}
		break;
		
	case 8: // M8 [Z<slot>][X] - Set motor override to start motor operation after stop override M7. If X, slot is PWM
		if(code_seen('Z')) {
			motorController = code_value();
		}
		if(code_seen('X')) {
			if(pwmControl[motorController]) {
				pwmControl[motorController]->setPWMRun();
				SERIAL_PGM(MSG_BEGIN);
				SERIAL_PGM("M8");
				SERIAL_PGMLN(MSG_TERMINATE);
				SERIAL_PORT.flush();
			}
		} else {
			if(motorControl[motorController] ) {
				motorControl[motorController]->setMotorRun();
				SERIAL_PGM(MSG_BEGIN);
				SERIAL_PGM("M8");
				SERIAL_PGMLN(MSG_TERMINATE);
				SERIAL_PORT.flush();
			}
		}
		break;
		
	// Activate a previously created PWM controller of type AbstractPWMControl - a non propulsion PWM device such as LED or pump
	// Note there is no encoder or direction pin, and no possibility of reverse. What would be reverse in a motor control is the first
	// half of the power scale instead.
	case 9: // M9 [Z<slot>] P<pin> C<channel> D<enable pin> [R<resolution 8,9,10 bits>] [X<prescale 0-7>] - PWM control
		timer_res = 8; // resolution in bits
		timer_pre = 1; // 1 is no prescale
		pin_number = -1;
		encode_pin = 0;
		if(code_seen('Z')) {
			  PWMDriver = code_value();
		}
		if(pwmControl[PWMDriver]) {
		 ((VariablePWMDriver*)pwmControl[PWMDriver])->setPWMs((PWM**)&ppwms);
		 ((VariablePWMDriver*)pwmControl[PWMDriver])->setEnablePins((Digital**)&pdigitals);
		 if(code_seen('P')) {
			  pin_number = code_value();
		 } else {
			  break;
		 }
		 if(code_seen('C')) {
			channel = code_value();
			if(channel <= 0) {
				break;
			}
			if( code_seen('D')) {
				enable_pin = code_value();
			} else {
				break;
			}
			if(code_seen('X')) {
				timer_pre = code_value();
			}
			if( code_seen('R')) {
				timer_res = code_value();
			}
			((VariablePWMDriver*)pwmControl[PWMDriver])->createPWM(channel, pin_number, enable_pin, timer_pre, timer_res);
			SERIAL_PGM(MSG_BEGIN);
			SERIAL_PGM("M9");
			SERIAL_PGMLN(MSG_TERMINATE);
			SERIAL_PORT.flush();
		 }
		}
		break;
		
	// Dynamically allocate a controller to a control slot. The slot parameter is used to refer
	// to the dynamically allocated controller in other M codes that relate to motor control functions.
	// The M10 code merely creates the instance of the proper controller and assigns the slot. Other M codes
	// refer to the slot and provide further configuration. when creating new type of controllers, this is the code
	// that can be expanded to instantiate those controllers
	case 10: // M10 Z<controller slot> T<controller type>
		if( code_seen('Z') ) {
			motorController = code_value();
			if( code_seen('T') ) {
				int controllerType = code_value();		 
				switch(controllerType) {
					case 0: // type 0 smart controller
						if( motorControl[motorController] ) {
							delete motorControl[motorController];
							motorControl[motorController] = 0; // in case assignment below fails
						}
						motorControl[motorController] = new RoboteqDevice();
						SERIAL_PGM(MSG_BEGIN);
						SERIAL_PGM("M10");
						SERIAL_PGMLN(MSG_TERMINATE);
						SERIAL_PORT.flush();
						break;
					case 1: // type 1 Hbridge
						// up to 10 channels, each channel has a direction pin (1), and a PWM pin (0)
						if(motorControl[motorController]) {
							// for each channel, delete the direction pin and PWM created in main pin array to prepare new assignment
							// each controller can have up to 10 channels, each with its own PWM and direction pin
							for(uint8_t i = 0; i < motorControl[motorController]->getChannels(); i++) {
									uint8_t pMotor1 = ((HBridgeDriver*)motorControl[motorController])->getMotorPWMPin(i);
									uint8_t pMotor2 = ((HBridgeDriver*)motorControl[motorController])->getMotorEnablePin(i);
									if(pMotor2 != 255 && pdigitals[pMotor2]) {
										delete pdigitals[pMotor2];
										pdigitals[pMotor2] = 0;
									}
									if(pMotor1 != 255 && ppwms[pMotor1]) {
										delete ppwms[pMotor1];
										ppwms[pMotor1] = 0;
									}
							}
							delete motorControl[motorController];
							motorControl[motorController] = 0; // in case assignment below fails
						}
						motorControl[motorController] = new HBridgeDriver();
						SERIAL_PGM(MSG_BEGIN);
						SERIAL_PGM("M10");
						SERIAL_PGMLN(MSG_TERMINATE);
						SERIAL_PORT.flush();
						break;
					case 2: // type 2 Split bridge, each channel has 2 PWM pins and an enable pin, so up to 5 channels
						if(motorControl[motorController]) {
								// for each channel, delete the direction pin and PWM created in main pin array to prepare new assignment
								// each controller can have up to 10 channels, each with its own PWM and direction pin
								for(uint8_t i = 0; i < motorControl[motorController]->getChannels(); i++) {
										uint8_t pMotor1 = ((SplitBridgeDriver*)motorControl[motorController])->getMotorPWMPin(i);
										uint8_t pMotor2 = ((SplitBridgeDriver*)motorControl[motorController])->getMotorEnablePin(i);
										if(pMotor2 != 255 && pdigitals[pMotor2]) {
											delete pdigitals[pMotor2];
											pdigitals[pMotor2] = 0;
										}
										if(pMotor1 != 255 && ppwms[pMotor1]) {
											delete ppwms[pMotor1];
											ppwms[pMotor1] = 0;
										}
										pMotor1 = ((SplitBridgeDriver*)motorControl[motorController])->getMotorPWMPinB(i);
										if(pMotor1 != 255 && ppwms[pMotor1]) {
											delete ppwms[pMotor1];
											ppwms[pMotor1] = 0;
										}
										
								}
								delete motorControl[motorController];
								motorControl[motorController] = 0; // in case assignment below fails
						}
						motorControl[motorController] = new SplitBridgeDriver();
						SERIAL_PGM(MSG_BEGIN);
						SERIAL_PGM("M10");
						SERIAL_PGMLN(MSG_TERMINATE);
						SERIAL_PORT.flush();
						break;
					case 3: // type 3 Switch bridge, each channel has 2 PWM pins and an enable pin, so up to 5 channels
						if(motorControl[motorController]) {
							// for each channel, delete the direction pin and PWM created in main pin array to prepare new assignment
							// each controller can have up to 10 channels, each with its own PWM and direction pin
							for(uint8_t i = 0; i < motorControl[motorController]->getChannels(); i++) {
								uint8_t pMotor1 = ((SwitchBridgeDriver*)motorControl[motorController])->getMotorDigitalPin(i);
								uint8_t pMotor2 = ((SwitchBridgeDriver*)motorControl[motorController])->getMotorEnablePin(i);
								if(pMotor2 != 255 && pdigitals[pMotor2]) {
									delete pdigitals[pMotor2];
									pdigitals[pMotor2] = 0;
								}
								if(pMotor1 != 255 && pdigitals[pMotor1]) {
									delete pdigitals[pMotor1];
									pdigitals[pMotor1] = 0;
								}
								pMotor1 = ((SwitchBridgeDriver*)motorControl[motorController])->getMotorDigitalPinB(i);
								if(pMotor1 != 255 && pdigitals[pMotor1]) {
									delete pdigitals[pMotor1];
									pdigitals[pMotor1] = 0;
								}
							}
							delete motorControl[motorController];
							motorControl[motorController] = 0; // in case assignment below fails
						}
						motorControl[motorController] = new SwitchBridgeDriver();
						SERIAL_PGM(MSG_BEGIN);
						SERIAL_PGM("M10");
						SERIAL_PGMLN(MSG_TERMINATE);
						SERIAL_PORT.flush();
						break;
					case 4: // Type 4 non-propulsion PWM driver 
						if(pwmControl[motorController]) {
							// for each channel, delete the direction pin and PWM created in main pin array to prepare new assignment
							// each controller can have up to 10 channels, each with its own PWM and direction pin
							for(uint8_t i = 0; i < pwmControl[motorController]->getChannels(); i++) {
								uint8_t pMotor1 = ((VariablePWMDriver*)pwmControl[motorController])->getPWMEnablePin(i);
									if(pMotor1 != 255 && pdigitals[pMotor1]) {
										delete pdigitals[pMotor1];
										pdigitals[pMotor1] = 0;
									}
									pMotor1 = ((VariablePWMDriver*)pwmControl[motorController])->getPWMLevelPin(i);
									if(pMotor1 != 255 && ppwms[pMotor1]) {
										delete ppwms[pMotor1];
										ppwms[pMotor1] = 0;
									}
							}
							delete pwmControl[motorController];
							motorControl[motorController] = 0; // in case assignment below fails
						}
						pwmControl[motorController] = new VariablePWMDriver();
						SERIAL_PGM(MSG_BEGIN);
						SERIAL_PGM("M10");
						SERIAL_PGMLN(MSG_TERMINATE);
						SERIAL_PORT.flush();
						break;
					default:
						SERIAL_PGM(MSG_BEGIN);
						SERIAL_PGM("BAD CONTROLLER TYPE:");
						SERIAL_PORT.println(controllerType);
						SERIAL_PGMLN(MSG_TERMINATE);
						SERIAL_PORT.flush();
						break;
				}
			} else {
				break;
			}
		}
		break;
		
	case 11: // M11 [Z<slot>] C<channel> [D<duration>] [X<duration>] - Set maximum cycle duration for given channel. If X, slot is PWM
		if(code_seen('Z')) {
			motorController = code_value();
		}
		if( code_seen('C') ) {
			channel = code_value();
			if(channel <= 0) {
				break;
			}
			if(code_seen('X')) {
				if(pwmControl[motorController]) {
					pwmControl[motorController]->setDuration(channel, code_value());
					SERIAL_PGM(MSG_BEGIN);
					SERIAL_PGM("M11");
					SERIAL_PGMLN(MSG_TERMINATE);
					SERIAL_PORT.flush();
				}
			} else {
				if(code_seen('D')) {
					if(motorControl[motorController]) {
						motorControl[motorController]->setDuration(channel, code_value());
						SERIAL_PGM(MSG_BEGIN);
						SERIAL_PGM("M11");
						SERIAL_PGMLN(MSG_TERMINATE);
						SERIAL_PORT.flush();
					}
				}
			}
		}
		break;
	
	case 12: // M12 [Z<slot>] C<channel> [P<offset>] [X<offset>] - set amount to add to G5 for min motor power, or X PWM level, If X, slot is PWM
		if(code_seen('Z')) {
			motorController = code_value();
		}
		if( code_seen('C') ) {
			channel = code_value();
			if(channel <= 0) {
				break;
			}
			if(code_seen('X')) {
				if(pwmControl[motorController]) {
					pwmControl[motorController]->setMinPWMLevel(channel, code_value());
					SERIAL_PGM(MSG_BEGIN);
					SERIAL_PGM("M12");
					SERIAL_PGMLN(MSG_TERMINATE);
					SERIAL_PORT.flush();
				}
			} else {
				if( code_seen('P')) {
					if(motorControl[motorController]) {
						motorControl[motorController]->setMinMotorPower(channel, code_value());
						SERIAL_PGM(MSG_BEGIN);
						SERIAL_PGM("M12");
						SERIAL_PGMLN(MSG_TERMINATE);
						SERIAL_PORT.flush();
					}
				}
			}
		}
		break;
		
	  case 13: //M13 [Z<slot>] [P<power>] [X<power>]- Set maximum motor power or optionally with X, a PWM control maximum level. If X, slot is PWM
		if(code_seen('Z')) {
		  motorController = code_value();
		}
		if( code_seen('P') ) {
		  if(motorControl[motorController]) {
			  motorControl[motorController]->setMaxMotorPower(code_value());
			  SERIAL_PGM(MSG_BEGIN);
			  SERIAL_PGM("M5");
			  SERIAL_PGMLN(MSG_TERMINATE);
			  SERIAL_PORT.flush();
		  }
		  } else {
		  if(code_seen('X')) {
			  if(pwmControl[motorController]) {
				  pwmControl[motorController]->setMaxPWMLevel(code_value());
				  SERIAL_PGM(MSG_BEGIN);
				  SERIAL_PGM("M5");
				  SERIAL_PGMLN(MSG_TERMINATE);
				  SERIAL_PORT.flush();
			  }
		  }
		}
	  break;
	  
	case 33: // M33 [Z<slot>] P<ultrasonic pin> D<min. distance in cm> [E<direction 1- forward facing, 0 - reverse facing sensor>] 
	// link Motor controller to ultrasonic sensor, the sensor must exist via M301
		if(code_seen('Z')) {
			motorController = code_value();
		}
	if(motorControl[motorController]) {
	  pin_number = 0;
	  if(code_seen('P')) {
        pin_number = code_value();
		if( code_seen('D')) {
			dist = code_value();
		} else {
			break;
		}
		dir_face = 1; // default forward
		if( code_seen('E')) {
			dir_face = code_value(); // optional
		}
		motorControl[motorController]->linkDistanceSensor((Ultrasonic**)psonics, pin_number, dist, dir_face);
		SERIAL_PGM(MSG_BEGIN);
		SERIAL_PGM("M33");
		SERIAL_PGMLN(MSG_TERMINATE);
		SERIAL_PORT.flush();
	  } // code_seen = 'P'
	}
	  break;
	  
	  case 35: //M35 - Clear all digital pins
		for(int i = 0; i < 32; i++) {
			 if(pdigitals[i]) {
				unassignPin(pdigitals[i]->pin);
				delete pdigitals[i];
				pdigitals[i] = 0;
			 }
		}
		SERIAL_PGM(MSG_BEGIN);
		SERIAL_PGM("M35");
		SERIAL_PGMLN(MSG_TERMINATE);
		SERIAL_PORT.flush();
	  break;
		  
	  case 36: //M36 - Clear all analog pins
		  	 for(int i = 0; i < 16; i++) {
			  	  if(panalogs[i]) {
					unassignPin(panalogs[i]->pin);
				  	delete panalogs[i];
					panalogs[i] = 0;
			  	  }
		  	 }
			 SERIAL_PGM(MSG_BEGIN);
			 SERIAL_PGM("M36");
			 SERIAL_PGMLN(MSG_TERMINATE);
			 SERIAL_PORT.flush();
	  break;
	  
	  case 37: //M37 - Clear all PWM pins, ALL MOTOR AND PWM DISABLED, perhaps not cleanly
		for(int i = 0; i < 12; i++) {
		  if(ppwms[i]) {
			  unassignPin(ppwms[i]->pin);
			  delete ppwms[i];
			  ppwms[i] = 0;
		  }
		}
	  	SERIAL_PGM(MSG_BEGIN);
	  	SERIAL_PGM("M37");
	  	SERIAL_PGMLN(MSG_TERMINATE);
	  	SERIAL_PORT.flush();
	  break;
	  
	  case 38: //M38  P<pin> - Remove PWM pin, MOTOR AND PWM DISABLED, perhaps not cleanly
	  	  pin_number = -1;
	  	  if (code_seen('P')) {
		  	  pin_number = code_value();
		  	  if(unassignPin(pin_number) ) {
			  	  for(int i = 0; i < 12; i++) {
				  	  if(ppwms[i] && ppwms[i]->pin == pin_number) {
					  	  delete ppwms[i];
						  ppwms[i] = 0;
				  	  } // pwms == pin_number
			  	  } // i iterate pwm array
				SERIAL_PGM(MSG_BEGIN);
				SERIAL_PGM("M38");
				SERIAL_PGMLN(MSG_TERMINATE);
				SERIAL_PORT.flush();
		  	  } // unassign pin
	  	  } // code P
	  break;
	  
	  case 39: //M39 P<pin> - Remove Persistent Analog pin 
	  	  pin_number = -1;
	  	  if (code_seen('P')) {
		  	  pin_number = code_value();
		  	  if(unassignPin(pin_number) ) {
			  	  for(int i = 0; i < 16; i++) {
				  	  if(panalogs[i] && panalogs[i]->pin == pin_number) {
					  	delete panalogs[i];
						panalogs[i] = 0;
					    break;
				  	  }
			  	  }
				SERIAL_PGM(MSG_BEGIN);
				SERIAL_PGM("M39");
				SERIAL_PGMLN(MSG_TERMINATE);
				SERIAL_PORT.flush();
		  	  }
	  	  }
	  break;
			
	  case 40: //M40 P<pin> - Remove persistent digital pin 
	       pin_number = -1;
	       if (code_seen('P')) {
		       pin_number = code_value();
		       if(unassignPin(pin_number) ) {
			       for(int i = 0; i < 32; i++) {
				       if(pdigitals[i] && pdigitals[i]->pin == pin_number) {
					       delete pdigitals[i];
						   pdigitals[i] = 0;
					       break;
				       }
			       }
				   	SERIAL_PGM(MSG_BEGIN);
				   	SERIAL_PGM("M40");
				   	SERIAL_PGMLN(MSG_TERMINATE);
				   	SERIAL_PORT.flush();
			   }
		   }
	  break;
		
	  case 41: //M41 - Create persistent digital pin, Write digital pin HIGH P<pin> (this gives you a 5v source on pin)
	     pin_number = -1;
	     if (code_seen('P')) {
		     pin_number = code_value();
		     if( assignPin(pin_number) ) {
			     dpin = new Digital(pin_number);
				 dpin->setPin(pin_number);
			     dpin->pinMode(OUTPUT);
			     dpin->digitalWrite(HIGH);
			     for(int i = 0; i < 32; i++) {
				     if(!pdigitals[i]) {
					     pdigitals[i] = dpin;
					     break;
				     }
			     }
				 SERIAL_PGM(MSG_BEGIN);
				 SERIAL_PGM("M41");
				 SERIAL_PGMLN(MSG_TERMINATE);
				 SERIAL_PORT.flush();
			 } else {
			     for(int i = 0; i < 32; i++) {
				     if(pdigitals[i] && pdigitals[i]->pin == pin_number) {
						 pdigitals[i]->setPin(pin_number);
						 pdigitals[i]->pinMode(OUTPUT);
					     pdigitals[i]->digitalWrite(HIGH);
					     break;
				     }
			     }
				 SERIAL_PGM(MSG_BEGIN);
				 SERIAL_PGM("M41");
				 SERIAL_PGMLN(MSG_TERMINATE);
				 SERIAL_PORT.flush();
		     }
	     }
	break;
	     
    case 42: //M42 - Create persistent digital pin, Write digital pin LOW P<pin> (This gives you a grounded pin)
	  pin_number = -1;
	  if (code_seen('P')) {
        pin_number = code_value();
		if( assignPin(pin_number) ) {
			dpin = new Digital(pin_number);
			dpin->pinMode(OUTPUT);
			dpin->digitalWrite(LOW);
			for(int i = 0; i < 32; i++) {
				if(!pdigitals[i]) {
					pdigitals[i] = dpin;
					break;
				}
			}
			SERIAL_PGM(MSG_BEGIN);
			SERIAL_PGM("M42");
			SERIAL_PGMLN(MSG_TERMINATE);
			SERIAL_PORT.flush();
		} else {
			for(int i = 0; i < 32; i++) {
				if(pdigitals[i] && pdigitals[i]->pin == pin_number) {
					pdigitals[i]->setPin(pin_number);
					pdigitals[i]->pinMode(OUTPUT);
					pdigitals[i]->digitalWrite(LOW);
					break;
				}
			}
			SERIAL_PGM(MSG_BEGIN);
			SERIAL_PGM("M42");
			SERIAL_PGMLN(MSG_TERMINATE);
			SERIAL_PORT.flush();
		}
	  }
     break;
	 
	
	case 44: // M44 P<pin> [U] - -Read digital pin with optional pullup
        pin_number = -1;
        if (code_seen('P')) {
          pin_number = code_value();
		}
    	if( assignPin(pin_number) ) {
			dpin = new Digital(pin_number);
			if(code_seen('U')) {
				dpin->pinMode(INPUT_PULLUP);
			}
			int res = dpin->digitalRead();
			unassignPin(pin_number); // reset it since this is a one-shot
			SERIAL_PGM(MSG_BEGIN);
			SERIAL_PGM(digitalPinHdr);
			SERIAL_PGMLN(MSG_DELIMIT);
			SERIAL_PGM("1 ");
			SERIAL_PORT.println(pin_number);
			SERIAL_PGM("2 ");
			SERIAL_PORT.println(res);
			SERIAL_PGM(MSG_BEGIN);
			SERIAL_PGM(digitalPinHdr);
			SERIAL_PGMLN(MSG_TERMINATE);
			SERIAL_PORT.flush();
			delete dpin;
		}
		break;
		
	 // PWM value between 0 and 255, default timer mode is 2; clear on match, default resolution is 8 bits, default prescale is 1
	 // Prescale: 1,2,4,6,7,8,9 = none, 8, 64, 256, 1024, external falling, external rising
	 // Use M445 to disable pin permanently or use timer more 0 to stop pulse without removing pin assignment
     case 45: // M45 - set up PWM P<pin> S<power val 0-255> [T<timer mode 0-3>] [R<resolution 8,9,10 bits>] [X<prescale 0-7>]
	  pin_number = -1;
	  if(code_seen('P') ) {
          pin_number = code_value();
	  } else {
		 break;
	  }
      if (code_seen('S')) {
        int pin_status = code_value();
		int timer_mode = 2;
		timer_res = 8;
		timer_pre = 1;
		
		if( pin_status < 0 || pin_status > 255) {
			pin_status = 0;
		}
		// this is a semi-permanent pin assignment so dont add if its already assigned
		if( assignPin(pin_number) ) {
			// timer mode 0-3: 0 stop, 1 toggle on compare match, 2 clear on match, 3 set on match (see HardwareTimer)
			if( code_seen('T') ) {
				timer_mode = code_value();
				if( timer_mode < 0 || timer_mode > 3 ) {
					timer_mode = 0;
				}
			}
			// timer bit resolution 8,9, or 10 bits
			if( code_seen('R')) {
				timer_res = code_value();
				if( timer_res < 8 || timer_res > 10 ) {
					timer_res = 8;
				}
			}
			// X - prescale 0-7 for power of 2
			if( code_seen('X') ) {
				timer_pre = code_value();
				if( timer_pre < 0 || timer_pre > 7 ) {
					timer_pre = 0;
				}
			}
			for(int i = 0; i < 12; i++) {
				if(ppwms[i] == NULL) {
					ppin = new PWM(pin_number);
					ppin->init(pin_number);
					ppin->setPWMPrescale(timer_pre);
					ppin->setPWMResolution(timer_res);
					ppin->pwmWrite(pin_status,timer_mode); // default is 2, clear on match. to turn off, use 0
					ppwms[i] = ppin;
					SERIAL_PGM(MSG_BEGIN);
					SERIAL_PGM("M45");
					SERIAL_PGMLN(MSG_TERMINATE);
					SERIAL_PORT.flush();
					break;
				}
			}
		 } else { // reassign pin with new mode and value
			 for(int i = 0; i < 12; i++) {
				 if(ppwms[i] && ppwms[i]->pin == pin_number) {
					 // timer mode 0-3: 0 stop, 1 toggle on compare match, 2 clear on match, 3 set on match (see HardwareTimer)
					 if( code_seen('T') ) {
						 timer_mode = code_value();
						 if( timer_mode < 0 || timer_mode > 3 ) {
							timer_mode = 2; // mess up the code get clear on match default
						 }
					 }
					 //ppwms[i]->init();
					 ppwms[i]->pwmWrite(pin_status, timer_mode);
					 SERIAL_PGM(MSG_BEGIN);
					 SERIAL_PGM("M45");
					 SERIAL_PGMLN(MSG_TERMINATE);
					 SERIAL_PORT.flush();
					 break;
				 }
			 }
		 }
      }
	  break;
	  
	  case 46: // M46 -Read analog pin P<pin>
        pin_number = -1;
        if (code_seen('P')) {
          pin_number = code_value();
			if( assignPin(pin_number) ) {
				apin = new Analog(pin_number);
				int res = apin->analogRead();
				res = apin->analogRead(); // de-jitter
				unassignPin(pin_number);
				SERIAL_PGM(MSG_BEGIN);
				SERIAL_PGM(analogPinHdr);
				SERIAL_PGMLN(MSG_DELIMIT);
				SERIAL_PGM("1 ");
				SERIAL_PORT.println(pin_number);
				SERIAL_PGM("2 ");
				SERIAL_PORT.println(res);
				SERIAL_PGM(MSG_BEGIN);
				SERIAL_PGM(analogPinHdr);
				SERIAL_PGMLN(MSG_TERMINATE);
				SERIAL_PORT.flush();
				delete apin;
			}
		}
     break;
	 
	 case 47: // M47 -Read analog pin P<pin> T<threshold> compare to battery threshold, if below, print battery message
	   pin_number = -1;
	   if (code_seen('P')) {
		   pin_number = code_value();
		   digitarg = code_seen('T') ? code_value() : 0;
		   if( assignPin(pin_number) ) {
			   apin = new Analog(pin_number);
			   int res = apin->analogRead();
			   res = apin->analogRead(); // de-jitter
			   unassignPin(pin_number);
			   if( res < digitarg ) { // result < threshold is 0 by default
				   publishBatteryVolts(res);
			   } else {
				   SERIAL_PGM(MSG_BEGIN);
				   SERIAL_PGM("M47");
				   SERIAL_PGMLN(MSG_TERMINATE);
			   }
			   SERIAL_PORT.flush();
			   delete apin;
		   }
	 }
	 break;
	 
     case 80: // M80 - Turn on Power Supply
	  #if defined(PS_ON_PIN) && PS_ON_PIN > -1
        SET_OUTPUT(PS_ON_PIN); //GND
        WRITE(PS_ON_PIN, PS_ON_AWAKE);
        // If you have a switch on suicide pin, this is useful
        #if defined(SUICIDE_PIN) && SUICIDE_PIN > -1
            SET_OUTPUT(SUICIDE_PIN);
            WRITE(SUICIDE_PIN, HIGH);
        #endif
      #endif
	  	SERIAL_PGM(MSG_BEGIN);
	  	SERIAL_PGM("M80");
	  	SERIAL_PGMLN(MSG_TERMINATE);
	  	SERIAL_PORT.flush();
	  break;

     case 81: // M81 [Z<slot>] X - Turn off Power Z shut down motorcontroller in slot, X shut down PWM, slot -1 do all
	  int scode;
	  if( code_seen('Z')) {
		scode = code_value();
		if(scode == -1) {
			if(code_seen('X')) {
				for(int k = 0; k < 10; k++) {
					if(pwmControl[k]) {
						if(pwmControl[k]->isConnected()) {
							pwmControl[k]->commandEmergencyStop(81);
						}
					}
				}
			} else {
				for(int k = 0; k < 10; k++) {
					if(motorControl[k]) {
						if(motorControl[k]->isConnected()) {
							motorControl[k]->commandEmergencyStop(81);
						}
					}
				}
			}
		} else {
			motorController = scode; // slot seen
			if(code_seen('X')) {
				if(pwmControl[motorController]) {
					if(pwmControl[motorController]->isConnected()) {
						pwmControl[motorController]->commandEmergencyStop(81);
					}
				}
			} else {			
				if(motorControl[motorController]) {
					if( motorControl[motorController]->isConnected()) {
						motorControl[motorController]->commandEmergencyStop(81);
					}
				}
			}
		}
	  }
      #if defined(SUICIDE_PIN) && SUICIDE_PIN > -1
        suicide();
      #elif defined(PS_ON_PIN) && PS_ON_PIN > -1
        SET_OUTPUT(PS_ON_PIN);
        WRITE(PS_ON_PIN, PS_ON_ASLEEP);
      #endif
	    _delay_ms(1000); // Wait 1 sec before switch off
		SERIAL_PGM(MSG_BEGIN);
		SERIAL_PGM("M81");
		SERIAL_PGMLN(MSG_TERMINATE);
		SERIAL_PORT.flush();
	  break;

	  
    case 115: // M115
	  SERIAL_PGM(MSG_BEGIN);
      SERIAL_PGM(MSG_M115_REPORT);
	  SERIAL_PGMLN(MSG_DELIMIT);
	  SERIAL_PGMLN(MSG_115_REPORT2);
	  SERIAL_PGM(MSG_BEGIN);
	  SERIAL_PGM(MSG_M115_REPORT);
	  SERIAL_PGMLN(MSG_TERMINATE);
	  SERIAL_PORT.flush();
      break;
	  
	
    case 300: // M300 - emit ultrasonic pulse on given pin and return duration P<pin number>
      uspin = code_seen('P') ? code_value() : 0;
      if (uspin > 0) {
		Ultrasonic* upin = new Ultrasonic(uspin);
		pin_number = upin->getPin();
		SERIAL_PGM(MSG_BEGIN);
		SERIAL_PGM(sonicCntrlHdr);
		SERIAL_PGMLN(MSG_DELIMIT);
		SERIAL_PGM("1 "); // pin
		SERIAL_PORT.println(pin_number);
		SERIAL_PGM("2 "); // sequence
		SERIAL_PORT.println(upin->getRange()); // range
		SERIAL_PGM(MSG_BEGIN);
		SERIAL_PGM(sonicCntrlHdr);
		SERIAL_PGMLN(MSG_TERMINATE);
		SERIAL_PORT.flush();
		delete upin;
      }
    break;
		
    case 301: // M301 P<pin> - attach ultrasonic device to pin
		// wont assign pin 0 as its sensitive
		uspin = code_seen('P') ? code_value() : 0;
		// this is a permanent pin assignment so dont add if its already assigned
		if( assignPin(uspin) ) {
			for(int i = 0; i < 10; i++) {
				if(!psonics[i]) {
					psonics[i] = new Ultrasonic(uspin);
					SERIAL_PGM(MSG_BEGIN);
					SERIAL_PGM("M301");
					SERIAL_PGMLN(MSG_TERMINATE);
					SERIAL_PORT.flush();
					break;
				}
			}
		}
		break;
	
	case 302: // M302 P<pin> - remove ultrasonic pin
		uspin = code_seen('P') ? code_value() : 0;
		unassignPin(uspin);
		for(int i = 0; i < 10; i++) {
				if(psonics[i] && psonics[i]->pin->pin == uspin) {
					delete psonics[i];
					psonics[i] = 0;
					SERIAL_PGM(MSG_BEGIN);
					SERIAL_PGM("M302");
					SERIAL_PGMLN(MSG_TERMINATE);
					SERIAL_PORT.flush();
					break;
				}
		}
      break;
	
	case 303: // M303 - Check the analog inputs for all pins defined by successive M304 directives. Generate a read and output data if in range.
		for(int i = 0 ; i < 16; i++) {
			if( panalogs[i] && panalogs[i]->mode == INPUT) {
				printAnalog(panalogs[i], i);
				SERIAL_PORT.flush();
			}
		}
      break;
	  
	case 304:// M304 P<pin> [L<min>] [H<max>] [U] - toggle analog read optional INPUT_PULLUP with optional exclusion range 0-1024 via L<min> H<max>
		// if optional L and H values exclude readings in that range
		uspin = code_seen('P') ? code_value() : 0;
		// this is a permanent pin assignment so dont add if its already assigned
		if( assignPin(uspin) ) {
			for(int i = 0; i < 16; i++) {
				if(!panalogs[i]) {
					analogRanges[0][i] = code_seen('L') ? code_value() : 0;
					analogRanges[1][i] = code_seen('H') ? code_value() : 0;
					panalogs[i] = new Analog(uspin);
					if(code_seen('U'))  {
						panalogs[i]->pinMode(INPUT_PULLUP);
						SERIAL_PGM(MSG_BEGIN);
						SERIAL_PGM("M304");
						SERIAL_PGMLN(MSG_TERMINATE);
						SERIAL_PORT.flush();
					}
					break;
				}
			}
		} else { // reassign values for assigned pin
			for(int i = 0; i < 16; i++) {
				if(panalogs[i] && panalogs[i]->pin == uspin) {
					analogRanges[0][i] = code_seen('L') ? code_value() : 0;
					analogRanges[1][i] = code_seen('H') ? code_value() : 0;
					SERIAL_PGM(MSG_BEGIN);
					SERIAL_PGM("M304");
					SERIAL_PGMLN(MSG_TERMINATE);
					SERIAL_PORT.flush();
					break;
				}
			}
		}
      break;
	
	case 305: // M305 - Read the pins defined in M306 and output them if they are of the defined target value
		for(int i = 0 ; i < 32; i++) {
			if( pdigitals[i] && (pdigitals[i]->mode == INPUT || pdigitals[i]->mode == INPUT_PULLUP)) {
				printDigital(pdigitals[i], digitalTarget[i]);
				SERIAL_PORT.flush();
			}
		}
      break;
	
	case 306://  M306 P<pin> T<target> [U] - toggle digital read, 0 or 1 for target value, default 0 optional INPUT_PULLUP 
		// Looks for target value, if so publish with <digitalpin> header and 1 - pin 2 - value
		uspin = code_seen('P') ? code_value() : 0;
		digitarg = code_seen('T') ? code_value() : 0;
		// this is a permanent pin assignment so dont add if its already assigned
		if( assignPin(uspin) ) {
			for(int i = 0; i < 32; i++) {
				if(!pdigitals[i]) {
					pdigitals[i] = new Digital(uspin);
					if(code_seen('U')) {
						pdigitals[i]->pinMode(INPUT_PULLUP);
					}
					digitalTarget[i] = digitarg;
					SERIAL_PGM(MSG_BEGIN);
					SERIAL_PGM("M306");
					SERIAL_PGMLN(MSG_TERMINATE);
					SERIAL_PORT.flush();
					break;
				}
			}
		} else {
			for(int i = 0; i < 32; i++) {
				if(pdigitals[i] && pdigitals[i]->pin == uspin) {
					digitalTarget[i] = digitarg;
					SERIAL_PGM(MSG_BEGIN);
					SERIAL_PGM("M306");
					SERIAL_PGMLN(MSG_TERMINATE);
					SERIAL_PORT.flush();
					break;
				}
			}
		}
		break;
		
	case 445: // M445 P<pin> - Turn off pulsed write pin - disable PWM
      if(code_seen('P')) {
        pin_number = code_value();
		unassignPin(pin_number);
		for(int i = 0; i < 12; i++) {
			if(ppwms[i] && ppwms[i]->pin == pin_number) {
				ppwms[i]->pwmWrite(0,0); // default is 2, clear on match. to turn off, use 0 
				delete ppwms[i];
				ppwms[i] = 0;
				SERIAL_PGM(MSG_BEGIN);
				SERIAL_PGM("M445");
				SERIAL_PGMLN(MSG_TERMINATE);
				SERIAL_PORT.flush();
				break;
			}
		}
	  }
	  break;
	  
    case 500: // M500 Store settings in EEPROM
        Config_StoreSettings();
		SERIAL_PGM(MSG_BEGIN);
		SERIAL_PGM("M500");
		SERIAL_PGMLN(MSG_TERMINATE);
		SERIAL_PORT.flush();
    break;
	
    case 501: // M501 Read settings from EEPROM
        Config_RetrieveSettings();
		SERIAL_PGM(MSG_BEGIN);
		SERIAL_PGM("M501");
		SERIAL_PGMLN(MSG_TERMINATE);
		SERIAL_PORT.flush();
    break;
	
    case 502: // M502 Revert to default settings
        Config_ResetDefault();
		SERIAL_PGM(MSG_BEGIN);
		SERIAL_PGM("M502");
		SERIAL_PGMLN(MSG_TERMINATE);
		SERIAL_PORT.flush();
    break;
	
    case 503: // M503 print settings currently in memory
        Config_PrintSettings();
		SERIAL_PORT.flush();
    break;
	
	  
	case 700: // return stats
	  SERIAL_PGM(MSG_BEGIN);
	  SERIAL_PGM(MSG_STATUS);
	  SERIAL_PGMLN(MSG_DELIMIT);
	  // Check startup - does nothing if bootloader sets MCUSR to 0
	  mcu = MCUSR;
	  if(mcu & 1) {
		  SERIAL_PGMLN(MSG_POWERUP);
	  }
	  if(mcu & 2) {
		  SERIAL_PGMLN(MSG_EXTERNAL_RESET);
	  }
	  if(mcu & 4) {
		  SERIAL_PGMLN(MSG_BROWNOUT_RESET);
	  }
	  if(mcu & 8) {
		  SERIAL_PGMLN(MSG_WATCHDOG_RESET);
	  }
	  if(mcu & 32) {
		  SERIAL_PGMLN(MSG_SOFTWARE_RESET);
	  }
	  MCUSR=0;
	  SERIAL_PGM(VERSION_STRING);
	  #ifdef STRING_VERSION_CONFIG_H
	  #ifdef STRING_CONFIG_H_AUTHOR
	  SERIAL_PGM(MSG_CONFIGURATION_VER);
	  SERIAL_PGM(STRING_VERSION_CONFIG_H);
	  SERIAL_PGM(MSG_AUTHOR);
	  SERIAL_PGMLN(STRING_CONFIG_H_AUTHOR);
	  #endif
	  #endif
	  SERIAL_PGM(MSG_FREE_MEMORY);
	  SERIAL_PORT.println(freeMemory());
	  SERIAL_PGM(MSG_BEGIN);
	  SERIAL_PGM(MSG_STATUS);
	  SERIAL_PGMLN(MSG_TERMINATE);
	  SERIAL_PORT.flush();
	  break; 
	  
	case 701: // Report digital pins in use
		SERIAL_PGM(MSG_BEGIN);
		SERIAL_PGM(digitalPinSettingHdr);
		SERIAL_PGMLN(MSG_DELIMIT);
		for(int i = 0; i < 32; i++) {
			if( pdigitals[i] ) {
				SERIAL_PORT.print(pdigitals[i]->pin);
				switch(pdigitals[i]->mode) {
					case INPUT:
						SERIAL_PGMLN(" INPUT");
						break;
					case INPUT_PULLUP:
						SERIAL_PGMLN(" INPUT_PULLUP");
						break;
					case OUTPUT:
						SERIAL_PGMLN(" OUTPUT");
						break;
					default:
						SERIAL_PGMLN(" ERROR - UNKNOWN");
						break;
				}
			}
		}
		SERIAL_PGM(MSG_BEGIN);
		SERIAL_PGM(digitalPinSettingHdr);
		SERIAL_PGMLN(MSG_TERMINATE);
		SERIAL_PORT.flush();
		break;
		
	case 702: // Report analog pins in use
		SERIAL_PGM(MSG_BEGIN);
		SERIAL_PGM(analogPinSettingHdr);
		SERIAL_PGMLN(MSG_DELIMIT);
		for(int i = 0; i < 16; i++) {
			if( panalogs[i]  ) {
				SERIAL_PORT.print(panalogs[i]->pin);
				switch(panalogs[i]->mode) {
					case INPUT:
						SERIAL_PGMLN(" INPUT");
						break;
					case INPUT_PULLUP:
						SERIAL_PGMLN(" INPUT_PULLUP");
						break;
					case OUTPUT:
						SERIAL_PGMLN(" OUTPUT");
						break;
					default:
						SERIAL_PGMLN(" ERROR - UNKNOWN");
						break;
				}
			}
		}
		SERIAL_PGM(MSG_BEGIN);
		SERIAL_PGM(analogPinSettingHdr);
		SERIAL_PGMLN(MSG_TERMINATE);
		SERIAL_PORT.flush();
		break;
		
	case 703: // Report ultrasonic pins in use
		SERIAL_PGM(MSG_BEGIN);
		SERIAL_PGM(ultrasonicPinSettingHdr);
		SERIAL_PGMLN(MSG_DELIMIT);
		for(int i = 0; i < 10; i++) {
			if( psonics[i] ) {
				SERIAL_PGM("Pin:");
				SERIAL_PORT.println(psonics[i]->getPin());
			}
		}
		SERIAL_PGM(MSG_BEGIN);
		SERIAL_PGM(ultrasonicPinSettingHdr);
		SERIAL_PGMLN(MSG_TERMINATE);
		SERIAL_PORT.flush();
		break;
		
	case 704: // Report PWM pins in use
		SERIAL_PGM(MSG_BEGIN);
		SERIAL_PGM(pwmPinSettingHdr);
		SERIAL_PGMLN(MSG_DELIMIT);
		for(int i = 0; i < 12; i++) {
			if( ppwms[i] ) {
				SERIAL_PGM("Pin:");
				SERIAL_PORT.print(ppwms[i]->pin);
				SERIAL_PGM(" Timer channel:");
				SERIAL_PORT.print(ppwms[i]->channel);
				switch(ppwms[i]->mode) {
					case INPUT:
						SERIAL_PGM(" INPUT");
						break;
					case INPUT_PULLUP:
						SERIAL_PGM(" INPUT_PULLUP");
						break;
					case OUTPUT:
						SERIAL_PGM(" OUTPUT");
						break;
					default:
						SERIAL_PGM(" ERROR - UNKNOWN");
						break;
				}
				SERIAL_PORT.println();
			}
		}
		SERIAL_PGM(MSG_BEGIN);
		SERIAL_PGM(pwmPinSettingHdr);
		SERIAL_PGMLN(MSG_TERMINATE);
		SERIAL_PORT.flush();
		break;
		
	case 705:
			SERIAL_PGM(MSG_BEGIN);
			SERIAL_PGM(motorControlSettingHdr);
			SERIAL_PGMLN(MSG_DELIMIT);
			for(int j = 0; j < 10; j++) {
				if( motorControl[j] ) {
					for(int i = 0 ; i < motorControl[j]->getChannels(); i++) { //per channel
						SERIAL_PGM("Motor channel:");
						SERIAL_PORT.print(i+1);
						SERIAL_PGM(" Min Power:");
						SERIAL_PORT.print(motorControl[j]->getMinMotorPower(i+1));
						SERIAL_PGM(" Speed:");
						SERIAL_PORT.print(motorControl[j]->getMotorSpeed(i+1));
						SERIAL_PGM(" Curr. Dir:");
						SERIAL_PORT.print(motorControl[j]->getCurrentDirection(i+1));
						SERIAL_PGM(" Default. Dir:");
						SERIAL_PORT.println(motorControl[j]->getDefaultDirection(i+1));
						SERIAL_PGM(" Encoder Pin:");
						if(motorControl[j]->getWheelEncoder(i+1)) {
							SERIAL_PORT.print(motorControl[j]->getWheelEncoder(i+1)->pin);
							SERIAL_PGM(" Count:");
							SERIAL_PORT.print(motorControl[j]->getEncoderCount(i+1));
							SERIAL_PGM(" Duration:");
							SERIAL_PORT.println(motorControl[j]->getMaxMotorDuration(i+1));
							//SERIAL_PGM(motorControl[j]->getDriverInfo(i+1));
						} else {
							SERIAL_PGMLN("None.");
						}
					}
					SERIAL_PGM("Ultrasonic pins:");
					if( motorControl[j]->totalUltrasonics() ) {
						SERIAL_PORT.println(motorControl[j]->totalUltrasonics());
						for(int k = 0; k < motorControl[j]->totalUltrasonics(); k++) {
							SERIAL_PGM("Pin:");
							SERIAL_PORT.print(psonics[motorControl[j]->getUltrasonicIndex(k+1)]->getPin());
							SERIAL_PGM(" Facing:");
							SERIAL_PORT.print(motorControl[j]->getUltrasonicFacing(k+1));
							SERIAL_PGM(" Shutdown cm:");
							SERIAL_PORT.println(motorControl[j]->getMinMotorDist(k+1));
						}
					} else {
						SERIAL_PGMLN("None.");
					}
				} // if motorControl[j]
			} // j each motor controller
			SERIAL_PGM(MSG_BEGIN);
			SERIAL_PGM(motorControlSettingHdr);
			SERIAL_PGMLN(MSG_TERMINATE);
			SERIAL_PORT.flush();
			//
			// PWM control
			//
			SERIAL_PGM(MSG_BEGIN);
			SERIAL_PGM(pwmControlSettingHdr);
			SERIAL_PGMLN(MSG_DELIMIT);
			for(int j = 0; j < 10; j++) {
				if(pwmControl[j]) {
					for(int i = 0 ; i < pwmControl[j]->getChannels(); i++) { //per channel
						SERIAL_PGM("PWM channel:");
						SERIAL_PORT.print(i+1);
						SERIAL_PGM(" Min Level:");
						SERIAL_PORT.print(pwmControl[j]->getMinPWMLevel(i+1));
						SERIAL_PGM(" Duration:");
						SERIAL_PORT.println(pwmControl[j]->getMaxPWMDuration(i+1));
					}
				}
			} // j each PWM controller
			SERIAL_PGM(MSG_BEGIN);
			SERIAL_PGM(pwmControlSettingHdr);
			SERIAL_PGMLN(MSG_TERMINATE);
			SERIAL_PORT.flush();
			break;
			
	case 706: // Report all pins in use
		SERIAL_PGM(MSG_BEGIN);
		SERIAL_PGM(pinSettingHdr);
		SERIAL_PGMLN(MSG_DELIMIT);
		for(int i = 0; i < 100; i++) {
			if( pinAssignment(i) == PIN_ASSIGNED ) {
				SERIAL_PORT.print(i);
				SERIAL_PORT.print(',');
			}
		}
		Serial.println();
		SERIAL_PGM(MSG_BEGIN);
		SERIAL_PGM(pinSettingHdr);
		SERIAL_PGMLN(MSG_TERMINATE);
		SERIAL_PORT.flush();
		break;
		
	case 798: // M798 Z<motor control> [X] Report controller status for given controller. If X, slot is PWM
		char* buf;
		SERIAL_PGM(MSG_BEGIN);
		SERIAL_PGM(controllerStatusHdr);
		SERIAL_PGMLN(MSG_DELIMIT);
		if (code_seen('Z')) {
			motorController = code_value();
		}
		
		if(code_seen('X')) {
				if(pwmControl[motorController]) {
					for(int i = 0; i < pwmControl[motorController]->getChannels() ; i++ ) {
						SERIAL_PGM("PWM Channel:");
						SERIAL_PORT.println(i+1);
						pwmControl[motorController]->getDriverInfo(i+1,outbuffer);
						//SERIAL_PORT.println(outbuffer);
						char* buf = outbuffer;
						while(*buf){
								Serial.print(*buf);
								buf++;
						}
						Serial.println();
					}
				}
		} else {
			if( motorControl[motorController] && motorControl[motorController]->isConnected() ) {
				for(int i = 0; i < motorControl[motorController]->getChannels() ; i++ ) {
					SERIAL_PGM("Motor Channel:");
					SERIAL_PORT.println(i+1);
					motorControl[motorController]->getDriverInfo(i+1, outbuffer);
					//SERIAL_PORT.println(outbuffer);
					char* buf = outbuffer;
					while(*buf){
						 Serial.print(*buf);
						 buf++;
					}
					Serial.println();
				}
			}
		} // code_seen('X')
		SERIAL_PGM(MSG_BEGIN);
		SERIAL_PGM(controllerStatusHdr);
		SERIAL_PGMLN(MSG_TERMINATE);
		SERIAL_PORT.flush();
		break;	
		
	case 799: // M799 [Z<controller>][X] Reset controller, if no argument, reset all. If X, slot is PWM
		if (code_seen('Z')) {
			motorController = code_value();
			if(code_seen('X')) {
				if(pwmControl[motorController]) {
					pwmControl[motorController]->commandEmergencyStop(799);
					SERIAL_PGM(MSG_BEGIN);
					SERIAL_PGM("M799");
					SERIAL_PGMLN(MSG_TERMINATE);
					SERIAL_PORT.flush();
				}
			} else {
				if(motorControl[motorController]) {
					motorControl[motorController]->commandEmergencyStop(799);
					SERIAL_PGM(MSG_BEGIN);
					SERIAL_PGM("M799");
					SERIAL_PGMLN(MSG_TERMINATE);
					SERIAL_PORT.flush();
				}
			}
		} else { // no slot defined, do them all if present
			for(int j = 0;j < 10; j++) {
				if(code_seen('X')) {
					if(pwmControl[j]) {
						pwmControl[j]->commandEmergencyStop(-1);
					}
				} else {
					if(motorControl[j]) {
						motorControl[j]->commandEmergencyStop(-1);
					}
				}
			}
			SERIAL_PGM(MSG_BEGIN);
			SERIAL_PGM("M799");
			SERIAL_PGMLN(MSG_TERMINATE);
			SERIAL_PORT.flush();
		}
		break;		
		
			
	case 802: // Acquire analog pin data M802 Pnn Sxxx Mxxx P=Pin number, S=number readings, M=microseconds per reading. X - pullup.
		// Publish <dataset> 1 - pin, 2 - reading
		if( code_seen('P')) {
			apin = new Analog((uint8_t)code_value());
			if( code_seen('X') ) {
				apin->pinMode(INPUT_PULLUP);
			} else {
				apin->pinMode(INPUT);
			}
		}
		nread = 0;
		if( code_seen('S') ) {
			nread = code_value();
		}
		micros = 0;
		if( code_seen('M')) {
			micros = (uint32_t)code_value();
		}
		values = new int(nread);
		for(int i = 0; i < nread; i++) {
			*(values+i) = apin->analogRead();
			for(int j = 0; j < micros; j++) _delay_us(1);
		}
		SERIAL_PGM(MSG_BEGIN);
		SERIAL_PGM(analogPinHdr);
		SERIAL_PGMLN(MSG_DELIMIT);
		for(int i = 0; i < nread; i++) {
			SERIAL_PORT.print(i+1); // sequence
			SERIAL_PORT.print(' ');
			// 0 element is pin number
			if( i == 0 ) {
				SERIAL_PORT.println(apin->pin);
			} else {
				SERIAL_PORT.println(*(values+i)); // value
			}
		}
		delete values;
		SERIAL_PGM(MSG_BEGIN);
		SERIAL_PGM(analogPinHdr);
		SERIAL_PGMLN(MSG_TERMINATE);
		SERIAL_PORT.flush();
		break;	
		
		
    case 999: // M999: Reset
		Stopped = false;
		//lcd_reset_alert_level();
		gcode_LastN = Stopped_gcode_LastN;
		//FlushSerialRequestResend();
		if( watchdog_timer != NULL ) {
			delete watchdog_timer;
		}
		watchdog_timer = new WatchdogTimer();
		SERIAL_PGM(MSG_BEGIN);
		SERIAL_PGM("M999");
		SERIAL_PGMLN(MSG_TERMINATE);
		SERIAL_PORT.flush();
		watchdog_timer->watchdog_init(15); // 15 ms
		break;
		
	default:
		int ibuf = 0;
		SERIAL_PGM(MSG_BEGIN);
		SERIAL_PGM(MSG_UNKNOWN_MCODE);
		while(cmdbuffer[ibuf]) SERIAL_PORT.print(cmdbuffer[ibuf++]);
		SERIAL_PGMLN(MSG_TERMINATE);
		SERIAL_PORT.flush();
		break;
	
  } // switch m code

} //processMCode

void FlushSerialRequestResend()
{
  //char cmdbuffer[bufindr][100]="Resend:";
  SERIAL_PORT.flush();
  SERIAL_PGMLN(MSG_RESEND);
  SERIAL_PORT.println(gcode_LastN + 1);
  SERIAL_PORT.flush();
}


/*---------------------------------------------------
* Arrive here at the end of each command processing iteration to check for status related events
* ---------------------------------------------------
*/
void manage_inactivity() {
  // check motor controllers
  for(int j =0; j < 10; j++) {
	  if(motorControl[j]) {
		if( motorControl[j]->isConnected() ) {
			motorControl[j]->checkEncoderShutdown();
			motorControl[j]->checkUltrasonicShutdown();
			if( motorControl[j]->queryFaultFlag() != fault ) {
				fault = motorControl[j]->queryFaultFlag();
				publishMotorFaultCode(fault);
				SERIAL_PORT.flush();
			}
		}
	  }
  }
  
  if( realtime_output ) {		
	// Check the ultrasonic ranging for all devices defined by successive M301 directives
	for(int i = 0 ; i < 10; i++) {
		if( psonics[i] ) {
			printUltrasonic(psonics[i], i);
			SERIAL_PORT.flush();
		}
	}  
  }// realtime output
}

void kill() {
  cli(); // Stop interrupts
#if defined(PS_ON_PIN) && PS_ON_PIN > -1
  Digital psoPin = new Digital(PS_ON_PIN);
  psoPin.pinMode(INPUT);
#endif
  for(int j = 0; j < 10; j++) {
	motorControl[j]->commandEmergencyStop(-2);
  }
  //SERIAL_ERROR_START;
  //SERIAL_ERRORLNPGM(MSG_ERR_KILLED);
  suicide();
  while(1) { /* Intentionally left empty */ } // Wait for reset
}

void Stop() {
  if(!Stopped) {
    Stopped = true;
    Stopped_gcode_LastN = gcode_LastN; // Save last g_code for restart
	for(int i = 0; i < 10; i++)
		if(motorControl[i])
			motorControl[i]->commandEmergencyStop(-3);
    //SERIAL_ERROR_START;
    //SERIAL_ERRORLNPGM(MSG_ERR_STOPPED);
  }
}
/*
* The fault code is bit-ordered for 8 cases
*/
void publishMotorFaultCode(int fault) {
	uint8_t bfault = 0;
	uint8_t j = 1;
	SERIAL_PGM(MSG_BEGIN);
	SERIAL_PGM(motorFaultCntrlHdr);
	SERIAL_PGMLN(MSG_DELIMIT);
	for(int i = 0; i < 8 ; i++) {
		bfault = fault & (1<<i);
		switch(bfault) {
			default:
			case 0: // bit not set
				break;
			case 1:
				SERIAL_PORT.print(j++);
				SERIAL_PORT.print(' ');
				SERIAL_PGMLN(MSG_MOTORCONTROL_1);
				break;
			case 2:
				SERIAL_PORT.print(j++);
				SERIAL_PORT.print(' ');
				SERIAL_PGMLN(MSG_MOTORCONTROL_2);
				break;
			case 4:
				SERIAL_PORT.print(j++);
				SERIAL_PORT.print(' ');
				SERIAL_PGMLN(MSG_MOTORCONTROL_3);
				break;
			case 8:
				SERIAL_PORT.print(j++);
				SERIAL_PORT.print(' ');
				SERIAL_PGMLN(MSG_MOTORCONTROL_4);
				break;
			case 16:
				SERIAL_PORT.print(j++);
				SERIAL_PORT.print(' ');
				SERIAL_PGMLN(MSG_MOTORCONTROL_5);
				break;
			case 32:
				SERIAL_PORT.print(j++);
				SERIAL_PORT.print(' ');
				SERIAL_PGMLN(MSG_MOTORCONTROL_6);
				break;
			case 64:
				SERIAL_PORT.print(j++);
				SERIAL_PORT.print(' ');
				SERIAL_PGMLN(MSG_MOTORCONTROL_7);
				break;
			case 128:
				SERIAL_PORT.print(j++);
				SERIAL_PORT.print(' ');
				SERIAL_PGMLN(MSG_MOTORCONTROL_8);
				break;
		}
	}
	SERIAL_PGM(MSG_BEGIN);
	SERIAL_PGM(motorFaultCntrlHdr);
	SERIAL_PGMLN(MSG_TERMINATE);
}
/*
* Deliver the battery voltage from smart controller
*/
void publishBatteryVolts(int volts) {
	SERIAL_PGM(MSG_BEGIN);
	SERIAL_PGM(batteryCntrlHdr);
	SERIAL_PGMLN(MSG_DELIMIT);
	SERIAL_PGM("1 ");
	SERIAL_PORT.println(volts);
	SERIAL_PGM(MSG_BEGIN);
	SERIAL_PGM(batteryCntrlHdr);
	SERIAL_PGMLN(MSG_TERMINATE);
}
/************************************************************************/
/* only call this if we know code is stall                              */
/************************************************************************/
void publishMotorStatCode(int stat) {
	SERIAL_PGM(MSG_BEGIN);
	SERIAL_PGM(motorFaultCntrlHdr);
	SERIAL_PGMLN(MSG_DELIMIT);
	SERIAL_PGM("1 ");
	SERIAL_PGMLN(MSG_MOTORCONTROL_9);
	SERIAL_PGM(MSG_BEGIN);
	SERIAL_PGM(motorFaultCntrlHdr);
	SERIAL_PGMLN(MSG_TERMINATE);
}

/*
* Print the ultrasonic range
*/
void printUltrasonic(Ultrasonic* us, int index) {
		float range = us->getRange();
		uint8_t ultpin = us->getPin();
		if( range != sonicDist[index] ) {
			sonicDist[index] = range;
			SERIAL_PGM(MSG_BEGIN);
			SERIAL_PGM(sonicCntrlHdr);
			SERIAL_PGMLN(MSG_DELIMIT);
			SERIAL_PGM("1 "); // pin
			SERIAL_PORT.println(ultpin);
			SERIAL_PGM("2 "); // sequence
			SERIAL_PORT.println(range); // range
			SERIAL_PGM(MSG_BEGIN);
			SERIAL_PGM(sonicCntrlHdr);
			SERIAL_PGMLN(MSG_TERMINATE);
		}
}
/*
* If we have values in analogRanges for this pin, check the reading and if it is between these ranges
* reject the reading. This allows us to define a center or rest point for a joystick etc.
* If no values were specified on the M code invocation, ignore and process regardless of value.
*/
void printAnalog(Analog* apin, int index) {
	//pin = new Analog(upin);
	int nread = apin->analogRead();
	// jitter comp.
	nread = apin->analogRead();
	if( analogRanges[0][index] != 0 && nread >= analogRanges[0][index] && nread <= analogRanges[1][index])
		return;
	SERIAL_PGM(MSG_BEGIN);
	SERIAL_PGM(analogPinHdr);
	SERIAL_PGMLN(MSG_DELIMIT);
	SERIAL_PORT.print('1'); // sequence
	SERIAL_PORT.print(' ');
	// 0 element is pin number
	SERIAL_PORT.println(apin->pin);
	SERIAL_PORT.print('2'); // sequence
	SERIAL_PORT.print(' ');
	SERIAL_PORT.print(nread);
	SERIAL_PORT.println();
	SERIAL_PGM(MSG_BEGIN);
	SERIAL_PGM(analogPinHdr);
	SERIAL_PGMLN(MSG_TERMINATE);
	//delete pin;
}
/*
* 'target' represents the expected value. Two elements returned in sequence. 1 - Pin, 2 - reading
*/
void printDigital(Digital* dpin, int target) {
	//dpin = new Digital(upin);
	//dpin->pinMode(INPUT);
	int nread = dpin->digitalRead();
	//delete dpin;
	// look for activated value
	if( !(nread ^ target) ) {
		SERIAL_PGM(MSG_BEGIN);
		SERIAL_PGM(digitalPinHdr);
		SERIAL_PGMLN(MSG_DELIMIT);
		SERIAL_PORT.print('1'); // sequence
		SERIAL_PORT.print(' ');
		SERIAL_PORT.println(dpin->pin);
		SERIAL_PORT.print('2'); // sequence
		SERIAL_PORT.print(' ');
		SERIAL_PORT.println(nread);
		SERIAL_PGM(MSG_BEGIN);
		SERIAL_PGM(digitalPinHdr);
		SERIAL_PGMLN(MSG_TERMINATE);
	}
}

