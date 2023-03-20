/*
 * Oving7_3_OneFile.c
 *
 * Created: 14.03.2023 19:31:46
 * Author : Iver
 */ 

/*
 * Oving7_3.c
 *
 * Created: 13.03.2023 09:50:38
 * Author : Iver
 */ 

/*
Copyright (c) [2012-2020] Microchip Technology Inc.  
    All rights reserved.
    You are permitted to use the accompanying software and its derivatives 
    with Microchip products. See the Microchip license agreement accompanying 
    this software, if any, for additional info regarding your rights and 
    obligations.
    
    MICROCHIP SOFTWARE AND DOCUMENTATION ARE PROVIDED "AS IS" WITHOUT 
    WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT 
    LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE, NON-INFRINGEMENT 
    AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP OR ITS
    LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT, NEGLIGENCE, STRICT 
    LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER LEGAL EQUITABLE 
    THEORY FOR ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES INCLUDING BUT NOT 
    LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES, 
    OR OTHER SIMILAR COSTS. 
    
    To the fullest extend allowed by law, Microchip and its licensors 
    liability will not exceed the amount of fees, if any, that you paid 
    directly to Microchip to use this software. 
    
    THIRD PARTY SOFTWARE:  Notwithstanding anything to the contrary, any 
    third party software accompanying this software is subject to the terms 
    and conditions of the third party's license agreement.  To the extent 
    required by third party licenses covering such third party software, 
    the terms of such license will apply in lieu of the terms provided in 
    this notice or applicable license.  To the extent the terms of such 
    third party licenses prohibit any of the restrictions described here, 
    such restrictions will not apply to such third party software.
*/

//#include <xc.h>

#define F_CPU	4000000UL
#define USART3_BAUD_RATE(BAUD_RATE) ((float)(F_CPU * 64 / (16 * (float)BAUD_RATE)) + 0.5)

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <util/delay.h>

FUSES = {
	.WDTCFG = 0x00, // WDTCFG {PERIOD=OFF, WINDOW=OFF}
	.BODCFG = 0x06, // BODCFG {SLEEP=SAMPLE, ACTIVE=ENABLE, SAMPFREQ=128Hz, LVL=BODLEVEL0}
	.OSCCFG = 0x00, // OSCCFG {CLKSEL=OSCHF}
	.SYSCFG0 = 0xC8, // SYSCFG0 {EESAVE=CLEAR, RSTPINCFG=RST, CRCSEL=CRC16, CRCSRC=NOCRC}
	.SYSCFG1 = 0x0C, // SYSCFG1 {SUT=8MS, MVSYSCFG=DUAL}
	.CODESIZE = 0x00, // CODESIZE {CODESIZE=User range:  0x0 - 0xFF}
	.BOOTSIZE = 0x00, // BOOTSIZE {BOOTSIZE=User range:  0x0 - 0xFF}
};

LOCKBITS = 0x5CC5C55C; // {KEY=NOLOCK}

#define DATA_SIZE 16
#define TWI_READ true
#define TWI_WRITE false

void USART3_init(void);
int USART3_printChar(const char character, FILE *stream);
void TWI_initClient(uint8_t address);
void TWI_initPins(void);
void _TWI_StoreByte(uint8_t data);
uint8_t _TWI_RequestByte(void);
void _onTWIStop(void);
void setupReadBuffer(volatile uint8_t* buffer, uint8_t size);
void setupWriteBuffer(volatile uint8_t* buffer, uint8_t size);


FILE USART_stream = FDEV_SETUP_STREAM(USART3_printChar, NULL, _FDEV_SETUP_WRITE);

static volatile bool isFirst = true;
static volatile bool wasRead = false;

static volatile uint8_t i2c_index = 0;

static volatile uint8_t* writeBuffer = 0;
static volatile uint8_t writeBufferSize = 0;

static volatile uint8_t* readBuffer = 0;
static volatile uint8_t readBufferSize = 0;

int main(void)
{           
	
	PORTB.DIRSET = PIN3_bm;
	
	USART3_init();
	
    //Setup CPU Clocks
    
    //Setup TWI I/O
    TWI_initPins();
    
    //Setup TWI Interface
    TWI_initClient(0x4D);
    
    //Data to R/W to. (Must be volatile)
    volatile uint8_t data[DATA_SIZE];
    
    //Initialize Memory to 0x00
    for (uint8_t i = 0; i < DATA_SIZE; i++)
    {
        data[i] = 0x00;
    }
    
    //Attach R/W Buffers
    //Note: The buffers can be separated, or they can be the same memory, if desired.
    setupReadBuffer(&data[0], 16);
    setupWriteBuffer(&data[8], 8);
    
    //Enable Interrupts
    sei();
        
    while (1)
    {   
        //Wait...
			printf("readBuffer %d\n", *readBuffer);	
			printf("writeBuffer %d\n", *writeBuffer);
			_delay_ms(500);
    }
}


