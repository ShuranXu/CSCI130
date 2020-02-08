#include <avr/io.h>
#include <util/delay.h>

/*

Assignment:

i. Write a C program that waits for input from the serial terminal, when an input is received,
transmit the next character on the ASCII table on a new line. For example:

Input: A
Output: B
Input: s
Output: t

ii. The program must have 3 functions, initUART, transmitByte, receiveByte from the lecture notes.

iii. The participant may choose any baud rate, however the initUART function must run the USART
in double speed mode.
	
*/

/*********************************************
	Macros
**********************************************/

#define F_CPU 16000000UL
#define BAUD 9600

/*********************************************
	Local Function Declaration
**********************************************/

static void initUART(uint32_t baud);
static unsigned char receiveByte( void );
static void transmitByte(uint8_t data);
static void printCR(void);
static void transmitString(char* stringPtr);

/*********************************************
	Main Function
**********************************************/

int main(int argc, char const *argv[]){

	initUART(BAUD);
	uint8_t data_recv= 0;
	uint8_t input_msg[] = "Input: ";
	uint8_t output_msg[] = "Output: ";
	printCR();

	while(1){

		transmitString(input_msg);
		data_recv = receiveByte();
		//print out the value that the user passed in
		transmitByte(data_recv);
		printCR();
		//print out "Output: " to the serial console
		transmitString(output_msg);
		//print out the next ASCII value to the serial console
		transmitByte(data_recv + 1); 
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
	print a string in the serial console
*/

static void transmitString(char* stringPtr){

	char* iter = stringPtr;
	while(*iter != '\0'){
		transmitByte(*iter++);
	}
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
   Read byte from UART
*/ 
static unsigned char receiveByte( void ){
	// Wait for incoming byte  
   	while (!(UCSR0A &  (1 << RXC0)));
   // Return the byte
   return UDR0;
} 


