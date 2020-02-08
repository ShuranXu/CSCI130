#include <avr/io.h>
#include <util/delay.h>

/*

Assignment:

Write a C program that uses the 8 LEDs as a binary display. The program will display the binary
representation of the number 0 to 255 in sequence at 1 Hz interval. When the number reaches
255, it will loop back to 0. 

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

*/

/*********************************************
	Macros
**********************************************/

#define MIN 0
#define MAX 255
#define F_CPU 16000000UL

/*********************************************
	Local Function Declaration
**********************************************/

static void delay_ms (uint16_t ms);
static void convert_to_binary(uint8_t bits[], uint8_t number);
static void update_leds(uint8_t bits[]);

/*********************************************
	Main Function
**********************************************/
//better version

int main(int argc, char const *argv[])
{
	//set D2-D7 as output pins
	DDRD |= ((1 << PD2)| (1 << PD3) | (1 << PD4 ) | ( 1 << PD5 ) | ( 1 << PD6 ) | ( 1 << PD7 )); 
	//set B0 and B1 as output pins
	DDRB |= ((1 << PB0) | (1 << PB1));

	uint8_t i = MIN;
	uint8_t bits[8];

	while(1){

		if( i > MAX ){
			i = MIN;
		}

		i++;
		PORTD = i<<2;
		PORTB = i>>6;
		delay_ms(50);
	}

	return 0;
}


// int main(int argc, char const *argv[])
// {
// 	//set D2-D7 as output pins
// 	DDRD |= ((1 << PD2)| (1 << PD3) | (1 << PD4 ) | ( 1 << PD5 ) | ( 1 << PD6 ) | ( 1 << PD7 )); 
// 	//set B0 and B1 as output pins
// 	DDRB |= ((1 << PB0) | (1 << PB1));

// 	uint8_t i = MIN;
// 	uint8_t bits[8];

// 	while(1){

// 		if( i > MAX ){
// 			i = MIN;
// 		}

// 		convert_to_binary(bits,i);
// 		update_leds(bits);
// 		i++;
// 	}

// 	return 0;
// }

/*********************************************
	Local Function Definition
**********************************************/

static void update_leds(uint8_t bits[]){

	PORTD = 0b00000000;
	PORTB = 0b00000000;

	delay_ms(100);//500

	PORTD |= ((bits[0] << PD2) | (bits[1] << PD3) | (bits[2] << PD4) | (bits[3] << PD5) | (bits[4] << PD6) | (bits[5] << PD7));
	PORTB |= ((bits[6] << PB0) | (bits[7] << PB1));

	delay_ms(100); //500

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

static void delay_ms (uint16_t ms) {
	uint16_t i;
	for (i = 0; i < ms; i++)
		_delay_ms(1);	
}
