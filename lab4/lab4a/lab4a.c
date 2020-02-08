#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <inttypes.h>

/*

Assignment:

i. Connect the IO Trainer board’s Vcc, GND and Pot pins to the Romeo board. The Pot pin can
connect to any Analog Port.

ii. Write a C program read the analog value of the pot and display it on the serial terminal. The value
must be displayed on the same line.

iii. Aside from the USART functions, the program must have 2 other functions, initADC and analog.
iv. The example functions In the lecture notes set the ADC for 10-bit resolution. Modify the
functions to output 8-bit resolution. 
	
*/

/*********************************************
	Macros & Enums
**********************************************/

#define F_CPU 16000000UL
#define BAUD 9600
/*********************************************
	Local Function Declaration
**********************************************/

static void initUART(uint32_t baud);
static void transmitByte(uint8_t data);
static void transmitString(char* stringPtr);
static void initADC(void);
static uint16_t analog(uint8_t channel);
static void printUint16(uint16_t num);
static void delayMs (uint16_t ms);
/*********************************************
	Main Function
**********************************************/

int main(int argc, char const *argv[]){

	initUART(BAUD);
	initADC();
	
	while(1){

		printUint16(analog(0)); //select ADC0 as the input channel
		transmitByte((uint8_t)' ');
		delayMs(500);
	}
	return 0;
}

/*********************************************
	Local Function Definition
**********************************************/

/*
	Initialize ADC such that it has the following configurations:

	1). Vref internally connect to AVcc
	2). Right adjusted for 8-bit resolution
	3). Disable digital buffer for analog input pins
*/

static void initADC(void){

	ADCSRA = 0;
	//enable ADC
	ADCSRA |= ( 1 << ADEN );
	//Vref internally connect to AVcc
	ADMUX |= ( 1 << REFS0 );

	//right adjusted for 8-bit resolution
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1);
	//disable digital input for ADC pins
	DIDR0 |= ((1 << ADC5D) | (1 << ADC4D) | (1 << ADC3D) | (1 << ADC2D) | (1 << ADC1D) | (1 << ADC0D));
	//start the first conversion
	ADCSRA |= (1 << ADSC);

	return;
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
	Analog to digital for 8-bit resolution
*/

static uint16_t analog(uint8_t channel){

	//clear MUX[3:0]
	ADMUX &= 0xF0; 
	//select channel
	ADMUX |= channel; 
	//start conversion
	ADCSRA |= (1 << ADSC);
	//wait for the conversion completed
	while(ADCSRA & ( 1 << ADSC));
	//ADIF=1
	ADCSRA |= (1 << ADIF);  

	return ADC;
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
	Print a uint16_t number to the serial console. The negtive sign is printed if the number is negative.
	In addition, necessary character shifting is also performed for pretty formatting. 
*/

static void printUint16(uint16_t num){

	char str[3];
	sprintf(str,"%hu",num);
	str[2] = '\0';
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
	delay function
*/

static void delayMs (uint16_t ms) {
	uint16_t i;
	for (i = 0; i < ms; i++)
		_delay_ms(1);	
}

