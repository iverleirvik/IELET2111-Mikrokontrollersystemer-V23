/*
 * Oving6.c
 *
 * Created: 28.02.2023 13:41:28
 * Author : Iver
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>

void tca0_INIT(void);
void led_INIT(void);

int main(void)
{
	
	tca0_INIT();
	led_INIT();
	
    /* Replace with your application code */
    while (1) 
    {
		;
    }
}

ISR(TCA0_OVF_vect)	{
	
	PORTB.OUT ^= PIN3_bm;
	
	TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
}


void tca0_INIT(void)	{
	
	TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
	TCA0.SINGLE.CTRLA = TCA_SINGLE_ENABLE_bm | TCA_SINGLE_CLKSEL_DIV256_gc;
	TCA0.SINGLE.PERBUF = 15625;
	sei();
}

void led_INIT(void)	{
	
	PORTB.DIRSET = PIN3_bm;
}

