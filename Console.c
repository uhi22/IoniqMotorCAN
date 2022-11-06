/* 

 Console:
  - Debug-Ausgaben über Hardware-UART

*/ 

 
#include <avr/io.h>
#include <util/delay.h>
//#include <stdio.h>
//#include <stdint.h> 
#include <avr/pgmspace.h>
#include <string.h>

#define MIT_UART_DEBUG


#define baudrate 9600 /* 38400 ist der größte sinnvolle Wert bei 8MHz clock, wegen Teiler=13 */

void USART_Init(void)
{
#define UBRR (F_CPU / (baudrate * 16L) - 1)
/* Set baud rate */
UBRRH = (unsigned char)(UBRR>>8);
UBRRL = (unsigned char)UBRR;
/* Enable receiver and transmitter */
UCSRB = (1<<RXEN)|(1<<TXEN);
/* Set frame format: 8data, 2stop bit */
UCSRC = (1<<URSEL)|(1<<USBS)|(3<<UCSZ0);
}


void printConsoleChar( unsigned char data )
{
  /* Wait for empty transmit buffer */
  while ( !( UCSRA & (1<<UDRE)) );
  /* Put data into buffer, sends the data */
  UDR = data;
}


//
// String über UART senden. Quelle ist PROGMEM (Flash).
//
void printConsoleString_p(const char *stringImFlash) {
#ifdef MIT_UART_DEBUG
 char Zeichen;
 while ((Zeichen = pgm_read_byte(stringImFlash)))
 {
   printConsoleChar(Zeichen);
   stringImFlash++;
 }
 #endif
}


//
// String über UART senden. Quelle ist RAM.
//
void printConsoleString(char* s) {
#ifdef MIT_UART_DEBUG
 uint8_t i;
 i=0;
 while (i<100) { // Rettungsanker gegen Endlosschleife
   if (s[i]==0) return; // String-Ende
   printConsoleChar(s[i]);
   i++;
 }
#endif
}
 
 
//
// 16-Bit-Wert als Dezimalzahl (5-stellig) über UART senden
//
void printConsoleDecimalU16( uint16_t value )
{
#ifdef MIT_UART_DEBUG
    char s[6];
	s[0]='0'+(value / 10000);
	value %=10000;
	s[1]='0'+(value / 1000);
	value %=1000;
	s[2]='0'+(value / 100);
	value %=100;
	s[3]='0'+(value / 10);
	value %=10;
	s[4]='0'+(value);
    s[5]=0;
    printConsoleString( s );
#endif
}


//
// 32-Bit-Wert als Dezimalzahl (10-stellig) über UART senden
//
void printConsoleDecimalU32( uint32_t value )
{
#ifdef MIT_UART_DEBUG
    char s[11];
	s[0]='0'+(value / 1000000000);
	value %=1000000000;

	s[1]='0'+(value / 100000000);
	value %=100000000;

	s[2]='0'+(value / 10000000);
	value %=10000000;

	s[3]='0'+(value / 1000000);
	value %=1000000;

	s[4]='0'+(value / 100000);
	value %=100000;

	s[5]='0'+(value / 10000);
	value %=10000;

	s[6]='0'+(value / 1000);
	value %=1000;

	s[7]='0'+(value / 100);
	value %=100;

	s[8]='0'+(value / 10);
	value %=10;

	s[9]='0'+(value);
    s[10]=0;
    printConsoleString( s );
#endif
}



//
// 16-Bit-Wert als Dezimalzahl (5-stellig plus Vorzeichen) über UART senden
//
void printConsoleDecimalS16( int16_t value )
{
#ifdef MIT_UART_DEBUG
    char s[7];
	if (value<0) { s[0]='-'; value=-value; } else { s[0]='+'; }
	s[1]='0'+(value / 10000);
	value %=10000;
	s[2]='0'+(value / 1000);
	value %=1000;
	s[3]='0'+(value / 100);
	value %=100;
	s[4]='0'+(value / 10);
	value %=10;
	s[5]='0'+(value);
    s[6]=0;
    printConsoleString( s );
#endif
}



//
// 8-Bit-Wert als Dezimalzahl (3-stellig) über UART senden
//
void printConsoleDecimalU8( uint8_t value )
{
#ifdef MIT_UART_DEBUG
    char s[4];
	s[0]='0'+(value / 100);
	value %=100;
	s[1]='0'+(value / 10);
	value %=10;
	s[2]='0'+(value);
    s[3]=0;
    printConsoleString( s );
#endif	
}

//
// 8-Bit-Wert als Hexadezimalzahl (2-stellig) über UART senden
//
void printConsoleHexU8( uint8_t value )
{
#ifdef MIT_UART_DEBUG
    char s[3];
	char a;
	a=value>>4;
	if (a<=9) {
      a+='0';
	} else {
      a+='A'-10;
    }  
	s[0]=a;
	a=value & 0x0f;
	if (a<=9) {
      a+='0';
	} else {
      a+='A'-10;
    }  
	s[1]=a;
    s[2]=0;
    printConsoleString( s );
#endif	
}

//
// 16-Bit-Wert als Hexadezimalzahl (2-stellig) über UART senden
//
void printConsoleHexU16( uint16_t value ) {
  printConsoleHexU8(value>>8);
  printConsoleHexU8((uint8_t)value);
}

 
void initConsole (void) {
  USART_Init();
    
  // Startmeldung ausgeben
  printConsoleString_p(PSTR("\x0D\x0AinitConsole\x0D\x0A"));

}
