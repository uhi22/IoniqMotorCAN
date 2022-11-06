
/*
  SPI-Treiber für AVR
  Uwe Hennig
  
  Änderungshistorie:
  2014-12-04 Uwe:
    - aus LichtCAN übernommen. Kann nun sowohl Software-SPI als auch Hardware-SPI.
  2014-12-09 Uwe:
    - umgezogen nach AVR/Lib/SPI.
	- Aufgeräumt: Änderungshistorie dazu. Auskommentierte Header raus.

  2015-06-12 Uwe:
    - SPI_SLAVESELECT nun optional: Falls nicht definiert, wird auch kein Port beeinflusst.

*/



#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#include "board.h"
#include "spi.h"


#define SPIPutCharWithoutWaiting(a) spiF_talkSpi(a)
#define SPIPutChar(a) spiF_talkSpi(a)
#define SPIWait() {}

uint8_t spiTxByte;
uint8_t spiRxByte;


#ifdef HARDWARE_SPI

void spi_put( unsigned char value )
{
	//ENC_DEBUG("spi_put(%2x)\n", (unsigned) value );
	SPDR = value;
	while( !(SPSR & (1<<SPIF)) ) ;
}

unsigned char spi_get(void)
{
	unsigned char value = SPDR;
	//ENC_DEBUG("spi_get()=%2x\n", (unsigned) value );
	return value;
}

#ifdef TALK_SPI_REAL_FUNCTION
unsigned char spiF_talkSpi(unsigned char value) {
  SPDR = value;
  while(!(SPSR & (1<<SPIF)));
  return SPDR;
}
#endif

#else
/* software - SPI */

//   Send nClocks x 8 clock transitions with MOSI=1 (0xff). 

void spiF_8Clocks  (unsigned char  nClocks) {
  while (nClocks--){
    SPIPutCharWithoutWaiting(0xff);
    SPIWait();
  }
}
//
//
//
uint8_t spiF_talkSpi(uint8_t b) {

 uint8_t i;
 
 spiTxByte = b;
 SOFTSPI_CLK_LO;
 for (i=0;i<8; i++) {
   // zuerst MSB, zuletzt LSB.
   // Daten setzen, dann Clock 1, Clock 0
   spiRxByte<<=1;
   if (SOFTSPI_MISO) spiRxByte |= 1;   
   if (spiTxByte & 0x80) { SOFTSPI_MOSI_HI;} else { SOFTSPI_MOSI_LO;}
   SOFTSPI_DELAY;
   SOFTSPI_CLK_HI;
   SOFTSPI_DELAY;
   SOFTSPI_CLK_LO;
   SOFTSPI_DELAY;
   spiTxByte<<=1;
 }
 #ifdef MIT_SPI_BYTE_DEBUG
  printConsoleString_p(PSTR("\x0dSPI "));
  printConsoleHexU8(b);
  printConsoleString_p(PSTR(" "));
  printConsoleHexU8(spiRxByte);
 #endif 
 return spiRxByte;
}

#endif

void spiF_initInterface(void) {
  #ifdef HARDWARE_SPI
	// configure pins MOSI, SCK and slave selects as output
    SPI_DDR |= (1<<SPI_MOSI) | (1<<SPI_SCK) | (1<<SPI_SLAVESELECT1);
	#ifdef SPI_SLAVESELECT2 
	   SPI_DDR |= (1<<SPI_SLAVESELECT2) 
	#endif

	// pull SCK high
	//SPI_PORT |= (1<<SPI_SCK);

	// configure pin MISO as input
	//SPI_DDR &= ~(1<<SPI_MISO);


	// chipselect Ausgang und high
	/* Vorsicht: Spezialverhalten des /SS Pins: Laut Datenblatt muss er als
	   Ausgang konfiguriert sein, auch wenn er keinen Slave steuert. Würde er
	   als Eingang und Low sein, interpretiert das die SPI als "fremder Master" und
	   wechselt in den Slave-Mode!!! */
	//DDRC |= 1; PORTC |= 1;
    SPI_PORT |= (1 << SPI_SLAVESELECT1);
	#ifdef SPI_SLAVESELECT2
      SPI_PORT |= (1 << SPI_SLAVESELECT2);    // SS HIGH
    #endif

	//SPI: enable, master, positive clock phase, msb first, SPI speed fosc/2
    SPCR =  (1<<SPE) | (1<<MSTR) | (0<<SPR1)| (0<<SPR0) ;
	// Double Clock --> maximum clock is XTAL/2=8MHz/2=4MHz
	//                                          16 MHz/2 = 8 MHz.
	// Der MCP2515 verträgt maximal 10 MHz SPI-Takt. --> ok
	SPSR = (1<<SPI2X);

	_delay_us(100);    
  #else
    SOFTSPI_INIT;
  #endif
}
