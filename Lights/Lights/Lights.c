#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>

volatile unsigned char TimerFlag = 0; //TimerISR() sets this to 1, we need to clear to 0

unsigned long _avr_timer_M = 1; //start count from here to 0, default 1 ms
unsigned long _avr_timer_cntcurr = 0; // current internal count of 1 ms ticks

unsigned short brightness; //should store a value from 0 - 20, 0 for off, 20 for brightest
signed char manualOffset; // should be a value between -9 and 9
unsigned short time_On; //Used in the PWM to dim lights
unsigned short time_Off; //Used in the PWM to dim lights

unsigned char GetBit(unsigned char x, unsigned char k){
	return ((x & (0x01 << k)) != 0);
}
unsigned char SetBit(unsigned char x, unsigned char k, unsigned char b){
	return (b ? x | (0x01 << k) : x & ~(0x01 << k));
}
void TimerOn(){
    //Initialize and start the timer
	//AVR timer/counter controller register TCCR1
	/*
     bit3 = 0: CTC mode(clear timer on compare)
     bit2-bit1-bit0 = 011: pre-scaler/64
     00001011: 0x0B
     So 8 MHz clock or 8000000 / 64 = 125000 ticks/s
     So TCNT1 register will count at 125000 ticks/s
     */
	TCCR1B = 0x0B;
    
	//AVR output compare register OCR1A
	/*
     Timer interrupt will be generated when TCNT1 == OCR1A
     Want a 1 ms tick; .001 s * 125000 ticks/s = 125
     So when TCNT1 register equals 125, 1 ms has passed
     So we compare to 125
     */
	OCR1A = 125;
    
	//AVR timer interrupt mask register
	TIMSK1 = 0x02; //bit1: OCIE1A: enable compare match interrupt
    
	//init avr counter
	TCNT1 = 0;
    
	_avr_timer_cntcurr = _avr_timer_M;
	//TimerISR called every _avr_timer_cntcurr milliseconds
    
	//enable global interrupts
	SREG |= 0x80;
}
void TimerOff(){
    //Stop the timer
	TCCR1B = 0x00; //timer off bc 0
}
void TimerISR(){
    //Auto-call when the timer ticks, with the contents filled by the user ONLY with an instruction that sets TimerFlag = 1
	TimerFlag = 1;
}
ISR(TIMER1_COMPA_vect){
    //Interrupt
	_avr_timer_cntcurr--; //count down to 0
	if(_avr_timer_cntcurr == 0)
	{
		TimerISR(); //call ISR that user uses
		_avr_timer_cntcurr = _avr_timer_M;
	}
}
void TimerSet(unsigned long M){
    //Set the timer to tick every M ms
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}
void ADC_init(){
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
	// ADEN: Enables analog-to-digital conversion
	// ADSC: Starts analog-to-digital conversion
	// ADATE: Enables auto-triggering, allowing for constant
	//	    analog to digital conversions.
}
void Set_A2D_Pin(unsigned char pinNum){
	ADMUX = (pinNum <= 0x07) ? pinNum : ADMUX;
	//Allow channel to stabilize
	static unsigned char i = 0;
	for(i = 0; i <15; i++) asm("nop");
	// Pins on PORTA are used as input for A2D conversion
	// The default channel is 0 (PA0)
	// The value of pinNum determines the pin on PORTA
	// used for A2D conversion
	// Valid values range between 0 and 7, where the value
	// represents the desired pin for A2D conversion
}

enum PWM_States{pwm_init, pwm}PWM_State;
unsigned short cnt;
void PWM_set(){
	//brightness is a number between 0 and 20
	//turn on for brightness (ms) and off for 20-brightness (ms)
	switch(PWM_State){
		case pwm_init:
			PWM_State = pwm;
			break;
		case pwm:
			PWM_State = pwm;
			break;
		default:
			break;
	}
	switch(PWM_State){
		case pwm_init:
			cnt = 0;
			break;
		case pwm:
			if(cnt < time_On){
				PORTB = 0xFF;
				cnt++;
			}
			else if( (cnt >= time_On) && (cnt < time_Off)){
				PORTB = 0x00;
				cnt++;
			}
			else if(cnt == time_Off){
				PORTB = 0x00;
				cnt=0;
			}
			break;
		default:
			break;
	}
}

