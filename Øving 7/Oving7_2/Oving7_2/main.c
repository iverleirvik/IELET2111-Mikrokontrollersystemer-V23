/*
 * Oving7_2.c
 *
 * Created: 12.03.2023 11:25:50
 * Author : Iver
 */ 


#define F_CPU	4000000UL
#define USART3_BAUD_RATE(BAUD_RATE) ((float)(F_CPU * 64 / (16 * (float)BAUD_RATE)) + 0.5)

#include <avr/io.h>
#include <stdbool.h>
#include <stdio.h>

#define TWI_READ true
#define TWI_WRITE false

#define I2C_SLAVE_ADDRESS                               0x4D
#define I2C_DIRECTION_BIT_WRITE                         0

#define TWI_IS_CLOCKHELD() TWI0.MSTATUS & TWI_CLKHOLD_bm
#define TWI_IS_BUSERR() TWI0.MSTATUS & TWI_BUSERR_bm
#define TWI_IS_ARBLOST() TWI0.MSTATUS & TWI_ARBLOST_bm

#define CLIENT_NACK() TWI0.MSTATUS & TWI_RXACK_bm
#define CLIENT_ACK() !(TWI0.MSTATUS & TWI_RXACK_bm)

#define TWI_IS_BUSBUSY() ((TWI0.MSTATUS & TWI_BUSSTATE_BUSY_gc) == TWI_BUSSTATE_BUSY_gc)
//#define TWI_IS_BAD() ((TWI_IS_BUSERR()) | (TWI_IS_ARBLOST()) | (CLIENT_NACK()) | (TWI_IS_BUSBUSY()))

#define TWI_WAIT() while (!((TWI_IS_CLOCKHELD()) || (TWI_IS_BUSERR()) || (TWI_IS_ARBLOST()) || (TWI_IS_BUSBUSY())))

void TWI_initHost(void);
void TWI_initPins(void);
bool _startTWI(uint8_t addr, bool read);
bool _writeToTWI(uint8_t* data, uint8_t len);
bool TWI_sendByte(uint8_t addr, uint8_t data);
bool TWI_sendBytes(uint8_t addr, uint8_t* data, uint8_t len);
void _readFromTWI(uint8_t* data, uint8_t len);
bool TWI_readByte(uint8_t addr, uint8_t* data);
bool TWI_readBytes(uint8_t addr, uint8_t* data, uint8_t len);

void USART3_init(void);
int USART3_printChar(const char character, FILE *stream);

FILE USART_stream = FDEV_SETUP_STREAM(USART3_printChar, NULL, _FDEV_SETUP_WRITE);

