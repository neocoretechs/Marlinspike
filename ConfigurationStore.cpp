#include "RoboCore.h"
#include "ConfigurationStore.h"

void _EEPROM_writeData(int &pos, uint8_t* value, uint8_t size)
{
    do {
        eeprom_write_byte((unsigned char*)pos, *value);
        pos++;
        value++;
    } while(--size);
}
#define EEPROM_WRITE_VAR(pos, value) _EEPROM_writeData(pos, (uint8_t*)&value, sizeof(value))
void _EEPROM_readData(int &pos, uint8_t* value, uint8_t size)
{
    do {
        *value = eeprom_read_byte((unsigned char*)pos);
        pos++;
        value++;
    } while(--size);
}
#define EEPROM_READ_VAR(pos, value) _EEPROM_readData(pos, (uint8_t*)&value, sizeof(value))
//======================================================================================

#define EEPROM_OFFSET 100

// IMPORTANT:  Whenever there are changes made to the variables stored in EEPROM
// in the functions below, also increment the version number. This makes sure that
// the default values are used whenever there is a change to the data, to prevent
// wrong data being written to the variables.
// ALSO:  always make sure the variables in the Store and retrieve sections are in the same order.
#define EEPROM_VERSION "V10"

void Config_StoreSettings() 
{
  char ver[4]= "000";
  int i=EEPROM_OFFSET;
  EEPROM_WRITE_VAR(i,ver); // invalidate data first  
  //EEPROM_WRITE_VAR(i,max_acceleration_units_per_sq_second);
  //EEPROM_WRITE_VAR(i,acceleration);
  char ver2[4]=EEPROM_VERSION;
  i=EEPROM_OFFSET;
  EEPROM_WRITE_VAR(i,ver2); // validate data
}

void Config_PrintSettings()

{  // Always have this function, even with EEPROM_SETTINGS disabled, the current values will be shown
	SERIAL_PGM(MSG_BEGIN);
	SERIAL_PGM(eepromHdr);
	SERIAL_PGMLN(MSG_DELIMIT);
    SERIAL_PGMLN("1 ");
    //SERIAL_ECHOPAIR("  M201 X" ,max_acceleration_units_per_sq_second[0] ); 
    SERIAL_PGMLN("2 ");
    //SERIAL_ECHOPAIR("  M204 S",acceleration );
	SERIAL_PGM(MSG_BEGIN);
	SERIAL_PGM(eepromHdr);
	SERIAL_PGMLN(MSG_TERMINATE);
} 

void Config_RetrieveSettings()
{
    int i=EEPROM_OFFSET;
    char stored_ver[4];
    char ver[4]=EEPROM_VERSION;
    EEPROM_READ_VAR(i,stored_ver); //read stored version
    //  SERIAL_ECHOLN("Version: [" << ver << "] Stored version: [" << stored_ver << "]");
    if (strncmp(ver,stored_ver,3) == 0)
    {
        // version number match
        //EEPROM_READ_VAR(i,axis_steps_per_unit);  
        //EEPROM_READ_VAR(i,max_acceleration_units_per_sq_second);
        
        // steps per sq second need to be updated to agree with the units per sq second (as they are what is used in the planner)
		//reset_acceleration_rates();
        
        //EEPROM_READ_VAR(i,acceleration);
        //EEPROM_READ_VAR(i,minsegmenttime);
        //#ifndef PIDTEMP
        //float Kp,Ki,Kd;
        //#endif
        // do not need to scale PID values as the values in EEPROM are already scaled		
        //EEPROM_READ_VAR(i,Kp);
        //EEPROM_READ_VAR(i,Ki);
        //EEPROM_READ_VAR(i,Kd);

		// Call updatePID (similar to when we have processed M301)
		//updatePID();
        //SERIAL_ECHO_START;
        //SERIAL_ECHOLNPGM("Stored settings retrieved");
    }
    else
    {
        Config_ResetDefault();
    }
}

void Config_ResetDefault()
{
    //float tmp1[]=DEFAULT_AXIS_STEPS_PER_UNIT;
    //float tmp2[]=DEFAULT_MAX_FEEDRATE;
    //long tmp3[]=DEFAULT_MAX_ACCELERATION;
    //for (short i=0;i<4;i++) 
    //{
        //axis_steps_per_unit[i]=tmp1[i];  
        //max_acceleration_units_per_sq_second[i]=tmp3[i];
    //}
    
    // steps per sq second need to be updated to agree with the units per sq second
    //reset_acceleration_rates();
    
    //acceleration=DEFAULT_ACCELERATION;
    //minimumfeedrate=DEFAULT_MINIMUMFEEDRATE;
    //minsegmenttime=DEFAULT_MINSEGMENTTIME;       
    //mintravelfeedrate=DEFAULT_MINTRAVELFEEDRATE;
//SERIAL_ECHO_START;
//SERIAL_ECHOLNPGM("Hardcoded Default Settings Loaded");

}
