/* * HPS Private Timer Controller Driver * ------------------------------------
 * * Description: * Driver for the HPS Private Timer Controller in the DE1-SoC Computer
 * * * * Company: University of Leeds  * */
#ifndef HPS_PrivateTimer_H_
#define HPS_PrivateTimer_H_
//Include required header files
#include <stdbool.h>
#include <stdBool.h>
//Boolean variable type "bool" and "true"/"false" constants.
//Error and Status Codes
#define TIMER_SUCCESS 0
#define TIMER_ERRORNOINIT -1

signed int Timer_initialise(unsigned int base_address);
bool Timer_isInitialised( void );
signed int Load_timer( unsigned int load_value);
signed int Get_time( unsigned int timer_value);
signed int Control_timer( unsigned int prescaler, unsigned int interrupt, unsigned int reload, unsigned int enable);
signed int Clear_interrupt();
#endif /*HPS_PrivateTimer_H_*/
