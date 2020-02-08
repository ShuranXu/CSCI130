#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
/*

Assignment:

i. Connect the Sharp GP2Y0A41SK0F IR distance sensor to the Romeo board.

ii. The Sharp sensor is a sensor that detects an object and returns a voltage that represents the
distance to that object. The detection range of this sensor is between 4cm and 30cm. Figure 6-1
located on page 8 of the sensor’s datasheet shows a graph of the output voltage vs. the distance to
reflective object. Write a C program that read in the analog value and displace the distance value
in cm. Note that the graph is non-linear. It is best to create a lookup table.

iii. The signal from the sensor can be very noisy. Participant MUST use some sort of filtering
method in code. The output of the program must be accurate within +/- 1cm.


Assumption taken:

1.Vcc must be 5V
2.No direct exposure of light for the sensor
	
*/

/*********************************************
	Macros & Enums
**********************************************/
#define F_CPU 			16000000UL
#define BAUD 			9600
#define DEBUG 			1
#define VCC_VAL 		5	
#define REGISTERED_NUM 	28

/*********************************************
	Struct Definition
**********************************************/
typedef struct sensor{

	uint32_t distance; //unit in cm
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
static void printUint32(uint32_t num);
static void delayMs (uint16_t ms);
static float convertToInputVoltage(uint32_t val);
static float absDiff(float y, float x);
static uint32_t getBestDistance(float voltage);
static float roundNum(float var);
/*********************************************
	Global Lookup Table
**********************************************/
static const sensor_t lookupTable[REGISTERED_NUM] = {

	{30,0.41},
	{29,0.43},
	{28,0.48},
	{27,0.5},
	{26,0.56},
	{25,0.55},
	{24,0.58},
	{23,0.59},
	{22,0.60},
	{21,0.62},
	{20,0.63},
	{19,0.70},
	{18,0.73},
	{17,0.77},
	{16,0.80},
	{15,0.89},
	{14,0.92},
	{13,1.0},
	{12,1.08},
	{11,1.18},
	{10,1.28},
	{9,1.40},
	{8,1.58},
	{7,1.79},
	{6,2.00},
	{5,2.40},
	{4,2.72},
	{3,3.00}
};
/*********************************************
	Main Function
**********************************************/

int main(int argc, char const *argv[]){

	initUART(BAUD);
	initADC();

	uint32_t dout = 0;
	float inputVol = 0.0;

	while(1){

		dout = (uint32_t)analog(0);  
		inputVol = roundNum(convertToInputVoltage(dout));
		if( inputVol < 0.40 ){
			transmitString("unrecognizable distance");
		}
		else{
			transmitString("distance = ");
			printUint32(getBestDistance(roundNum(inputVol)));
		}
		printCR();
		delayMs(500);
	}
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
	Obtain the input voltage value based on the ADC value
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

static uint32_t getBestDistance(float voltage){

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
