#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

/*

Assignment:

i. Connect all 8 ouputs of the Pololu Reflectance Sensor Array to the analog ports on the Romeo
board. Remember to connect the Vcc and GND and remember the special note about A7.

ii. Rewrite the initADC and analog functions to output 10-bit resolution.

iii. Using the program written in 4A(with the rewritten 10-bit functions), test the values of the
sensors by placing a finger over one of the sensors in the array. Note how the value changes as
the finger moves closer/further away from a sensor.

iv. Write a C program that displays a number that represents the position of one’s finger on the array.
The number should be a continuous number between 1000 and 8000. For example, if one finger is
placed squarely on Sensor 4, the position is 4000. As the finger shift to Sensor 5, the number
should linearly increase to 5000.

Hint 1: Participant can write the program using any algorithm but the following formula might be
useful.

𝑝𝑜𝑠𝑖𝑡𝑖𝑜𝑛 =
1000 · 𝑉𝑎𝑙0 + 2000 · 𝑉𝑎𝑙1 + ⋯ + 8000 · 𝑉𝑎𝑙8
𝑉𝑎𝑙0 + 𝑉𝑎𝑙1 + ⋯ + 𝑉𝑎𝑙8

Hint 2: Remember to take into account (4B iii)
	
*/

/*********************************************
	Macros & Enums
**********************************************/

#define F_CPU 		16000000UL
#define BAUD 		9600
#define ARRAY_LEN  	(8)

/*********************************************
	Local Function Declaration
**********************************************/

static void initUART(uint32_t baud);
static void transmitByte(uint8_t data);
static void printCR(void);
static void transmitString(char* stringPtr);
static void initADC(void);
static uint16_t analog(uint8_t channel);
static void printUint32(uint32_t num);
static void delayMs (uint16_t ms);
static uint32_t calPosition( uint32_t vals[], int len);
static void inverseData(uint32_t max, uint32_t vals[],int len);
/*********************************************
	Main Function
**********************************************/

int main(int argc, char const *argv[]){

	initUART(BAUD);
	initADC();
	
	uint32_t dout[ARRAY_LEN] = {0};
	uint32_t max = 0;
	int i;

	while(1){

		max = 0;//reset
		for(i = 0;i<ARRAY_LEN;++i){
			dout[i] = analog(i); //capture sensor outputs
			if(max < dout[i]){
				max = dout[i]; //update the maximum value for each occurance of new sensor outputs
			}
		}
		inverseData(max,dout,ARRAY_LEN);
		printUint32(calPosition(dout,ARRAY_LEN));
		
		printCR();
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
	//right adjusted for 10-bit resolution
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
	//disable digital input for ADC pins
	DIDR0 = 0;
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

	return ADC;
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
	Print a uint32_t number to the serial console. The negtive sign is printed if the number is negative.
	In addition, necessary character shifting is also performed for pretty formatting. 
*/

static void printUint32(uint32_t num){

	char str[5];
	memset(str,0,sizeof(str));
	sprintf(str,"%u",(unsigned int)num);
	str[4] = '\0';
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

/*
	Calculate the position value based on the current setting
*/
static uint32_t calPosition( uint32_t vals[], int len){

  int i;
  uint32_t sum = 0;
  uint32_t mul = 0;

  for(i=0;i<len;++i){

    sum += vals[i];
    mul += 1050 *(i+1)*vals[i];
  }
  return (uint32_t)(mul/sum);
}


static void inverseData(uint32_t max, uint32_t vals[],int len){

	int i = 0;
	for(;i<len;++i){
		vals[i] = max - vals[i];
	}
}


