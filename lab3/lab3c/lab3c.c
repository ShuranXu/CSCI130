#include <avr/io.h>
#include <util/delay.h>
#include <avr/cpufunc.h>
#include <stdio.h>

/*

Assignment:

i. Connect the 4 DIP switches (SW1_1-SW1_4) on the IO Trainer Board to 4 pins on the Romeo
Board

v. Write a C program that read in the state of the 4 DIP switches as a binary number where SW1_1
is the most significant bit and SW1_4 is the least significant bit. The program will display both a
binary and decimal representation of the state of the DIP switches. For example:
SW1_1 = on; SW1_2 = on; SW1_3 = off; SW1_4 = on;
Serial Output:
00001101 13

The program will only display a new number when there is the state of the DIP switches have
been changed.

ii. Include the printDec function from 3B. Add a new function defined as:
void printByte(uint8_t num)

The function takes in 8-bit unsigned integer as a parameter. The function would output the ASCII
representation of the 8-bit number in binary through the UART. The output should have leading
zeros.


Connection Diagram:

Atmega328p | Romeo board | IO Board | Jumper Component

	PD2    |     D2     |  JP2_5   | 	S1  
	PD3    |     D3     |  JP2_6   | 	S2
	PD4    |     D4     |  JP2_7   | 	S3
	PD5    |     D5     |  JP2_8   | 	S4
	
*/

/*********************************************
	Macros & Enums
**********************************************/

#define F_CPU 16000000UL
#define BAUD 9600

enum{

	S1 = 0,
	S2,
	S3,
	S4
};

/*********************************************
	Global variables
**********************************************/

int input_bit_shifts[] = {2,3,4,5};
int number_bit_shifts[] = {3,2,1,0};

/*********************************************
	Local Function Declaration
**********************************************/

static void initUART(uint32_t baud);
static void transmitByte(uint8_t data);
static void printCR(void);
static void transmitString(char* stringPtr);
static void printByte(uint8_t num);
static void printDec(int num);
static void updateNumberFromInput(uint8_t input,int switch_number, uint8_t* number);

/*********************************************
	Main Function
**********************************************/

int main(int argc, char const *argv[]){

	//set D2-D5 as input pins 
	DDRD &=~((1 << PD2) | (1 << PD3) | (1 << PD4) | (1 << PD5));
	//set up pull-up resistors
	PORTD |=((1 << PD2) | (1 << PD3) | (1 << PD4) | (1 << PD5));

	initUART(BAUD);
	
	volatile uint8_t input;
	uint8_t number = 0;
	uint8_t old_number = number;

	while(1){

		//insert NOP for synchronization
		_NOP();

		input = PIND;

		updateNumberFromInput(input,S1, &number);
		updateNumberFromInput(input,S2, &number);
		updateNumberFromInput(input,S3, &number);
		updateNumberFromInput(input,S4, &number);

		if( number != old_number ){
			printByte(number);
			transmitByte(' ');
			printDec(number);
			printCR(); 
			old_number = number;
		}
	}
	return 0;
}


/*********************************************
	Local Function Definition
**********************************************/

/*
	Update the number based on the switch
*/
static void updateNumberFromInput(uint8_t input,int switch_number, uint8_t* number){

	if((input & (1 << input_bit_shifts[switch_number])) == 0 ){ 
			*number |= (1 << number_bit_shifts[switch_number]);
		}
		else{
			*number &= ~(1 << number_bit_shifts[switch_number]);
		}
}

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
	convert a 8-bit number to its binary form
*/

static void printByte(uint8_t num){

  int c,k,j;
  j = 4;
  char bits[9] = {'0','0','0','0','0','0','0','0','\0'};

  for (c = 3; c >= 0; c--){
      k = num >> c;
      bits[j++] = (k & 1) ? '1' : '0';
    }
  transmitString(bits);

}

