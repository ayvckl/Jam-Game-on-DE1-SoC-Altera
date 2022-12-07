/*
 * main.c
 *  Created on: 20 April 2022
 *     Author: Beyza AYVACIKLI
 * Description:
 * This is a game can be play with buttons and LT24 screen configration
 * Main aim of the game the carakter should be reach to jam and
 *  it shouldnt touch the enemy
 *  It has been designed based on two-dimensional games.
 * The images used are in the project folder.
 */

#include "DE1SoC_WM8731/DE1SoC_WM8731.h"
#include "HPS_Watchdog/HPS_Watchdog.h"
#include "DE1SoC_LT24/DE1SoC_LT24.h"
#include "HPS_usleep/HPS_usleep.h"
#include "DE1SoC_SevenSeg/DE1SoC_SevenSeg.h"  // Including Seven segment display driver
#include "HPS_PrivateTimer.h"	// Including HPS Private Timer driver

//Include Floating Point Math Libraries
#include "stdbool.h"
#include <math.h>


//Include our images
#include "start.h"    // Start screen of the game
#include "gameover.h"       // Final screen of the game
#include "background.h" //  background image
#include "enemy.h"   //30x30 for all enemy
#include "cute.h"
#include "explosion.h"
#include "jam.h"
#include "win.h"
#include "sound.h"

//Debugging Function
#include <stdlib.h>
void exitOnFail(signed int status, signed int successStatus){
    if (status != successStatus) {
        exit((int)status); //Add breakpoint here to catch failure
    }
}
// Select which displays to use.
#define DISPLAY_0 						0
#define DISPLAY_1 						2
#define DISPLAY_2 						4

#define MSECONDS_THRESHOLD 				562499
#define SECONDS_THRESHOLD 				56249999 //MSECONDS_THRESHOLD*100
#define MUNITE_THRESHOLD				3374999999//SECONDS_THRESHOLD*60
#define HOUR_THRESHOLD					202499999940//MUNITE_THRESHOLD*60

//Initialise Phase Accumulator
#define F_SAMPLE 								48000.0        //Sampling rate of WM8731 Codec (Do not change)
#define PI2      								6.28318530718  //2 x Pi      (Apple or Peach?)
//Value for timer
#define SECONDS_THRESHOLD 						56249999 //MSECONDS_THRESHOLD*100
/*
 * This is for to copy one buffer to other. It used for every time one bufer sending to the LT24
 */
unsigned int copytoBackgroundFrame(unsigned short* backgrounBuffer,const unsigned short* copyBuffer,
								   unsigned int copyBufferposx_initial, unsigned int copyBufferposy_initial,
								   unsigned int copyBuffersizey,unsigned int copyBuffersizex ) {
	unsigned int x;
	unsigned int y;
	for( y=copyBufferposx_initial;y<(copyBufferposx_initial+copyBuffersizex);y++ ){
    	for( x=copyBufferposy_initial;x<(copyBufferposy_initial+copyBuffersizey);x++ ){
    		backgrounBuffer[((x*240)+y)]=copyBuffer[(y-copyBufferposx_initial)+((x-copyBufferposy_initial)*copyBuffersizex)];
    		}
    	}
}
// For checking a overlaping or not
// 20 and 10 values should be develop
unsigned int overlapingCheck(unsigned int enemy1posx, unsigned int enemy1posy,
							 unsigned int enemy2posx, unsigned int enemy2posy,
							 unsigned int enemy2sizey) {
	// overlaping check
	if((((enemy1posx-enemy2posx)<20)
		||((enemy2posx-enemy1posx)<20))
		&&(((enemy1posy-enemy2posy)<(enemy2sizey-10)))){
		return 1;
	} else {
		return 0;
	}
}
//for last sending buffer to LT24
 unsigned short sendingLT24Buf [76800];
// Base address definiton for key buttons
volatile unsigned int *key_ptr = (unsigned int *)0xFF200050;

// Store the state of the keys last time we checked.
// This allows us to determine when a key is pressed, then released.
unsigned int key_last_state = 0;