int main(void)	{
	
	USART3_init();
	
	TWI_initHost();
	TWI_initPins();
	
	uint8_t *dataen;
	
	PORTE.DIRSET = PIN1_bm;
	
	while (1)
	{
		TWI_readByte(I2C_SLAVE_ADDRESS, dataen);
		printf("%d\n", *dataen);
		//printf("HELLLO\n");
	}
	
	return 0;
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

bool isTWIBad(void)
{
	//Checks for: NACK, ARBLOST, BUSERR, Bus Busy
	if ((((TWI0.MSTATUS) & (TWI_RXACK_bm | TWI_ARBLOST_bm | TWI_BUSERR_bm)) > 0  )
	|| (TWI_IS_BUSBUSY()))
	{
		//printf("seks\n");
		return true;
	}
	return false;
}

void TWI_initHost(void)
{
	//Standard 100kHz TWI, 4 Cycle Hold, 50ns SDA Hold Time
	TWI0.CTRLA = TWI_SDAHOLD_50NS_gc;
	
	//Clear Dual Control
	TWI0.DUALCTRL = 0x00;
	
	//Enable Run in Debug
	TWI0.DBGCTRL = TWI_DBGRUN_bm;
	
	//Clear MSTATUS (write 1 to flags). BUSSTATE set to idle
	TWI0.MSTATUS = TWI_RIF_bm | TWI_WIF_bm | TWI_CLKHOLD_bm | TWI_RXACK_bm |
	TWI_ARBLOST_bm | TWI_BUSERR_bm | TWI_BUSSTATE_IDLE_gc;
	
	//Set for 100kHz from a 4MHz oscillator
	TWI0.MBAUD = 15;
	
	//[No ISRs] and Host Mode
	TWI0.MCTRLA = TWI_ENABLE_bm;

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

bool _startTWI(uint8_t addr, bool read)
{
	//If the Bus is Busy
	if (TWI_IS_BUSBUSY())
	{
		//printf("fire\n");
		return false;
	}
	
	//Send Address
	TWI0.MADDR = (addr << 1) | read;
	
	//Wait...
	TWI_WAIT();
	
	if (isTWIBad())
	{
		//Stop the Bus
		TWI0.MCTRLB = TWI_MCMD_STOP_gc;
		//printf("fem\n");
		return false;
	}
	
	//TWI Started
	return true;
}

//Internal Function: Write len bytes to TWI. Does NOT STOP the bus. Returns true if successful
bool _writeToTWI(uint8_t* data, uint8_t len)
{
	uint8_t count = 0;
	
	while (count < len)
	{
		//Write a byte
		TWI0.MDATA = data[count];
		//Wait...
		TWI_WAIT();
		
		//If the client NACKed, then abort the write
		if (CLIENT_NACK())
		{
			return false;
		}
		else if(CLIENT_ACK())	{
			count++;
			PORTE.OUT ^= PIN1_bm;
		}
		
		//Increment the counter
		//count++;
	}
	
	return true;
}

bool TWI_sendByte(uint8_t addr, uint8_t data)
{
	//Address Client Device (Write)
	if (!_startTWI(addr, TWI_WRITE))
	return false;
	
	bool success = _writeToTWI(&data, 1);
	
	//Stop the bus
	TWI0.MCTRLB = TWI_MCMD_STOP_gc;
	
	return success;
}

bool TWI_sendBytes(uint8_t addr, uint8_t* data, uint8_t len)
{
	//Address Client Device (Write)
	if (!_startTWI(addr, TWI_WRITE))
	return false;
	
	//Write the bytes to the client
	bool success = _writeToTWI(data, len);

	//Stop the bus
	TWI0.MCTRLB = TWI_MCMD_STOP_gc;
	
	
	
	return success;
}

void _readFromTWI(uint8_t* data, uint8_t len)
{
	uint8_t bCount = 0;
	
	//Release the clock hold
	
	TWI0.MSTATUS = TWI_CLKHOLD_bm;
	
	//TWI0.MCTRLB = TWI_MCMD_RECVTRANS_gc;
	
	while (bCount < len)
	{
		//Wait...
		TWI_WAIT();
		
		//Store data
		data[bCount] = TWI0.MDATA;
		/**/
		printf("real: %d\n", data[bCount]);
		//Increment the counter
		bCount += 1;
		
		if (bCount != len)
		{
			//If not done, then ACK and read the next byte
			TWI0.MCTRLB = TWI_ACKACT_ACK_gc | TWI_MCMD_RECVTRANS_gc;
		}
	}
	
	//NACK and STOP the bus
	TWI0.MCTRLB = TWI_ACKACT_NACK_gc | TWI_MCMD_STOP_gc;
}


bool TWI_readByte(uint8_t addr, uint8_t* data)
{
	printf("to\n");
	//Address Client Device (Read)
	if (!_startTWI(addr, TWI_READ)) return false;
	
	//Read byte from client
	_readFromTWI(data, 1);
	printf("tre\n");

	return true;
}


bool TWI_readBytes(uint8_t addr, uint8_t* data, uint8_t len)
{
	//Address Client Device (Read)
	if (!_startTWI(addr, TWI_READ))
	return false;
	
	//Read bytes from client
	_readFromTWI(data, len);
	
	return true;
}


