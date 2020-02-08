#include <avr/io.h>
#include <util/delay.h>
#include <avr/cpufunc.h>
#include <stdio.h>

/*

Assignment:

i. Connect 2 of the 4 momentary switches (S1-S4) on the IO Trainer Board to 2 pins on the Rome
Board

ii. Write a C program that replicate the functions of Lab 2B but use the serial terminal as a display.
Like Lab 2B, the program will read two momentary switches and used them to increase or
decrease a count by 1. The count can go from -9999 to 9999. The decimal representation of the
count will be output on the serial command. The program will only display a new number when
there is an increment or decrement in the count.

The program must have a function defined as:
void printDec(int num)
The function takes in one integer as a parameter. The function would output the ASCII
representation of the number through the UART. Both negative and positive number has to be
presented properly.

iii. Create a connection chart in the comments above the program like in Lab 2B.


Connection Diagram:

Atmega328p | Romeo board | IO Board | Jumper Component

	PD2    |     D2     |  JP2_5   | 	S1  
	PD3    |     D3     |  JP2_6   | 	S2
	
*/

/*********************************************
	Macros & Enums
**********************************************/

#define F_CPU 16000000UL
#define BAUD 9600

enum{
	NONE=0,
	ADD=1,
	SUBTRACT=2
};

/*********************************************
	Local Function Declaration
**********************************************/

static void initUART(uint32_t baud);
static void transmitByte(uint8_t data);
static void printCR(void);
static void printDec(int num);
static void transmitString(char* stringPtr);
static int updateCounter(int counter, int change);

/*********************************************
	Main Function
**********************************************/

int main(int argc, char const *argv[]){

	//set D2 and D3 as input pins 
	DDRD &=~((1 << PD2) | (1 << PD3));
	//set up pull-up resistors
	PORTD |=((1 << PD2) | (1 << PD3));

	initUART(BAUD);
	
	int counter = 0;
	volatile uint8_t input;
	int change = NONE;

	while(1){

		//insert NOP for synchronization
		_NOP();

		input = PIND;

		if((input & (1 << 3)) == 0 ){ //press S2
			change = SUBTRACT;
		}
		else if((input & (1 << 2)) == 0 ){ //press S1
			change = ADD;
		}
		else
			continue;

		counter = updateCounter(counter, change);
		change = NONE;
		printDec(counter);
		printCR(); 
	}
	return 0;
}

/*********************************************
	Local Function Definition
**********************************************/

/*
  Initialize settings for uart functions, the function runs the USART
in double speed mode.
*/ 

static void initUART(uint32_t baud){

   //double-speed mode UBRR formula
   unsigned int ubrr = F_CPU/8/baud -1;
   //shift MSB and store in UBRR0H 
   UBRR0H = (unsigned char) (ubrr >> 8); 
   //store LSB in UBRR0L      
   UBRR0L = (unsigned char) ubrr;
   //enable double speed mode
   UCSR0A = (1 << U2X0);
   //enable transmitter/receiver 
   UCSR0B = (1 << RXEN0) | (1 << TXEN0);
   //8-Bit Characters, 1 Stop bits, Even parity
   UCSR0C = (1 << UCSZ00) | (1 << UCSZ01) | (1 << UPM01);
   
}

/* 
	print carriage return & newline
*/

static void printCR(void){

	transmitByte((uint8_t)'\n');
	transmitByte((uint8_t)'\r');
}

/*
   Write byte to UART
*/ 

static void transmitByte(uint8_t data){

	// Wait for empty transmit buffer
   	while ( !(UCSR0A & (1 << UDRE0)) );
    // Start transmission by writing to UDR0 register
   	UDR0 = data;
}  

/*
	Print a signed int number to the serial console. The negtive sign is printed if the number is negative.
	In addition, necessary character shifting is also performed for pretty formatting. 
*/

static void printDec(int num){

	char str[32];
	sprintf(str,"%d",num);
	transmitString(str);
}


/*
	print a string in the serial console
*/

static void transmitString(char* stringPtr){

	char* iter = stringPtr;
	while(*iter != '\0'){
		transmitByte(*iter++);
	}
}

/*
	Update counter value based on the button pressed.
*/
static int updateCounter(int counter, int change){

	if( change == ADD ){

		return (counter + 1);
	}
	else if( change == SUBTRACT ){
		return (counter - 1);
	}
	else
		return counter;
}
