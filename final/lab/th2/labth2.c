#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <avr/cpufunc.h>

/*

Assignment:

1) The sensor used in Theremin2 is the Pololu Reflectance Sensor Array.
2) As the user move their finger across the sensor, the program should return the position of the finger.
In Lab4B the sensor returns the position between 1000 and 8000. Modify it such that the position
returned should be between 0 and 4000.


Connection Diagram:

Atmega328p | 		Romeo board 		| Pololu Reflectance Sensor Array

	PC0    |     A0      				| pin 1    
	PC1    |     A1      				| pin 2   
	PC2    |     A2      				| pin 3 
	PC3    |     A3      				| pin 4   
	PC4    |     A4      				| pin 5   
	PC5    |     A5      				| pin 6  
	ADC6   |     A6      				| pin 7   
	ADC7   |     A7      				| pin 8 
	N/A    | 5V pin on POWER section 	| Vcc
	N/A.   | GND pin on POWER section   | GND 


Atmega328p | 		Romeo board 		| IO Board | Jumper Component

	PB1    |     		D9      		|  JP4_2   | 	BZ1
	PD2    |     		D2      		|  JP2_5   |    S1
	
*/

/*********************************************
	Macros & Enums
**********************************************/

#define F_CPU 		16000000UL
#define BAUD 		9600
#define ARRAY_LEN  	(8)
#define DEBUG 		0
/*********************************************
	Local Function Declaration
**********************************************/

static void initUART(uint32_t baud);
static void transmitByte(uint8_t data);
static void printCR(void);
static void transmitString(char* stringPtr);
static void initADC(void);
static uint16_t analog(uint8_t channel);
static void printUint16(uint16_t num);
static void delayMs (uint16_t ms);
static uint16_t calPosition( uint32_t vals[], int len);
static void inverseData(uint32_t max, uint32_t vals[],int len);
static void initTimer(void);
static void tone(uint16_t frequency);
static void mute(void);
static void configureTimerOutput(void);
/*********************************************
	Main Function
**********************************************/

int main(int argc, char const *argv[]){

	initUART(BAUD);
	initADC();
	initTimer();
	
	//set D2 as input pins 
	DDRD &=~(1 << PD2);
	//set up pull-up resistor
	PORTD |=(1 << PD2);

	uint32_t dout[ARRAY_LEN] = {0};
	uint32_t max = 0;
	uint16_t frequency;
	volatile uint8_t input = 0;
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
		frequency = calPosition(dout,ARRAY_LEN);
		printUint16(frequency);
		printCR();

		//wait for sync
		_NOP();
		input = PIND;
		
		if( ((input >> 2) & 0b00000001) == 0 ){ //press S1
			configureTimerOutput();
			tone(frequency);
		}
		else{
			mute();
		}
		delayMs(500);
	}
	return 0;
}


/*********************************************
	Local Function Definition
**********************************************/

/*

initTimer() should:

a) Use ICRx as TOP
b) Tie the 16-bit timer to one of the two output pins(OCxA/B) and set it as an output
c) Clear OCxA/B when match
d) Fast PWM mode
e) 1/8 prescale

*/

static void initTimer(void){

 	// clear OC1A/B when match set OC1A when BOTTOM. Set mode 14
	TCCR1A = (1 << COM1A1) | (1 << WGM11);
	// timer uses main system clock with 1/8 prescale
	TCCR1B = (1 << WGM13)| (1 << WGM12) | (1 << CS11); 
	// make ICR1 used for TOP, default for 500 Hz PWM
	ICR1 = 4000; 
	//make OCR1A defaults to 2000
	OCR1A = 2000;
	// set OC1A pin as output
	DDRB |= (1 << PB1); 
}

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

tone(uint16_t frequency) must control the 16-bit
timer to produce a square wave that has 50% duty cycle and the frequency of the wave should match
the parameter.

*/
static void tone(uint16_t frequency){

	uint16_t tmp;
	tmp = ((uint16_t)(F_CPU / frequency) >> 3) - 1;
	OCR1A = tmp;
	ICR1 = (tmp << 1) + 1;
}

/*

“mute(void)” function. This function will stop the tone. This can be done by
setting the data direction from output to input and clearing the pin.

*/

static void mute(void){

	// set OC1A pin as input
	DDRB &= ~(1 << PB1); 
	//clear the pin.
	PORTB &= ~(1 << PB1);
}

/*

configureTimerOutput( ) resets and configures the pins on port B so that OC1A pin can be the timer output

*/
static void configureTimerOutput(void){

	DDRB = 0;
	PORTB = 0;
	// set OC1A pin as output
	DDRB |= (1 << PB1); 
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
	Print a uint16_t number to the serial console. 
*/

static void printUint16(uint16_t num){

	char str[3];
	memset(str,0,sizeof(str));
	sprintf(str,"%u",(unsigned int)num);
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

static uint16_t calPosition( uint32_t vals[], int len){

  int i;
  uint32_t sum = 0;
  uint32_t mul = 0;

  for(i=0;i<len;++i){

    sum += vals[i];
    mul += 570 * i * vals[i];
    // mul += 1050 *(i+1)*vals[i];
  }
  return (uint16_t)(mul/sum);
}

/*
	Inverse data elements of array 'vals'
*/
static void inverseData(uint32_t max, uint32_t vals[],int len){

	int i = 0;
	for(;i<len;++i){
		vals[i] = max - vals[i];
	}
}


