#include <avr/io.h>
#define F_CPU 16000000UL
#include <util/delay.h>

void delay_ms (uint16_t ms);
void delay_us (uint16_t us);


/*
	Main function
*/
int main(void) {
	DDRB |= (1 << PB5);
	while(1) {
		PORTB &= ~(1 << PB5);
		delay_ms(100);
		PORTB |= (1 << PB5);
		delay_ms(100);
	}

	return 0;
}


void delay_ms (uint16_t ms) {
	uint16_t i;
	for (i = 0; i < ms; i++)
		_delay_ms(1);
		
}
void delay_us (uint16_t us) {
	uint16_t i;
	
	for (i = 0; i < us; i++)
		_delay_us(1);
		
}