enum button_States{button_init, button_wait, button_up, button_up_wait_rel, button_down, button_down_wait_rel}button_State;
void adjustLight(){
	switch(button_State){
		case button_init:
			button_State = button_wait;
			break;
		case button_wait:
			if( (!GetBit(~PIND, 0) && !GetBit(~PIND, 1)) || (GetBit(~PIND, 0) && GetBit(~PIND, 1)) ){
				button_State = button_wait;
			}
			else if(GetBit(~PIND, 0) && !GetBit(~PIND, 1)){
				button_State = button_up;
			}
			else if(GetBit(~PIND, 1) && !GetBit(~PIND, 0)){
				button_State = button_down;
			}
			break;
		case button_up:
			if(GetBit(~PIND, 0)){
				button_State = button_up_wait_rel;
			}
			else if(!GetBit(~PIND, 0)){
				button_State = button_wait;
			}
			break;
		case button_up_wait_rel:
			if(GetBit(~PIND, 0)){
				button_State = button_up_wait_rel;
			}
			else if(!GetBit(~PIND, 0)){
				button_State = button_wait;
			}
			break;
		case button_down:
			if(GetBit(~PIND, 1)){
				button_State = button_down_wait_rel;
			}
			else if(!GetBit(~PIND, 1)){
				button_State = button_wait;
			}
			break;
		case button_down_wait_rel:
			if(GetBit(~PIND, 1)){
				button_State = button_down_wait_rel;
			}
			else if(!GetBit(~PIND, 1)){
				button_State = button_wait;
			}
			break;
		default:
			break;
	}
	switch(button_State){
		case button_init:
			manualOffset = 0;
			break;
		case button_wait:
			break;
		case button_up:
			if(manualOffset < 9){
				manualOffset = manualOffset + 1;
			}
			break;
		case button_up_wait_rel:
			break;
		case button_down:
			if(manualOffset > -9){
				manualOffset = manualOffset - 1;
			}
			break;
		case button_down_wait_rel:
			break;
		default:
			break;
	}
}
	
unsigned short current_val; // stores current value read
unsigned short my_short; //stores the average light
unsigned short temp;
/* cycleInputs takes values from PA0 to PA7 one at a time.  
converts the analog value from the photoresistor and converts it to a digital value stored in my_short */
enum cycle_States{cycle_init, cycle}cycle_State;
unsigned char currentPin;
void cycleInputs(){
	switch(cycle_State){ //transitions
		case cycle_init:
			cycle_State = cycle;
			break;
		case cycle:
			cycle_State = cycle;
			break;
		default:
			break;
	}	
	switch(cycle_State){ //actions
		case cycle_init:
			brightness = 1;
			time_On = brightness;
			time_Off = (21-brightness);
			currentPin = 0x00;
			Set_A2D_Pin(currentPin);
			my_short = 0;
			break;
		case cycle:
			Set_A2D_Pin(currentPin);
			current_val = ADC;
			my_short = my_short  + (current_val / 8);
			
			//only executed when on the last photoresistor
			if(currentPin == 0x07){
				if(my_short <= 0){
					if(manualOffset > 0){ 
						brightness = manualOffset; // manualOffset is a value between -9 to 9
												// it will increase brightness if above 10 and decrease if below 10
					}
					else{
						brightness = 0;
					}
				}
				else {
					//update the value of brightness once every eight cycles through all the photoresistors
					if(my_short > 1020){
						my_short = 1020;
					}
					//brightness = ((my_short + 50) / 51); //brightness is a number between 0 and 20
					brightness = 5;
					if(manualOffset < 0){ // if manualOffset is negative
						temp = abs(manualOffset);
						if((brightness - temp) <= 0){
							brightness = 0;
						}
						else{
							brightness = brightness - temp;
						}
					}
					else{ // manualOffset is positive
						brightness = brightness + manualOffset;
					}
				}
				
				if(brightness < 0){
					brightness = 0;
				}
				else if(brightness > 20){
					brightness = 20;
				}
				
				my_short = 0;
				time_On = brightness;
				time_Off = (21-brightness);
				currentPin = 0x00;
			}
			else{
				currentPin++;
			}
			break;
		default:
			break;
	}
}

int main(void)
{
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0x00; PORTD = 0xFF;
	DDRA = 0x00; PORTA = 0xFF;
	
	unsigned long pwm_elapsed_time = 1;
	TimerSet(1);
	TimerOn();
	ADC_init();
	cycle_State = cycle_init;
	PWM_State = pwm_init;
	button_State = button_init;
	while(1)
	{
		if(pwm_elapsed_time >= 20){
			adjustLight();
			cycleInputs();
			pwm_elapsed_time = 0;
			PORTC = brightness;
		}
		PWM_set();
		while(!TimerFlag);
		TimerFlag = 0;
		pwm_elapsed_time += 1;
	}
	return 0;
}
