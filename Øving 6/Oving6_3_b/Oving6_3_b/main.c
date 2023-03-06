/*
 * Oving6_3_b.c
 *
 * Created: 01.03.2023 13:19:03
 * Author : Iver
 */ 

#define F_CPU 4000000UL
#include <avr/io.h>
#include <util/delay.h>

void tca0_init(void);
void led_init(void);
void ADC0_init(void);
uint16_t ADC0_read();

int main(void)
{
	tca0_init();
	led_init();
	
	ADC0_init();
	
	
	
    /* Replace with your application code */
    while (1) 
    {
		TCA0.SINGLE.CMP0BUF = ADC0_read();
    }
}

void tca0_init(void)	{
	PORTMUX.TCAROUTEA = PORTMUX_TCA0_PORTD_gc;
	TCA0.SINGLE.CTRLB = TCA_SINGLE_CMP0EN_bm	|	TCA_SINGLE_WGMODE_DSBOTTOM_gc;
	TCA0.SINGLE.EVCTRL &= ~(TCA_SINGLE_CNTAEI_bm);
	TCA0.SINGLE.PERBUF = 0x02EE;	//0x01F4; Periode
	TCA0.SINGLE.CMP0BUF = 0x2710;	// Duty cycle
	TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV4_gc	|	TCA_SINGLE_ENABLE_bm;
	}

void led_init(void)	{
	PORTD.DIR |= PIN0_bm;
}

void ADC0_init(void)
{
	/* Disable digital input buffer */
	PORTD.PIN4CTRL &= ~PORT_ISC_gm;
	PORTD.PIN4CTRL |= PORT_ISC_INPUT_DISABLE_gc;
	
	/* Disable pull-up resistor */
	PORTD.PIN4CTRL &= ~PORT_PULLUPEN_bm;

	ADC0.CTRLC = ADC_PRESC_DIV4_gc	/* CLK_PER divided by 4 */
	| VREF_REFSEL_VDD_gc ;   /* VDD as reference */
	
	
	ADC0.CTRLA = ADC_ENABLE_bm          /* ADC Enable: enabled */
	| ADC_RESSEL_10BIT_gc;   /* 10-bit mode */
	
	/* Select ADC channel */
	ADC0.MUXPOS  = ADC_MUXPOS_AIN4_gc;
	
	/* Enable FreeRun mode */
	ADC0.CTRLA |= ADC_FREERUN_bm;
}

uint16_t ADC0_read(void)
{
	/* Start ADC conversion */
	ADC0.COMMAND = ADC_STCONV_bm;
	
	/* Wait until ADC conversion done */
	if ( (ADC0.INTFLAGS & ADC_RESRDY_bm) )
	{
		ADC0.INTFLAGS = ADC_RESRDY_bm;
		
		return ADC0.RES;
	}
	
	/* Clear the interrupt flag by writing 1: */
	//ADC0.INTFLAGS = ADC_RESRDY_bm;
	
	//return ADC0.RES;
}
