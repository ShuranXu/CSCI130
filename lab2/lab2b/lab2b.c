#include <avr/io.h>
#include <util/delay.h>
#include <avr/cpufunc.h>

/*

Assignment:

Write a C program that uses the 8 LEDs as a binary display and 2 momentary switches as input.
The program should keep a count. When one of the two switches is pressed then released, the 
count should increase by 1 and if the other is pressed then released, the count should decrease by
1. The count should go from 0 to 255 and loop back to 0. The program should display the binary
representation of the count using the 8 LEDs.

Connection Diagram:

Atmega328p | Romeo board | IO Board | Jumper Component

	PD2    |     D2      |  JP3_1   | 	D1A  
	PD3    |     D3      |  JP3_2   | 	D1B  
	PD4    |     D4      |  JP3_3   | 	D1C
	PD5    |     D5      |  JP3_4   | 	D1D  
	PD6    |     D6      |  JP3_5   | 	D1E  
	PD7    |     D7      |  JP3_6   | 	D1F 
	PB0    |     D8      |  JP3_7   | 	D1G  
	PB1    |     D9      |  JP3_8   | 	D1H
	PB2    |     D10     |  JP2_5   | 	S1  
	PB3    |     D11     |  JP2_6   | 	S2

Idea:

Let us set S1 to be the counter incrementer, and S2 to be the counter decrementer. The step is 1.

*/

/*********************************************
	Macros
**********************************************/

#define MIN 0U
#define MAX 255U
#define F_CPU 16000000UL


enum{
	NONE=0,
	ADD=1,
	SUBTRACT=2
};

/*********************************************
	Local Function Declaration
**********************************************/

static void delay_us (uint16_t us);
static void convert_to_binary(uint8_t bits[], uint8_t number);
static void update_leds(uint8_t bits[]);
static uint8_t update_counter(uint8_t counter, int change);

/*********************************************
	Main Function
**********************************************/

int main(int argc, char const *argv[])
{
	//set D2-D7 as output pins
	DDRD |= ((1 << PD2)| (1 << PD3) | (1 << PD4 ) | ( 1 << PD5 ) | ( 1 << PD6 ) | ( 1 << PD7 )); 
	//set B0 and B1 as output pins
	DDRB |= ((1 << PB0) | (1 << PB1));

	//set B2 and B3 as input pins 
	DDRB &=~((1 << PB2) | (1 << PB3));
	//set up pull-up resistors
	PORTB |=((1 << PB2) | (1 << PB3));

	uint8_t bits[8];
	uint8_t count = MIN;
	volatile uint8_t input;
	int change = NONE;

	while(1){

		_NOP();
		input = PINB;

		if( ((input >> 3) & 0b00000001) == 0 ){ //press S2

			change = SUBTRACT;
		}
		else if( ((input >> 2) & 0b00000001) == 0 ){ //press S1

			change = ADD;
		}
		else{
			count = update_counter(count, change);
			change = NONE;
			convert_to_binary(bits,count);
			update_leds(bits);
		}
	}
}


/*********************************************
	Local Function Definition
**********************************************/

static uint8_t update_counter(uint8_t counter, int change){

	if( change == ADD ){

		return (counter + 1);
	}
	else if( change == SUBTRACT ){
		return (counter - 1);
	}
	else
		return counter;
}

static void update_leds(uint8_t bits[]){

	PORTD = 0b00000000;
	PORTB = 0b00001100; //need to ensure that pull-up resistors are always set.
	
	delay_us(10);
	PORTD |= ((bits[0] << PD2) | (bits[1] << PD3) | (bits[2] << PD4) | (bits[3] << PD5) | (bits[4] << PD6) | (bits[5] << PD7));
	PORTB |= ((bits[6] << PB0) | (bits[7] << PB1));
	
	return;
}

static void convert_to_binary(uint8_t bits[], uint8_t number){

	int c,k;

	for (c = 7; c >= 0; c--){
    	k = number >> c;
    	bits[c] = (k & 1) ? 1 : 0;
  	}
	return;
}

static void delay_us (uint16_t us) {
	uint16_t i;
	
	for (i = 0; i < us; i++)
		_delay_us(1);
}
