#include <avr/io.h>
#include <util/delay.h>
#include <avr/cpufunc.h>

/*

Assignment:

Write a C program that uses the 8 LEDs as a number display and 3 DIP switches as input. The
program should read in the state of the 3 DIP switches as a binary number where SW1_1 is the
most significant bit and SW1_3 is the least significant bit.

For example if SW1_1 = ON, SW1_2 = ON, SW1_3 = OFF then the number
interpreted by the program should be 0b00000110 = 6

In this program the 8 LEDs will not be display the binary representation of the number read from
the DIP switches. Instead the number read in from the DIP switches should be represented by the
number of LEDs that is lit up. For example if the DIP switches is set to 0b000000011, 3 LEDs
should be lit up.

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
	PB4    |     D12     |  JP2_7   | 	S3

*/

/*********************************************
	Macros
**********************************************/

#define MIN 0U
#define MAX 255U
#define F_CPU 16000000UL


/*********************************************
	Local Function Declaration
**********************************************/

static void delay_us (uint16_t us);
static void update_leds(uint8_t bits[]);
static void update_bit_array(uint8_t bits[], int lim, uint8_t val);
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
	DDRB &=~((1 << PB2) | (1 << PB3) | (1 << PB4));
	//set up pull-up resistors
	PORTB |=((1 << PB2) | (1 << PB3) | (1 << PB4));

	uint8_t bits[] = {0,0,0,0,0,0,0,0};
	volatile uint8_t input;
	int sum = 0;

	while(1){

		_NOP();
		input = PINB;

		sum += ((input >> 4) & 0b00000001) ? 0 : 1; //press S1
		sum += ((input >> 3) & 0b00000001) ? 0 : 2; //press S2
		sum += ((input >> 2) & 0b00000001) ? 0 : 4; //press S3

		update_bit_array(bits, sum, 1);
		update_leds(bits);
		update_bit_array(bits, sum, 0);
		
		sum = 0;
	}
}


/*********************************************
	Local Function Definition
**********************************************/

static void update_bit_array(uint8_t bits[], int lim, uint8_t val){

	int i;
	for(i=0;i<lim;i++){
		bits[i] = val;
	}
}

static void update_leds(uint8_t bits[]){

	PORTD = 0b00000000;
	PORTB = 0b00011100; //need to ensure that pull-up resistors are always set.
	
	delay_us(10);
	PORTD |= ((bits[0] << PD2) | (bits[1] << PD3) | (bits[2] << PD4) | (bits[3] << PD5) | (bits[4] << PD6) | (bits[5] << PD7));
	PORTB |= ((bits[6] << PB0) | (bits[7] << PB1));
	
	return;
}

static void delay_us (uint16_t us) {
	uint16_t i;
	
	for (i = 0; i < us; i++)
		_delay_us(1);
}