// This from online platform!!!
unsigned int getPressedKeys() {
    // memory the value.
    unsigned int key_current_state = *key_ptr;
    // If the key was down last cycle, and is up now, mark as pressed.
    unsigned int keys_pressed = (~key_current_state) & (key_last_state);
    // Save the key state for comparing with the next state.
    key_last_state = key_current_state;
    // Return value.
    return keys_pressed;
}
//
// Main Function
//
int main(void) {

	//enemy objects for every row one pointer, and every time diffrent color enemy
	const unsigned short *row1enemy [10] =  {enemy, enemy2, enemy, enemy4,  enemy2, enemy, enemy4, enemy, enemy2, enemy};
	const unsigned short *row2enemy [10] =  {enemy2, enemy3, enemy, enemy3,  enemy, enemy4, enemy3, enemy3, enemy3, enemy3};
	const unsigned short *row3enemy [10] = {enemy, enemy2, enemy, enemy4,  enemy2, enemy, enemy4, enemy, enemy2, enemy};

	// Initial values of variables.
	// For understanding to key pressed.
    unsigned int keys_pressed = 0;
    unsigned int second = 0;		//seconds value for display

	// Initial position of the cute object and jam object
	unsigned int cuteposx = 60;
	unsigned int cuteposy = 250;
	unsigned int jamposx = 100;
	unsigned int jamposy = 30;

	// Initial x and y positions of enemys on row 1-2-3
	unsigned int rowenemyposy [3] = {30,110,180};
	unsigned int rowenemyposx [3] = {0,0,0};

	// Counter for the enemy buffer
	unsigned int i[4] = {0};

	// To define overlaping
	unsigned int overlaping;
	// To define overlaping on 1,2,3 row enemy
	unsigned int overlaping1;
	unsigned int overlaping2;
	unsigned int overlaping3;

	unsigned int winSituation;

    /* Local Variables */
	// for memorying to previous values for timer
	unsigned int previousTimerValue[3] = {0};

	/////////////////////-> Unit 3 Lab 3.1
	/* Pointers to peripherals */

	// BUTTONPRESSED Register to detect button releases
	volatile unsigned int *BUTTONPRESSED = (unsigned int *) 0xFF20005C;
	// //Pointers for ToneGenerator->lab3.1
    volatile unsigned char* fifospace_ptr;
    volatile unsigned int*  audio_left_ptr;
    volatile unsigned int*  audio_right_ptr;

    //Variables
    //Phase Accumulator
    double phase = 0.0;  // Phase accumulator
    double inc [16] = {0.0}; // Phase increment

    // A series of notes to create a simple tune-> They are random
    double freqValue [16] = { C4, CS4,  E4, F4, FS4, G4,D4, DS4, GS4,A4, AS4, B4, C5, CS5, DS5, D5,};

    double ampl  = 0.0;  // Tone amplitude (i.e. volume)
    signed int audio_sample = 0;

    // Counter values for relevant loops
    int j;
    int k;
    unsigned int life=5;
	int y;

    //Initialise the Audio Codec.->
    exitOnFail(WM8731_initialise(0xFF203040),WM8731_SUCCESS);  //Exit if not successful
    HPS_ResetWatchdog();
    //Clear both FIFOs
    WM8731_clearFIFO(true,true);
    //Grab the FIFO Space and Audio Channel Pointers
    fifospace_ptr = WM8731_getFIFOSpacePtr();
    audio_left_ptr = WM8731_getLeftFIFOPtr();
    audio_right_ptr = WM8731_getRightFIFOPtr();
    ampl  = 8388608.0; // Pick desired amplitude (e.g. 2^23). WARNING: If too high = deafening!
    phase = 0.0;

	// Define inc values for each note of tune
	for(j=0; j<16; j++){
		inc [j] = freqValue [j] * PI2 / F_SAMPLE;
	}
//////////////////////////////////////////

    //Timer Seting with HPS_PrivateTimer library
    // Initialise and configure HPS Private Timer Driver.
    exitOnFail(Timer_initialise(0xFFFEC600),TIMER_SUCCESS);//Exit if not successful
    // Set the "Load" value of the timer to max value.
    exitOnFail(Load_timer(0xFFFFFFFF),TIMER_SUCCESS); //Exit if not successful
    // "Prescaler"=0, Enable = 1, Set Automatic reload = 1, disable ISR = 1
    exitOnFail(Control_timer(0, 0, 1, 1), TIMER_SUCCESS); //Exit if not successful
    exitOnFail(Clear_interrupt(),TIMER_SUCCESS);                 //Exit if not successful
    ResetWDT(); //Reset watchdog

	//Initialise the LT24 LCD Display.
    exitOnFail(LT24_initialise(0xFF200060,0xFF200080),LT24_SUCCESS);//Exit if not successful
    HPS_ResetWatchdog();

    while (1) {
    	// current value for timer
    	unsigned int currentTimerValue = Get_time(currentTimerValue);
    	unsigned int currentTimerValue2;
        keys_pressed = getPressedKeys();

        // 7-Segment HEX0 for writing live value
    	DE1SoC_SevenSeg_SetDoubleDec(DISPLAY_0, life);
    	 // 7-Segment HEX2 for --
    	DE1SoC_SevenSeg_SetDoubleDec(DISPLAY_1, 100);
    	 // 7-Segment HEX2 for--
        DE1SoC_SevenSeg_SetDoubleDec(DISPLAY_2, 100);
        //Button Capture for Key 0 -> starting Key

    	if(!(*BUTTONPRESSED & 0x1)){
    		exitOnFail(LT24_copyFrameBuffer(start,0,0,240,320),LT24_SUCCESS); //Exit if not successful
    		printf("LT24 start page");
    	}
    	 //Button Capture for Key 0
    	while(!(*BUTTONPRESSED & 0x1)){
    		///////////////////////-> GenerateTone Lab3.1
    		unsigned int currentTimerValue2;
            if ((fifospace_ptr[2] > 0) && (fifospace_ptr[3] > 0)) {
            	currentTimerValue2 = Get_time(currentTimerValue2);
            	if ((previousTimerValue[2] - currentTimerValue2) >= (MSECONDS_THRESHOLD*100)) {
            		usleep(125000);
            		k++;
            		if(k>15){k=0;}
            		previousTimerValue[2] = previousTimerValue[2] - (MSECONDS_THRESHOLD*100);
            	}
                phase = phase + inc[k];
                while (phase >= PI2) {
                    phase = phase - PI2;
                }
                audio_sample = (signed int)( ampl * sin( phase ) );
                *audio_left_ptr = audio_sample;
                *audio_right_ptr = audio_sample;
            }
            HPS_ResetWatchdog();
    	}
    	////////////////////////////////

    	// Figures drawing part, Every figur and backround sending to one buffer
    	// After just this buffer sending to LT24
    	for(y=0;y<76800;y++){
    	sendingLT24Buf[y]=background[y]; // this is for using to const buffer
    	}
    	copytoBackgroundFrame(sendingLT24Buf,jam,jamposx, jamposy, 25,25 );
    	copytoBackgroundFrame(sendingLT24Buf,row1enemy[i[1]],rowenemyposx[0], rowenemyposy[0], 30,30 );
    	copytoBackgroundFrame(sendingLT24Buf,row2enemy[i[2]],rowenemyposx[1], rowenemyposy[1], 30,30 );
    	copytoBackgroundFrame(sendingLT24Buf,row3enemy[i[3]],rowenemyposx[2], rowenemyposy[2], 30,30 );
    	// LT24 sending buffer
    	exitOnFail(LT24_copyFrameBuffer(sendingLT24Buf,0,0,240,320),
    	   	    	LT24_SUCCESS); //Exit if not successful

    	// Checking for a overlaping enemy and main charecter on rows
    	overlaping1 = overlapingCheck(cuteposx, cuteposy, rowenemyposx[0], rowenemyposy[0], 30);
    	overlaping2 = overlapingCheck(cuteposx, cuteposy, rowenemyposx[1], rowenemyposy[1], 30);
    	overlaping3 = overlapingCheck(cuteposx, cuteposy, rowenemyposx[2], rowenemyposy[2], 30);
    	overlaping = ( overlaping1 || overlaping2 || overlaping3);
    	// If there is any over laping life mines one and the cracter start begening
    	life =life-overlaping;
    	//the game aim take the jam, this part for checking to jam and the caracter overlap
    	// If they ovelape the gamer win
    	winSituation = overlapingCheck(cuteposx, cuteposy, jamposx, jamposy, 25);
    	// If there is a overlaping
    	if(winSituation){
        	//Win buffer to LT24
        	exitOnFail(LT24_copyFrameBuffer(win,0,0,240,320),LT24_SUCCESS); //Exit if not successful
        	//Win buffer should be stay on LCD
        	cuteposx = 60;
        	cuteposy = 250;
        	usleep(1500000);
        } else if(overlaping){
        	//game over buffer to LT24
    	    exitOnFail(LT24_copyFrameBuffer(explosion,cuteposx,cuteposy,24,24),
    	    LT24_SUCCESS); //Exit if not successful
    	    //game over buffer should be stay on LCD
    		usleep(1500000);
    	} else {
    		//Display the cute image. Game shoul continue
        	copytoBackgroundFrame(sendingLT24Buf,cute,cuteposx, cuteposy, 25,25 );
          	exitOnFail(LT24_copyFrameBuffer(sendingLT24Buf,0,0,240,320),
            	   	    	LT24_SUCCESS); //Exit if not successful
    	}
    	if ((previousTimerValue[0] - currentTimerValue) >= (MSECONDS_THRESHOLD)) {
    		// Reset the counter values for enemy color
    		if (i[1]>9) {i[1] = 0;}
    		if (i[2]>9) {i[2] = 0;}
    		if (i[3]>9) {i[3] = 0;}

    		// Changeing part x positions of enemy objects with different speed
    		rowenemyposx[0] = rowenemyposx[0] + 8;  // row 1
    		rowenemyposx[1] = rowenemyposx[1] + 5;  // row 2
    		rowenemyposx[2] = rowenemyposx[2] + 2;  // row 3

    		// Previous Timer Value checking
    		previousTimerValue[0] = previousTimerValue[0] - (MSECONDS_THRESHOLD);
    	}
		if (rowenemyposx[0]>200) { // row 2
			// after distplay row finsh, it should start again
			rowenemyposx[0] = 0;
		    i[1]++;// enemy color changing
		}
		if (rowenemyposx[1]>200) { // row 3
			// after distplay row finsh, it should start again
			rowenemyposx[1] = 0;
		    i[2]++;// enemy color changing
		}
		if (rowenemyposx[2]>200) { // row 4
			// after distplay row finsh, it should start again
			rowenemyposx[2] = 0;
		    i[3]++;// enemy color changing
		}
        // Capture the button-Lab
    	if (*BUTTONPRESSED & 0x8) { // If KEY3 is pressed
    		// chacing the caracter acording to background maze shape
    		if(cuteposx<=10){
    			cuteposx = 20; // Let the cute stays in the road
    		} else {
    			cuteposx = cuteposx - 5; // Move red enemy to left
    		}
    		// Clear BUTTONPRESSED registers
    		*BUTTONPRESSED =  (1<<1) | (1<<2) | (1<<3);
    	}
    	if (*BUTTONPRESSED & 0x4) { // If KEY2 is pressed
    		// chacing the caracter acording to background maze shape
    		if(cuteposx>=210){
    			cuteposx = 200; // Let the cute stays in the road
    		} else {
    			cuteposx = cuteposx + 5; // Move red enemy to right
    		}
    		// Clear BUTTONPRESSED registers
    		*BUTTONPRESSED =  (1<<1) | (1<<2) | (1<<3);
    	}
      	if (*BUTTONPRESSED & 0x2){
      		if(cuteposy>250){
        		// chacing the caracter acording to background maze shape
        		cuteposy = 250; //  the cute stays in maze
        	} else if(cuteposy<=10) {
        		cuteposy = 20; //  the cute stays in maze
        	} else if((cuteposx<=150)&&(cuteposx>=120)&&(cuteposy==250)) { // If KEY1 is pressed) {
        		cuteposy = 180; // move the stairs just up
        	} else if((cuteposx<=116)&&(cuteposx>=86)&&(cuteposy==180)) { // If KEY1 is pressed) {
        		cuteposy = 110; // move the stairs just up
        	} else if((cuteposx<=150)&&(cuteposx>=120)&&(cuteposy==110)) { // If KEY1 is pressed) {
        		cuteposy = 30; // move the stairs just up
        	}
        	// Clear BUTTONPRESSED registers
        	*BUTTONPRESSED =  (1<<1) | (1<<2) | (1<<3);
        	}

      	if((cuteposx>=180)&&(cuteposy==190)){
      	    cuteposy = 250;//Falling from empty space
      	}else if((cuteposx>160)&&(cuteposy==110)){
       	    cuteposy = 180;//Falling from empty space
      	}else if((cuteposx>25)&&(cuteposx<55)&&(cuteposy==110)){
       	    cuteposy = 180;//Falling from empty space
      	}else if((cuteposx>=170)&&(cuteposy==30)){
       	    cuteposy = 110;//Falling from empty space
      	  }
    	if(overlaping){
    		// Reset initial position of the cute (main object of the game)
    		cuteposx = 60;
    		cuteposy = 250;

    		// Reset initial positions for enemys to starting point of them
    		rowenemyposx [0] = 0;
    		rowenemyposx [1] = 0;
    		rowenemyposx [2] = 0;

    		// Reset counter values for enemy colr
    		i[0] = 0;
    		i[1] = 0;
    		i[2] = 0;
    		i[3] = 0;
        	// previous timer values equal to currentTimerValue for each time unit
        	previousTimerValue[2] = currentTimerValue;
        	previousTimerValue[1] = currentTimerValue;
        	previousTimerValue[0] = currentTimerValue;
        	if(life<=0){
        	//Display game over image. Setting the co-ordinates and size of the image.
    		exitOnFail(LT24_copyFrameBuffer(gameover,0,0,240,320),
    		    	LT24_SUCCESS); //Exit if not successful
			// Clear BUTTONPRESSED registers
			*BUTTONPRESSED =  (1<<1) | (1<<2) | (1<<3);
        	}
        	HPS_ResetWatchdog(); //Just reset the watchdog.
    	}

    	while(life<=0){// There is 5 change, this part for checking
    		///////////////->Unite 3 lab 3.1
    		//Always check the FIFO Space before writing or reading left/right channel pointers
            if ((fifospace_ptr[2] > 0) && (fifospace_ptr[3] > 0)) {
                //If there is space in the write FIFO for both channels:
            	// Read the current time for audio output generation
            	currentTimerValue2 = Get_time(currentTimerValue2);

            	if ((previousTimerValue[2] - currentTimerValue2) >= (MSECONDS_THRESHOLD*100)) {
            		usleep(125000);
            		k++;
            		if(k>15){k=0;}
            		previousTimerValue[2] = previousTimerValue[2] - (MSECONDS_THRESHOLD*100);
            	}

                //Increment the phase with a different tune value each time
                phase = phase + inc[k];
                //Ensure phase is wrapped to range 0 to 2Pi (range of sin function)
                while (phase >= PI2) {
                    phase = phase - PI2;
                }
                // Calculate next sample of the output tone.
                audio_sample = (signed int)( ampl * sin( phase ) );
                // Output tone to left and right channels.
                *audio_left_ptr = audio_sample;
                *audio_right_ptr = audio_sample;
            }
      	HPS_ResetWatchdog(); //Just reset the watchdog.
    	}
    	HPS_ResetWatchdog(); //Just reset the watchdog.

    }
}
