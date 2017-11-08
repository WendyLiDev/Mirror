/*
 * time.c
 *
 * Created: 10/31/2017 1:34:30 PM
 *  Author: student
 */ 

#ifndef F_CPU
#define F_CPU 8000000UL // 8 MHz clock speed
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include "lcd.h"

////////////////////////////////////////////////////////////////////////////////
//Functionality - Gets bit from a PINx
//Parameter: Takes in a uChar for a PINx and the pin number
//Returns: The value of the PINx
unsigned char GetBit(unsigned char port, unsigned char number)
{
	return ( port & (0x01 << number) );
}

//////////////////////////////////////////////////////////////////
//////////////////////TIMER CODE /////////////////////////////////
//////////////////////////////////////////////////////////////////
// TimerISR() sets this to 1. C programmer should clear to 0.
volatile unsigned char TimerFlag = 0;
// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks
void TimerOn() {
	// AVR timer/counter controller register TCCR1
	// bit3 = 0: CTC mode (clear timer on compare)
	// bit2bit1bit0=011: pre-scaler /64
	// 00001011: 0x0B
	// SO, 8 MHz clock or 8,000,000 /64 = 125,000 ticks/s
	// Thus, TCNT1 register will count at 125,000 ticks/s
	TCCR1B = 0x0B;
	// AVR output compare register OCR1A.
	// Timer interrupt will be generated when TCNT1==OCR1A
	// We want a 1 ms tick. 0.001 s * 125,000 ticks/s = 125
	// So when TCNT1 register equals 125,
	// 1 ms has passed. Thus, we compare to 125.
	OCR1A = 125;// AVR timer interrupt mask register
	// bit1: OCIE1A -- enables compare match interrupt
	TIMSK1 = 0x02;
	//Initialize avr counter
	TCNT1=0;
	// TimerISR will be called every _avr_timer_cntcurr milliseconds
	_avr_timer_cntcurr = _avr_timer_M;
	//Enable global interrupts: 0x80: 1000000
	SREG |= 0x80;
}
void TimerOff() {
	// bit3bit1bit0=000: timer off
	TCCR1B = 0x00;
}
void TimerISR() {
	TimerFlag = 1;
}
// In our approach, the C programmer does not touch this ISR, but rather TimerISR()
ISR(TIMER1_COMPA_vect) {
	// CPU automatically calls when TCNT1 == OCR1
	// (every 1 ms per TimerOn settings)
	// Count down to 0 rather than up to TOP (results in a more efficient comparison)
	_avr_timer_cntcurr--;
	if (_avr_timer_cntcurr == 0) {
		// Call the ISR that the user uses
		TimerISR();
		_avr_timer_cntcurr = _avr_timer_M;
	}
}
// Set TimerISR() to tick every M ms
void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}
/*Global variables*/
unsigned char seconds;
unsigned char minutes;
unsigned char hours;
/*Handles ticking the clock once every second and increments minutes after 60 seconds*/
enum tick_States{tick_init, tick_tick, tick_reset}tick_State;
void tick(){ 
	switch(tick_State){
		case tick_init:
			tick_State = tick_tick;
			break;
		case tick_tick:
			if(seconds < 59){
				tick_State = tick_tick;
			}
			else if(seconds >= 59){
				tick_State = tick_reset;
			}
			break;
		case tick_reset:
			tick_State = tick_tick;
			break;
		default:
			break;
	}
	switch(tick_State){
		case tick_init:
			seconds = 0;
			break;
		case tick_tick:
			seconds++;
			break;
		case tick_reset:
			minutes++;
			seconds = 0;
			break;
		default:
			break;
	}
}
/*Calculates the time using the tickFSM and uses B1 to increment minutes , b2 to increment hours*/
enum time_States{time_init, time_wait, time_minute, time_hour}time_State;
char* timeStr = "  :             Good Night!";
void calculateTime(){
	switch(time_State){
		case time_init:
			time_State = time_wait;
			break;
		case time_wait:
			if(GetBit(PINB, 1)){
				time_State = time_minute;
			}
			else if(GetBit(PINB, 2)){
				time_State = time_hour;
			}
			else if( (!GetBit(PINB, 1) || !GetBit(PINB, 2)) || (GetBit(PINB, 1) && GetBit(PINB, 2))){
				time_State = time_wait;
			}
			break;
		case time_minute:
			time_State = time_wait;
			break;
		case time_hour:
			time_State = time_wait;
			break;
		default:
			time_State = time_init;
			break;
	}
	switch(time_State){
		case time_init:
			minutes = 0;
			hours = 0;
			break;
		case time_wait:
			//updates time based on tick function
			if((minutes == 59) && (seconds ==59)){ 
				hours ++;
				minutes ++;
			}
			else if((hours == 23) && (minutes == 59) && (seconds == 59)){
				hours = 0;
				minutes = 0;
			}
			break;
		case time_minute: //handles the case when user inputs time change
			if(minutes == 59){
				minutes = 0;
				hours ++;
			}
			else{
				minutes ++;
			}
			break;
		case time_hour: //handles the case when user inputs time change
			if(hours == 23){
				hours = 0;
			}
			else{
				hours++;
			}
			break;
		default:
		break;
	}
	};
