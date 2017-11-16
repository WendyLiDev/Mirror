#include <avr/io.h>
#include <avr/interrupt.h>

volatile unsigned char TimerFlag = 0; //TimerISR() sets this to 1, we need to clear to 0

unsigned long _avr_timer_M = 1; //start count from here to 0, default 1 ms
unsigned long _avr_timer_cntcurr = 0; // current internal count of 1 ms ticks

unsigned char brightness; //should store a value from 0 - 20, 0 for off, 20 for brightest

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

enum PWM_States{pwm_init, pwm}PWM_State;
	
short time_On;
short time_Off;
short cnt;
void PWM_set(){
	//brightness is a number between 0 and 19
	//turn on for brightness and off for 20-brightness
	
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
			time_On = brightness;
			time_Off = (20-brightness);
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

unsigned char tmpB;
unsigned char tmpD;
unsigned short current_val; // stores current value read
unsigned short my_short; //stores the average light
/* cycleInputs takes values from PA0 to PA7 one at a time.  
converts the analog value from the photoresistor and converts it to a digital value stored in my_short */
enum cycle_States{cycle_init, cycle_zero, cycle_one, cycle_two, cycle_three, cycle_four, cycle_five, cycle_six, cycle_seven}cycle_State;
void cycleInputs(){
	switch(cycle_State){ //transitions
		case cycle_init:
			cycle_State = cycle_zero;
			break;
		case cycle_zero:
			cycle_State = cycle_one;
			break;
		case cycle_one:
			cycle_State = cycle_two;
			break;
		case cycle_two:
			cycle_State = cycle_three;
			break;
		case cycle_three:
			cycle_State = cycle_four;
			break;
		case cycle_four:
			cycle_State = cycle_five;
			break;
		case cycle_five:
			cycle_State = cycle_six;
			break;
		case cycle_six:
			cycle_State = cycle_seven;
			break;
		case cycle_seven:
			cycle_State = cycle_zero;
			break;
		default:
			break;
	}	
	switch(cycle_State){ //actions
		case cycle_init:
			Set_A2D_Pin(0x00);
			//tmpB = tmpD = 0;
			break;
		case cycle_zero:
			Set_A2D_Pin(0x00);
			current_val = ADC;
			my_short = 0;
			my_short = my_short + (current_val / 7);
			break;
		case cycle_one:
			Set_A2D_Pin(0x01);
			current_val = ADC;
			my_short = my_short + (current_val / 7);
			break;
		case cycle_two:
			Set_A2D_Pin(0x02);
			current_val = ADC;
			my_short = my_short + (current_val / 7);
			break;
		case cycle_three:
			Set_A2D_Pin(0x03);
			current_val = ADC;
			my_short = my_short + (current_val / 7);
			break;
		case cycle_four:
			Set_A2D_Pin(0x04);
			current_val = ADC;
			my_short = my_short + (current_val / 7);
			break;
		case cycle_five:
			Set_A2D_Pin(0x05);
			current_val = ADC;
			my_short = my_short + (current_val / 7);
			break;
		case cycle_six:
			Set_A2D_Pin(0x06);
			current_val = ADC;
			my_short = my_short + (current_val / 7);
			break;
		case cycle_seven:
			Set_A2D_Pin(0x07);
			current_val = ADC;
			my_short = my_short + (current_val / 7);
			if(my_short == 0){
				brightness = 0;
			}
			else {
				//update the value of brightness once every eight cycles through all the photoresistors
				brightness = ((my_short + 50) / 51) ; //I subtracted two because of some error in my photoresistors, your parts may vary
			}
			time_On = brightness;
			time_Off = (20-brightness);
			// tmpB = (char)my_short; //display on portb
			// tmpD = (char)(my_short >> 8); //display two bits on portd
			// tmpB = (char)brightness; //display the value of brightness on portb
			break;
		default:
			break;
	}
	PORTB = tmpB;
	PORTD = tmpD;
}

int main(void)
{
	DDRB = 0xFF; PORTB = 0x00;
	DDRD = 0xFF; PORTD = 0x00;
	DDRA = 0x00; PORTA = 0xFF;
	
	unsigned long pwm_elapsed_time = 2;
	unsigned long light_input_elapsed_time = 50;
	const unsigned long timerPeriod = 2;
	TimerSet(timerPeriod);
	TimerOn();
	ADC_init();
	cycle_State = cycle_init;
	PWM_State = pwm_init;
	//tmpB = tmpD = 0;
	
	while(1)
	{
		if(pwm_elapsed_time>=500){
			cycleInputs();
			pwm_elapsed_time = 0;
		}
		if(light_input_elapsed_time >= 2){
			PWM_set();
			light_input_elapsed_time = 0;
		}
		while(!TimerFlag);
		TimerFlag = 0;
		pwm_elapsed_time += timerPeriod;
		light_input_elapsed_time += timerPeriod;
	}
	return 0;
}