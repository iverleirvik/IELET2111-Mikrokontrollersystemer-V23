/*
 * Oving6_2_b.c
 *
 * Created: 28.02.2023 15:43:34
 * Author : Iver
 */ 

#include <avr/io.h>
#include <util/delay.h>

void tca0_init(void);
void led_init(void);

int main(void)
{
	tca0_init();
	led_init();
	
	
    /* Replace with your application code */
    while (1) 
    {
		TCA0.SINGLE.CMP0BUF = 0x00;	// 0x0032
		_delay_ms(500);
		TCA0.SINGLE.CMP0BUF = 0xFF;	// 0x00D0
		_delay_ms(500);
    }
}

void tca0_init(void)	{
	PORTMUX.TCAROUTEA = PORTMUX_TCA0_PORTD_gc;
	TCA0.SINGLE.CTRLB = TCA_SINGLE_CMP0EN_bm	|	TCA_SINGLE_WGMODE_DSBOTTOM_gc;
	TCA0.SINGLE.EVCTRL &= ~(TCA_SINGLE_CNTAEI_bm);
	TCA0.SINGLE.PERBUF = 0x01A0;//0x01F4;
	TCA0.SINGLE.CMP0BUF = 0x00D0;
	TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV4_gc	|	TCA_SINGLE_ENABLE_bm;
	}

void led_init(void)	{
	PORTD.DIR |= PIN0_bm;
}