/*Displays the time based on variables "hours" and "minutes" as "hours":"minutes"*/
enum disp_States{display_init, update}display_State;
void displayTime(){
	switch(display_State){
		case display_init:
		break;
		case update:
			//updates bit 0 of the time
			if(minutes > 58){ //only check when it is reaching the top of the hour
				if((hours / 10) == 0){timeStr[0] = '0';}
				else if((hours / 10) == 1){timeStr[0] = '1';}
				else if((hours / 10) == 2){timeStr[0] = '2';}
				else if((hours / 10) == 3){timeStr[0] = '3';}
				else if((hours / 10) == 4){timeStr[0] = '4';}
				else if((hours / 10) == 5){timeStr[0] = '5';}
				else if((hours / 10) == 6){timeStr[0] = '6';}
				else if((hours / 10) == 7){timeStr[0] = '7';}
				else if((hours / 10) == 8){timeStr[0] = '8';}
				else if((hours / 10) == 9){timeStr[0] = '9';}
				//updates bit 1 of the time
				if((hours % 10) == 0){timeStr[1] = '0';}
				else if((hours % 10) == 1){timeStr[1] = '1';}
				else if((hours % 10) == 2){timeStr[1] = '2';}
				else if((hours % 10) == 3){timeStr[1] = '3';}
				else if((hours % 10) == 4){timeStr[1] = '4';}
				else if((hours % 10) == 5){timeStr[1] = '5';}
				else if((hours % 10) == 6){timeStr[1] = '6';}
				else if((hours % 10) == 7){timeStr[1] = '7';}
				else if((hours % 10) == 8){timeStr[1] = '8';}
				else if((hours % 10) == 9){timeStr[1] = '9';}
			}
				//updates bit 3 of the time
			if(seconds > 58){ //only check when it is reaching the top of the minute
				if((minutes / 10) == 0){timeStr[3] = '0';}
				else if((minutes / 10) == 1){timeStr[3] = '1';}
				else if((minutes / 10) == 2){timeStr[3] = '2';}
				else if((minutes / 10) == 3){timeStr[3] = '3';}
				else if((minutes / 10) == 4){timeStr[3] = '4';}
				else if((minutes / 10) == 5){timeStr[3] = '5';}
				else if((minutes / 10) == 6){timeStr[3] = '6';}
				else if((minutes / 10) == 7){timeStr[3] = '7';}
				else if((minutes / 10) == 8){timeStr[3] = '8';}
				else if((minutes / 10) == 9){timeStr[3] = '9';}
				//updates bit 4 of the time
				if((minutes % 10) == 0){timeStr[4] = '0';}
				else if((minutes % 10) == 1){timeStr[4] = '1';}
				else if((minutes % 10) == 2){timeStr[4] = '2';}
				else if((minutes % 10) == 3){timeStr[4] = '3';}
				else if((minutes % 10) == 4){timeStr[4] = '4';}
				else if((minutes % 10) == 5){timeStr[4] = '5';}
				else if((minutes % 10) == 6){timeStr[4] = '6';}
				else if((minutes % 10) == 7){timeStr[4] = '7';}
				else if((minutes % 10) == 8){timeStr[4] = '8';}
				else if((minutes % 10) == 9){timeStr[4] = '9';}
			}
			//updates the greeting
			if(minutes > 58){ //only check when it is reaching the top of the hour
				if(hours == 6){
					//Good morning!
					timeStr[21] = 'm';
					timeStr[22] = 'o';
					timeStr[23] = 'r';
					timeStr[24] = 'n';
					timeStr[25] = 'i';
					timeStr[26] = 'n';
					timeStr[27] = 'g';
					timeStr[28] = '!';
					timeStr[29] = ' ';
					timeStr[30] = ' ';
					timeStr[31] = ' ';
					timeStr[32] = ' ';
				}
				else if(hours == 12){
					//Good afternoon! 
					timeStr[21] = 'a';
					timeStr[22] = 'f';
					timeStr[23] = 't';
					timeStr[24] = 'e';
					timeStr[25] = 'r';
					timeStr[26] = 'n';
					timeStr[27] = 'o';
					timeStr[28] = 'o';
					timeStr[29] = 'n';
					timeStr[30] = '!';
					timeStr[31] = ' ';
					timeStr[32] = ' ';
				}
				else if(hours == 18){
					//Good evening!
					timeStr[21] = 'e';
					timeStr[22] = 'v';
					timeStr[23] = 'e';
					timeStr[24] = 'n';
					timeStr[25] = 'i';
					timeStr[26] = 'n';
					timeStr[27] = 'g';
					timeStr[28] = '!';
					timeStr[29] = ' ';
					timeStr[30] = ' ';
					timeStr[31] = ' ';
					timeStr[32] = ' ';
				}
				else if(hours == 22){
					//Good Night!
					timeStr[21] = 'n';
					timeStr[22] = 'i';
					timeStr[23] = 'g';
					timeStr[24] = 'h';
					timeStr[25] = 't';
					timeStr[26] = '!';
					timeStr[27] = ' ';
					timeStr[28] = ' ';
					timeStr[29] = ' ';
					timeStr[30] = ' ';
					timeStr[31] = ' ';
					timeStr[32] = ' ';
				}
			}
			LCD_DisplayString(1, timeStr);
			break;
		default:
			break;
	}
	switch(display_State){
		case display_init:
			display_State = update;
			break;
		case update:
			display_State = update;
			break;
		default:
			break;
	}
}
int main(void)
{
    TimerSet(1000);
    TimerOn();
    DDRB = 0x00; PORTB = 0xFF;
	DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0xFF; PORTD = 0x00;
	tick_State = tick_init;
	time_State = time_init;
	display_State = display_init;
	LCD_init();
	//LCD_DisplayString( 1, "Hello World!");
	
    while (1)
    {
		tick();
		calculateTime();
		displayTime();
	    while (!TimerFlag);  // Wait for timer period
	    TimerFlag = 0;


    }
}