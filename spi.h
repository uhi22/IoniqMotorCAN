/*
  SPI-Treiber für AVR
  Uwe Hennig
  
  Änderungshistorie:
  2014-12-04 Uwe:
    - aus LichtCAN übernommen. Kann nun sowohl Software-SPI als auch Hardware-SPI.
  2014-12-09 Uwe:
    - umgezogen nach AVR/Lib/SPI.
	- Aufgeräumt: Änderungshistorie dazu. Auskommentierte Header raus.
	- Soft-SPI-Pins nur definiert, wenn USE_LIBRARY_DEFINITIONS_FOR_SOFT_SPI_PINS gesetzt ist
	
*/

#ifdef TALK_SPI_REAL_FUNCTION
  extern uint8_t spiF_talkSpi(uint8_t b) ;
#else
inline unsigned char spiF_talkSpi(unsigned char value) {
  SPDR = value;
  while(!(SPSR & (1<<SPIF)));
  return SPDR;
}
#endif

extern void spiF_initInterface(void);
extern void spiF_8Clocks(unsigned char nClocks);
extern uint8_t spiRxByte;

#define SPI_RESULT_BYTE spiRxByte

/* Die Definitionen für die Soft-SPI-Pins gehören eigentlich in ein projektspezifisches Header-File, z.B. board.h */
#ifdef USE_LIBRARY_DEFINITIONS_FOR_SOFT_SPI_PINS
	  #define SOFTSPI_DELAY /*_delay_us(2);*/
	  #define SOFTSPI_INIT { DDRB |= 0xA0;}
	  //#define SOFTSPI_CS_LO {PORTD &= ~0x04; SOFTSPI_DELAY}
	  //#define SOFTSPI_CS_HI {PORTD |=  0x04; SOFTSPI_DELAY}
	  #define SOFTSPI_MOSI_LO PORTB &= ~0x20;
	  #define SOFTSPI_MOSI_HI PORTB |=  0x20;
	  #define SOFTSPI_CLK_LO PORTB &= ~0x80;
	  #define SOFTSPI_CLK_HI PORTB |=  0x80;
	  #define SOFTSPI_MISO (PINB & 0x40)
#endif
