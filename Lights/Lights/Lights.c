#include <avr/io.h>


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

/* takeInput converts the analog value from the photoresistor and converts it to a digital value stored in my_short */
enum adc_States{adc_Start, adc_Init, adc_Wait} adc_State;
unsigned char tmpB;
unsigned char tmpD;
unsigned short current_val; // stores current value read
unsigned short my_short; //stores the average light
unsigned char brightness; //should store a value from 0 - 20, 0 for off, 20 for brightest
void takeInput(){
	switch(adc_State){
		case adc_Start:
			adc_State = adc_Init;
			break;
		case adc_Init:
			adc_State = adc_Wait;
			break;
		case adc_Wait:
			break;	
		default:
			adc_State = adc_Start;
			break;
	}
	switch(adc_State){
		case adc_Init:
			tmpB = tmpD = 0;
			break;
		case adc_Wait:
			current_val = ADC;
			my_short = ( my_short + current_val ) / 2;
			if(my_short == 0){
				brightness = 0;
			}
			else {
				brightness = (my_short + 50) / 51;
			}
			tmpB = (char)my_short; //display on portb
			tmpD = (char)(my_short >> 8); //display two bits on portd
			break;
		default:
			break;
	}
	PORTB = tmpB;
	PORTD = tmpD;
}

/* cycleInputs takes values from PA0 to PA7 one at a time.  */
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
			break;
		case cycle_zero:
			Set_A2D_Pin(0x00);
			break;
		case cycle_one:
			Set_A2D_Pin(0x01);
			break;
		case cycle_two:
			Set_A2D_Pin(0x02);
			break;
		case cycle_three:
			Set_A2D_Pin(0x03);
			break;
		case cycle_four:
			Set_A2D_Pin(0x04);
			break;
		case cycle_five:
			Set_A2D_Pin(0x05);
			break;
		case cycle_six:
			Set_A2D_Pin(0x06);
			break;
		case cycle_seven:
			Set_A2D_Pin(0x07);
			break;
		default:
			break;
	}
}

int main(void)
{
	DDRB = 0xFF; PORTB = 0x00;
	DDRD = 0xFF; PORTD = 0x00;
	DDRA = 0x00; PORTA = 0xFF;
	
	adc_State = adc_Start;
	cycle_State = cycle_init;
	tmpB = tmpD = 0;
	ADC_init();
	
	while(1)
	{
		takeInput();
		cycleInputs();
	}
	return 0;
}