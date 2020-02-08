#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <avr/cpufunc.h>

/*

Assignment:

1) The sensor used in Theremin1 is the Sharp IR distance sensor.
2) Write a function that read the input from the Sharp sensor. Using the raw ADC value, bound and
normalize it to output a number between 0 and 4000.
3) When the user’s hand is close to the sensor the number should be reaching closer to 0. When the
user’s hand is further away from the sensor, the value should be close to 4000.

Connection Diagram:

Atmega328p | Romeo board 					| Sharp IR distance sensor

	PC0    |     A0      					| Vo   
	N/A    |     5V pin on POWER section    | VCC  
	N/A    |     GND pin on POWER section   | GND 
	
	
Atmega328p | 		Romeo board 			| IO Board | Jumper Component

	PB1    |     		D9      			|  JP4_2   | 	BZ1
	PD2    |     		D2      			|  JP2_5   |    S1

*/

/*********************************************
	Macros & Enums
**********************************************/

#define F_CPU 			16000000UL
#define BAUD 			9600
#define VCC_VAL 		5	
#define REGISTERED_NUM 	75

/*********************************************
	Struct Definition
**********************************************/

typedef struct sensor{

	float distance; //unit in cm
	float voltage;  //unit in V
}sensor_t;

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
static void initTimer(void);
static void tone(uint16_t frequency);
static void mute(void);
static float convertToInputVoltage(uint32_t val);
static float absDiff(float y, float x);
static float getBestDistance(float voltage);
static float roundNum(float var);
static void configureTimerOutput(void);

/*********************************************
	Global Lookup Table
**********************************************/

static const sensor_t lookupTable[] = {

  {40,0.30},
  {39.5,0.305},
  {39,0.31},
  {38.5,0.315},
  {38,0.32},
  {37.5,0.34},
  {37,0.35},
  {36.5,0.365},
  {36,0.37},
  {35.5,0.375},
  {35,0.38},
  {34.5,0.385},
  {34,0.39},
  {33.5,0.395},
  {33,0.40},
  {32.5,0.405},
  {32,0.41},
  {31.5,0.415},
  {31,0.42},
  {30.5,0.425},
  {30,0.43},
  {29.5,0.435},
  {29,0.44},
  {28.5,0.45},
  {28,0.46},
  {27.5,0.47},
  {27,0.48},
  {26.5,0.49},
  {26,0.50},
  {25.5,0.51},
  {25,0.56},
  {24.5,0.575},
  {24,0.58},
  {23.5,0.585},
  {23,0.590},
  {22.5,0.598},
  {22,0.60},
  {21.5,0.61},
  {21,0.62},
  {20.5,0.625},
  {20,0.63},
  {19.5,0.66},
  {19,0.69},
  {18.5,0.71},
  {18,0.73},
  {17.5,0.75},
  {17,0.77},
  {16.5,0.79},
  {16,0.80},
  {15.5,0.84},
  {15,0.89},
  {14.5,0.90},
  {14,0.92},
  {13.5,0.96},
  {13,1.0},
  {12.5,1.03},
  {12,1.07},
  {11.5,1.10},
  {11,1.18},
  {10.5,1.21},
  {10,1.28},
  {9.5,1.31},
  {9,1.40},
  {8.5,1.51},
  {8,1.58},
  {7.5,1.62},
  {7,1.79},
  {6.5,1.92},
  {6,2.00},
  {5.5,2.18},
  {5,2.40},
  {4.5,2.5},
  {4,2.72},
  {3.5,2.94},
  {3,3.00}
};

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

	uint32_t dout = 0;
	float val = 0;
	float inputVol;
	volatile uint8_t input = 0;
	uint16_t frequency;
	
	while(1){

		dout = (uint32_t)analog(0);  
		inputVol = convertToInputVoltage(dout);
		val = getBestDistance(roundNum(inputVol));
		frequency = (uint16_t)(val * 100);
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
	Print a uint16_t number to the serial console. 
*/

static void printUint16(uint16_t num){

	char str[3];
	memset(str,0,sizeof(str));
	sprintf(str,"%u",(unsigned int)num);
	transmitString(str);
}


/*
	Convert ADC value to corresponding input voltage
*/
static float convertToInputVoltage(uint32_t val){

	return ((val * VCC_VAL * 1.0)/1024);
}

/*
	Get the absolute difference between two variables.  
	This function is used for calculating offset to be used for indexing the lookup table.
*/

static float absDiff(float y, float x){

	if( y > x){
		return (y - x);
	}
	else{
		return (x - y);
	}
}

/*
	Get the distance value that fits the input voltage value.
*/

static float getBestDistance(float voltage){

	int i;
	float minVal = 99; //initialize to be some big number
	float currDiff;
	int index = 0;

	for(i=0;i<REGISTERED_NUM;++i){

		currDiff = absDiff(lookupTable[i].voltage,voltage);
		if( minVal > currDiff){
			minVal = currDiff;
			index = i;
		}
	}
	return lookupTable[index].distance;
}

/*
	Here we round the voltage input value with .2f precision
*/

static float roundNum(float var){

    // we use array of chars to store number as a string. 
    char str[40];  
    // Print in string the value of var with two decimal point 
    sprintf(str, "%.2f", var); 
    // scan string value in var  
    sscanf(str, "%f", &var);  
  
    return var;  
} 