void USART3_init(void)
{
	PORTB.DIRSET = PIN0_bm;
	PORTB.DIRCLR = PIN1_bm;
	USART3.BAUD = (uint16_t)USART3_BAUD_RATE(9600);
	USART3.CTRLB |= USART_TXEN_bm | USART_RXEN_bm;
	stdout = &USART_stream;
}

int USART3_printChar(const char character, FILE *stream)
{
	while (!(USART3.STATUS & USART_DREIF_bm))	// Sjekker om forrige transmisjon er ferdig.
	{
		;
	}
	USART3.TXDATAL = (character); // Kan evt. bruke printf.
	
	return 0;
}













void TWI_initClient(uint8_t address)
{
	//Init Function Pointers to Null
	
	//Enable Operation in Debug
	TWI0.DBGCTRL = TWI_DBGRUN_bm;
	
	//Set Client I2C Address
	TWI0.SADDR = address << 1;
	
	//Enable Data Interrupts, Address/Stop Interrupts, Enable STOP, and the TWI Peripheral
	TWI0.SCTRLA = TWI_DIEN_bm | TWI_APIEN_bm | TWI_PIEN_bm | TWI_ENABLE_bm;
}

void TWI_initPins(void)
{
	//PA2/PA3

	//Output I/O
	PORTA.DIRSET = PIN2_bm | PIN3_bm;

	#ifdef TWI_ENABLE_PULLUPS
	//Enable Pull-Ups
	PORTA.PINCONFIG = PORT_PULLUPEN_bm;
	#endif
	
	//Select RA2/RA3
	PORTA.PINCTRLUPD = PIN2_bm | PIN3_bm;
}

//void __interrupt(TWI0_TWIS_vect) TWI0_ISR(void)
ISR (TWI0_TWIS_vect)
{	printf("HEELLLO\n");
	//printf("status:%04x \n", TWI0.SSTATUS);
	//printf("sdata:%04x \n", TWI0.SDATA);
	//printf("saddr:%04x \n", TWI0.SADDR);
	//printf("JE: %d", TWI0.SSTATUS);
	//PORTB.IN= PIN3_bm;

	if (TWI0.SSTATUS & TWI_DIF_bm)
	{
		printf("status:%04x \n", TWI0.SSTATUS);
		
		PORTB.OUTTGL=PIN3_bm;
		printf("erlend\n");
		//Data Interrupt Flag
		uint8_t data = 0x00;
		
		if (((TWI0.SSTATUS & TWI_DIR_bm) >> TWI_DIR_bp) == TWI_WRITE)
		{
			//Data Write (Host -> Client)
			data = TWI0.SDATA;
			_TWI_StoreByte(data);
			//printf("georg");
		}
		else
		{
			//printf("iver");
			//Data Read (Host <- Client)
			data = _TWI_RequestByte();

			TWI0.SDATA = data;
		}
		
		//ACK
		TWI0.SCTRLB = TWI_ACKACT_ACK_gc | TWI_SCMD_RESPONSE_gc;
	}
	
	//printf("status:%04x \n", TWI0.SSTATUS);
	
	if (TWI0.SSTATUS & TWI_APIF_bm)
	{
		printf("apif\n");
		//Address Match or STOP
		if (TWI0.SSTATUS & TWI_AP_ADR_gc)
		{
			//Address Match
			TWI0.SCTRLB = TWI_ACKACT_ACK_gc | TWI_SCMD_RESPONSE_gc;
		}
		else
		{
			//STOP Condition
			_onTWIStop();
		}
			
		TWI0.SCTRLB = TWI_ACKACT_NACK_gc | TWI_SCMD_COMPTRANS_gc;
	}
}












void _TWI_StoreByte(uint8_t data)
{
	#ifdef FIRST_BYTE_ADDR                                                          // If set, treat the 1st byte as an index
	if (!isFirst)
	{
		if (i2c_index < writeBufferSize)
		{
			writeBuffer[i2c_index] = data;
			i2c_index++;
		}
	}
	else
	{
		isFirst = false;
		i2c_index = data;
	}
	#else
	if (i2c_index < writeBufferSize)
	{
		writeBuffer[i2c_index] = data;
		i2c_index++;
	}
	#endif
}

uint8_t _TWI_RequestByte(void)
{
	wasRead = true;
	uint8_t data = 0x00;
	if (i2c_index < readBufferSize)
	{
		data = readBuffer[i2c_index];
		i2c_index++;
	}
	else
	{
		//This line is to correct for the extra byte loaded, but not sent when stopped.
		i2c_index = readBufferSize + 1;
	}
	return data;
}

void _onTWIStop(void)
{
	#ifdef FIRST_BYTE_ADDR
	if ((wasRead) && (i2c_index != 0))
	{
		// If reading bytes, an extra byte is loaded but not sent when stopped.
		i2c_index--;
	}
	#else
	//Reset the index
	i2c_index = 0;
	#endif
	
	isFirst = true;
	wasRead = false;
}

void setupReadBuffer(volatile uint8_t* buffer, uint8_t size)
{
	readBuffer = buffer;
	readBufferSize = size;
}

void setupWriteBuffer(volatile uint8_t* buffer, uint8_t size)
{
	writeBuffer = buffer;
	writeBufferSize = size;
}
