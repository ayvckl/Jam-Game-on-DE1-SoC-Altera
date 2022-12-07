/*
 * HPS_PrivateTimer.c
 *
 *  Created on: 30 Nis 2022
 *      Author: Beyza
 */


#include "HPS_PrivateTimer.h"
//Driver Base Addresses
volatile unsigned int *timer_base_ptr = 0x0; //0xFFFEC600
//Driver Initialised
bool timer_initialised = false;
//Register Offsets
#define TIMER_LOAD (0x00/sizeof(unsigned int))
#define TIMER_VALUE (0x04/sizeof(unsigned int))
#define TIMER_CONTROL (0x08/sizeof(unsigned int))
#define TIMER_INTERRUPT (0x0C/sizeof(unsigned int))
//Function to initialise the Timer
signed int Timer_initialise(unsigned int base_address){
	timer_base_ptr = (unsigned int *) base_address;
	timer_base_ptr[TIMER_CONTROL] = 0;
	timer_initialised = true; return TIMER_SUCCESS;
}
//Check if driver initialised
bool Timer_isInitialised() {
	return timer_initialised;
}
signed int Load_timer( unsigned int load_value) {
	//Sanity checks
	if (!Timer_isInitialised()) return TIMER_ERRORNOINIT;
	timer_base_ptr[TIMER_LOAD] = load_value;
	return TIMER_SUCCESS;
}

signed int Get_time( unsigned int timer_value) {
	//Sanity checks
	if (!Timer_isInitialised())return TIMER_ERRORNOINIT;
	timer_value = timer_base_ptr[TIMER_VALUE];
	return timer_value;

}
signed int Control_timer( unsigned int prescaler, unsigned int interrupt, unsigned int reload, unsigned int enable) {
	//Sanity checks
	if (!Timer_isInitialised()) return TIMER_ERRORNOINIT;
	timer_base_ptr[TIMER_CONTROL] = (prescaler << 8) | (interrupt << 2) | (reload << 1) | (enable << 0);
	return TIMER_SUCCESS;
}
signed int Clear_interrupt() {
//Sanity checks
	if (!Timer_isInitialised()) return TIMER_ERRORNOINIT;
	if (timer_base_ptr[TIMER_INTERRUPT] & 0x1) {
		// If the timer interrupt flag is set, clear the flag
		timer_base_ptr[TIMER_INTERRUPT] = 0x1; }
	return TIMER_SUCCESS;
}
