
#define F_CAN_CONTROLLER 8000000 /* 8 MHz XTAL at the CAN controller */
#define CAN_BAUD_500K /* CAN has 500kBaud */

#define LED_aus()  PORTD &= ~0x80;
#define LED_an()   PORTD |=  0x80;

#define DEBUG_SET_PA0(x) if (x) {PORTA |= 1; } else { PORTA &= ~1; }
#define DEBUG_SET_PA1(x) if (x) {PORTA |= 2; } else { PORTA &= ~2; }

#define ENABLE_CAN_INTERRUPT()  GICR |= (1<<INT0)  /* GICR: Enable  INT0 */
#define DISABLE_CAN_INTERRUPT() GICR &= ~(1<<INT0) /* GICR: Disable INT0 */
  
#define CAN_chipselect_active()  PORTC &= ~0x01;
#define CAN_chipselect_passive() PORTC |=  0x01;

#define cantracerF_AddToRawBuffer(a,b) /* empty macro, no can tracer used */

#define DIAGNOSTIC_NODE_ID DIAGNOSTIC_NODE_ID_MULTIMETER

#define OUTPUTCHANNEL_EXT_MIN ((DIAGNOSTIC_NODE_ID<<8) + 0) /* */
#define OUTPUTCHANNEL_EXT_MAX ((DIAGNOSTIC_NODE_ID<<8) + 15)
#define NUMBER_OF_OUTPUTCHANNELS_EXT (1+ OUTPUTCHANNEL_EXT_MAX - OUTPUTCHANNEL_EXT_MIN)

#define CAN_USE_PRIVATE_TALKSPI
#define HARDWARE_SPI
/* SPI-Pins für ATMega32 */
#define SPI_MOSI         PINB5
#define SPI_SCK          PINB7
#define SPI_SLAVESELECT1 PINB4
#define SPI_DDR          DDRB
#define SPI_PORT         PORTB



