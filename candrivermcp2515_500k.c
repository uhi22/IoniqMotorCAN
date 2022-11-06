/* 

  CAN Driver for Microchip MCP 2515

 Change history:
 - 2016-11-26 Uwe: CAN-Treiber-Bug: Falls mehrere Botschaften schnell hintereinander aufgegeben werden (z.B. lokal
   die DATETIME und übers UDP-CAN-Gateway noch eine andere), verschluckt der CAN-Treiber die Botschaft.
   Daher als Workaround alle 3 Hardware-Transmit-Buffer des MCP2515 verwenden. Das macht den Datenverlust
   zwar nicht unmöglich (z.B. bei hoher Buslast), aber deutlich unwahrscheinlicher. 

 - 2022-08-30 Uwe:
    - change from 125kBaud to 500kBaud

 ToDo:
   - react on errors (bus off)

*/ 

#include <avr/interrupt.h>  
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "CanDriverMCP2515.h"
#include "Can_Appl.h"
#include "can_def.h"
#include "board.h"
#ifndef CAN_USE_PRIVATE_TALKSPI
 #include "spi.h"
#endif



#ifdef CAN_USE_PRIVATE_TALKSPI
  static inline unsigned char spiF_talkSpi(unsigned char value) {
	SPDR = value;
	while( !(SPSR & (1<<SPIF)) ) ;
	return SPDR;
  } 
#endif


#define CMD_RESET      0xC0
#define CMD_READ       0x03
#define CMD_WRITE      0x02
#define CMD_BIT_MODIFY 0x05
#define CMD_READ_RX0   0x90 /* start at RXB0SIDH */
#define CMD_READ_RX1   0x94 /* start at RXB1SIDH */

#define ADDR_BFPCTRL   0x0C
#define ADDR_TXRTSCTRL 0x0D
#define ADDR_CANSTAT   0x0E
#define ADDR_CANCTRL   0x0F
#define ADDR_TEC       0x1C
#define ADDR_REC       0x1D
#define ADDR_CNF3      0x28
#define ADDR_CNF2      0x29
#define ADDR_CNF1      0x2A

#define ADDR_CANINTE   0x2B
#define ADDR_CANINTF   0x2C
#define ADDR_EFLG      0x2D
#define ADDR_TXB0CTRL  0x30
#define ADDR_TXB0SIDH  0x31
#define ADDR_TXB0SIDL  0x32
#define ADDR_TXB0EID8  0x33
#define ADDR_TXB0EID0  0x34

#define ADDR_TXB0DLC   0x35
#define ADDR_TXB0D0    0x36
#define ADDR_TXB0D1    0x37
#define ADDR_TXB0D2    0x38
#define ADDR_TXB0D3    0x39
#define ADDR_TXB0D4    0x3A
#define ADDR_TXB0D5    0x3B
#define ADDR_TXB0D6    0x3C
#define ADDR_TXB0D7    0x3D


#define ADDR_TXB1CTRL  0x40
#define ADDR_TXB2CTRL  0x50
#define ADDR_RXB0CTRL  0x60
#define ADDR_RXB0SIDH  0x61
#define ADDR_RXB0SIDL  0x62

#define ADDR_RXB0DLC 0x65
#define ADDR_RXB0D0  0x66
#define ADDR_RXB0D1  0x67
#define ADDR_RXB0D2  0x68
#define ADDR_RXB0D3  0x69
#define ADDR_RXB0D4  0x6A
#define ADDR_RXB0D5  0x6B
#define ADDR_RXB0D6  0x6C
#define ADDR_RXB0D7  0x6D


#define ADDR_RXB1CTRL  0x70

/* als Transmitbuffer-Array */
#define ADDR_TXBnCTRL  (0x30+candrv_TxBufferSelector)
#define ADDR_TXBnSIDH  (0x31+candrv_TxBufferSelector)
#define ADDR_TXBnSIDL  (0x32+candrv_TxBufferSelector)
#define ADDR_TXBnEID8  (0x33+candrv_TxBufferSelector)
#define ADDR_TXBnEID0  (0x34+candrv_TxBufferSelector)

#define ADDR_TXBnDLC   (0x35+candrv_TxBufferSelector)
#define ADDR_TXBnD0    (0x36+candrv_TxBufferSelector)
#define ADDR_TXBnD1    (0x37+candrv_TxBufferSelector)
#define ADDR_TXBnD2    (0x38+candrv_TxBufferSelector)
#define ADDR_TXBnD3    (0x39+candrv_TxBufferSelector)
#define ADDR_TXBnD4    (0x3A+candrv_TxBufferSelector)
#define ADDR_TXBnD5    (0x3B+candrv_TxBufferSelector)
#define ADDR_TXBnD6    (0x3C+candrv_TxBufferSelector)
#define ADDR_TXBnD7    (0x3D+candrv_TxBufferSelector)

#define OPMODE_NORMALMODE     0
#define OPMODE_SLEEPMODE      1
#define OPMODE_LOOPBACKMODE   2
#define OPMODE_LISTENONLYMODE 3
#define OPMODE_CONFIGMODE     4



#define CTRL_TXREQ 0x08 /* TXBnCTRL: Transmit Request Bit */


/* interface - variables */
#ifdef CANDRV_DEBUG
uint16_t candrv_debugdata[16];
#endif
uint8_t  candrv_ClkPrescaler = 0;
uint8_t  candrv_OpMode = 0;
uint16_t candrv_id;
uint8_t  candrv_TxBufferSelector = 0;

/******** local functions ************************************/
static inline void candrvLF_selectNextTxBuffer(void) {
  /* Transmit buffer base address is
    TxBufferNr  Address  Selector
      0         0x30      0x00
      1         0x40      0x10
      2         0x50      0x20  */
  candrv_TxBufferSelector+=0x10;
  if (candrv_TxBufferSelector>=0x30) {
    candrv_TxBufferSelector=0;
  }
}

void candrvLF_write(uint8_t address, uint8_t data) {
  #ifdef DEBUG_POINT_4
    DEBUG_POINT_4();
  #endif
  CAN_chipselect_active();
  spiF_talkSpi(CMD_WRITE);
  spiF_talkSpi(address);
  spiF_talkSpi(data);
  CAN_chipselect_passive();
  #ifdef DEBUG_POINT_4
    DEBUG_POINT_4();
  #endif
}

void candrvLF_write8(uint8_t address, uint8_t *data) {
 uint8_t i;
  CAN_chipselect_active();
  spiF_talkSpi(CMD_WRITE);
  spiF_talkSpi(address);
  for (i=0; i<=7; i++) {
    spiF_talkSpi(*data);
	data++;
  }
  CAN_chipselect_passive();
}

static inline uint8_t candrvLF_read(uint8_t address) {
 uint8_t v;
  CAN_chipselect_active();
  spiF_talkSpi(CMD_READ);
  spiF_talkSpi(address);
  v=spiF_talkSpi(0xFF);
  CAN_chipselect_passive();
  return v;
}

void candrvLF_bitmodify(uint8_t address, uint8_t mask, uint8_t data) {
  CAN_chipselect_active();
  spiF_talkSpi(CMD_BIT_MODIFY);
  spiF_talkSpi(address);
  spiF_talkSpi(mask);
  spiF_talkSpi(data);
  CAN_chipselect_passive();
}

void candrvLF_writeCanControl(void) {
  #define ABAT 0 /* don't abort */
  #define OSM 0 /* no one-shot-mode */
  #define CLKEN 1 /* output the clock on CLKOUT */
  candrvLF_write(ADDR_CANCTRL, (candrv_OpMode<<5) | (ABAT<<4) | (OSM<<3)| (CLKEN<<2) | candrv_ClkPrescaler);

}

void candrvLF_changeRX0BF_output_pin(uint8_t value) {
 /*
  B0FE  = 1 
  B0BFM = 0 Mode0=digital output
  B0BFS = 0 or 1 for low or high
 */
 candrvLF_write(ADDR_BFPCTRL, 5 + (value<<4));
}



static inline void candrvLF_readRxBuffer(uint8_t Rx0_or_Rx1) {
 uint16_t id;
 uint8_t i;
 uint8_t *dst;
  CAN_chipselect_active();
  spiF_talkSpi(Rx0_or_Rx1);
  id=spiF_talkSpi(0xFF); /* ID high */
  id<<=8;
  id|=spiF_talkSpi(0xFF); /* ID low */
  id>>=5; /* shift 5 braucht ca. 4µs bei XTAL 8 MHz. Teilen durch 32 ist genau so lang. */
  spiF_talkSpi(0xFF); /* EID8 */
  spiF_talkSpi(0xFF); /* EID0 */
  spiF_talkSpi(0xFF); /* DLC */
  dst = canApplF_provideRxBuffer(id);
  if (dst!=0) {
    for (i=0; i<8; i++) {
	  *dst=spiF_talkSpi(0xFF); /* Data byte */
	  dst++;
	}
	cantracerF_AddToRawBuffer(id, dst-8);
	canApplF_RxIndication(id);
  } else {
    /* no destination, but read from 2515 to reset the receive flag */
    for (i=0; i<8; i++) {
	  spiF_talkSpi(0xFF); /* Data byte */
	}
  }
  CAN_chipselect_passive();
}

/******** interface functions ************************************/

/* Interrupt of the CAN controller */
ISR (INT0_vect)
{
 uint8_t irqSource;
 #ifdef CAN_INTERRUPT_ENTRY
    CAN_INTERRUPT_ENTRY();
 #endif
 #ifdef PROJECT_WEBSERVER
 if ((PINB & 4)==0) {
   /* Ethernet-Chipselect ist aktiv. Wir dürften hier also
      keine SPI-Kommunikation machen, sonst kommt der
	  Ethernet-Controller durcheinander. */
 }
 #endif
 /* we have to look which flag is set and clear it */
 irqSource = candrvLF_read(ADDR_CANINTF);
 if (irqSource & 1) { /* RX0 interrupt flag */
   candrvLF_readRxBuffer(CMD_READ_RX0);
   /* The Read RX Buffer instruction automatically clears the
      interrupt flag. So this is only for debugging here:
	  candrvLF_bitmodify(ADDR_CANINTF, 0x01, 0x00); 
   */
 }
 if (irqSource & 2) { /* RX1 interrupt flag */
   candrvLF_readRxBuffer(CMD_READ_RX1);
   /* The Read RX Buffer instruction automatically clears the
      interrupt flag. So this is only for debugging here:
	  candrvLF_bitmodify(ADDR_CANINTF, 0x02, 0x00); 
   */
 }
 #ifdef CAN_INTERRUPT_EXIT
    CAN_INTERRUPT_EXIT();
 #endif
 /* Laufzeitmessung:
   Bei 8 MHz XTAL und 4 MHz SPI dauert der CAN-RX-Interrupt,
   welcher 17 Bytes brutto überträgt,
    * 85µs Original
	* 63µs mit talkSpi als inline function
	* 61µs mit zusätzlich candrvLF_readRxBuffer als inline function

   Bei 16 MHz XTAL: mit ca. 1 MHz Software-SPI: 180µs.
                    mit 8MHz SPI: - mit externer talkSpi: ca. 32..35µs
					              - mit lokaler talkSpi: genauso 32..35µs.
				   

  */


}

void candrvF_sendMessage(uint16_t id, 
      uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3,
      uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7) {
 #ifdef SHARED_SPI
   /* If we share the SPI with interrupt driven functionality, we
      must ensure that no interrupt with SPI access disturbs our frame --> disable interrupts */
   cli(); /* disable interrupts */
 #else
   /* If the CAN driver is the one and only on the SPI, we have to
      disable CAN interrupt here, because SPI communication in the interrupt would
	  disturb our transmit frame. */
	  DISABLE_CAN_INTERRUPT();
 #endif
 /* setup identifier */
 candrvLF_write(ADDR_TXBnSIDH, id>>3);
 candrvLF_write(ADDR_TXBnSIDL, (id & 7)<<5); /* EXIDE is 0 ==> 11-bit-ID*/
 /* setup DLC */
 candrvLF_write(ADDR_TXBnDLC, 8);
 /* setup data */
 candrvLF_write(ADDR_TXBnD0, d0);
 candrvLF_write(ADDR_TXBnD1, d1);
 candrvLF_write(ADDR_TXBnD2, d2);
 candrvLF_write(ADDR_TXBnD3, d3);
 candrvLF_write(ADDR_TXBnD4, d4);
 candrvLF_write(ADDR_TXBnD5, d5);
 candrvLF_write(ADDR_TXBnD6, d6);
 candrvLF_write(ADDR_TXBnD7, d7);
 /* trigger send */
 candrvLF_write(ADDR_TXBnCTRL, CTRL_TXREQ);
 candrvLF_selectNextTxBuffer();
 #ifdef SHARED_SPI
   sei(); /* enable interrupts */
 #else
   ENABLE_CAN_INTERRUPT();
 #endif
}

void candrvF_sendMessageBuffer(uint16_t id, uint8_t *buffer) {
 #ifdef SHARED_SPI
   /* If we share the SPI with interrupt driven functionality, we
      must ensure that no interrupt with SPI access disturbs our frame --> disable interrupts */
   cli(); /* disable interrupts */
 #else
   /* If the CAN driver is the one and only on the SPI, we have to
      disable CAN interrupt here, because SPI communication in the interrupt would
	  disturb our transmit frame. */
	  DISABLE_CAN_INTERRUPT();
 #endif
 /* setup identifier */
 candrvLF_write(ADDR_TXBnSIDH, id>>3);
 candrvLF_write(ADDR_TXBnSIDL, (id & 7)<<5); /* EXIDE is 0 ==> 11-bit-ID*/
 /* setup DLC */
 candrvLF_write(ADDR_TXBnDLC, 8);
 /* setup data */
 candrvLF_write8(ADDR_TXBnD0, buffer);
 /* trigger send */
 candrvLF_write(ADDR_TXBnCTRL, CTRL_TXREQ);
 candrvLF_selectNextTxBuffer();
 #ifdef SHARED_SPI
   sei(); /* enable interrupts */
 #else
   ENABLE_CAN_INTERRUPT();
 #endif
}


void candrvF_init(void) {

  DDRC  |= 0x01; // Port C0 für Chipselect CAN-Controller

  /* enter configuration mode */
  candrv_OpMode = OPMODE_CONFIGMODE;
  candrvLF_writeCanControl();




  /* set CNF1, CNF2, CNF3, TXRTSCTL, Filter, Mask */
  /* CNF1: Synchronization jump width, baud rate prescaler */
  /*  0x40 ==> Length=2*TQ */
  #if (F_CAN_CONTROLLER==16000000UL)
    /*   Quarz 16MHz/8 ==> 2MHz ==> TQ=500ns */
    /*   TQ = 2 x (BRP + 1) / FOSC */
    //#define BRP 3
	#error "ERROR: CAN Driver: The quartz frequency for CAN controller has to be 8 MHz"
  #elif (F_CAN_CONTROLLER==8000000UL)
    #ifdef CAN_BAUD_125K
      /*   Quarz 8MHz/4 ==> 2MHz ==> TQ=500ns */
      /*   TQ = 2 x (BRP + 1) / FOSC */
      #define BRP 1
    #endif
    #ifdef CAN_BAUD_500K
      /*   Quarz 8MHz/2 ==> 4MHz ==> TQ=250ns */
      /*   TQ = 2 x (BRP + 1) / FOSC */
      #define BRP 0
    #endif

  #else
    #error "ERROR: CAN Driver: The quartz frequency for CAN controller has to be 8 MHz or 16 MHz!"
  #endif

  #ifdef CAN_BAUD_125K
    #define SYNCJUMPWIDTH 0 /* 0 is 1xTQ, which is ok, we have XTAL clock */
    #define BLTMODE 1 /* use CNF3 for PHSEG2 */
    #define SAM 0
    #define PHSEG1 6 /* 6 is 7xTQ */
    #define PRSEG  1 /* 1 is 2xTQ */
    #define PHSEG2 5 /* 5 is 6xTQ */
	/* SyncSeg + PropSeg + PS1 + PS2 = 1 + 2 + 7 + 6 = 16 */
  #endif

  #ifdef CAN_BAUD_500K
    #define SYNCJUMPWIDTH 0 /* 0 is 1xTQ, which is ok, we have XTAL clock */
    #define BLTMODE 1 /* use CNF3 for PHSEG2 */
    #define SAM 0
    #define PHSEG1 4 /* 4 is 5xTQ */
    #define PRSEG  0 /* 0 is 1xTQ */
    #define PHSEG2 0 /* 0 is 1xTQ */
	/* according to http://www.bittiming.can-wiki.info/, the sample point shall be at 87.5% */
	/* SyncSeg + PropSeg + PS1 + PS2 = 1 + 1 + 5 + 1 = 8 */
  #endif


  #define SOF  0 /* 0 dont use SOF output */
  #define WAKFIL 0 /* dont use wakeup filter */

  candrvLF_write(ADDR_CNF1, (SYNCJUMPWIDTH<<6) | BRP); 
  candrvLF_write(ADDR_CNF2, (BLTMODE<<7) | (SAM<<6) | (PHSEG1<<3) | PRSEG); 
  candrvLF_write(ADDR_CNF3, (SOF<<7) | (WAKFIL<<6) | PHSEG2); 

  /* configure RX buffers */
  /* todo: RXB0CTRL for rollover to RX buffer 1, when RXB0 is full */

  /* configure the MCP2515 interrupt sources */
  candrvLF_write(ADDR_CANINTE, 0x03); /* for 1st try, only rx0 and rx1 are used */

  /* configure the ATMEGA to accept the interrupt */
  /* MCUCR: ISC01, ISC00 = 0 ==> low level on INT0 generated interrupt */
  MCUCR &= 0xFC;
  ENABLE_CAN_INTERRUPT();

  /* enter normal mode */
  candrv_OpMode = OPMODE_NORMALMODE;
  candrvLF_writeCanControl();

  #ifdef CANDRV_CLOCKOUT_CHECK
  candrv_ClkPrescaler=(candrv_ClkPrescaler+1) & 0x03;
  candrvLF_writeCanControl();

  _delay_ms(50);

  candrv_ClkPrescaler=(candrv_ClkPrescaler+1) & 0x03;
  candrvLF_writeCanControl();

  _delay_ms(500);

  candrv_ClkPrescaler=(candrv_ClkPrescaler+1) & 0x03;
  candrvLF_writeCanControl();

  _delay_ms(500);

  candrv_ClkPrescaler=(candrv_ClkPrescaler+1) & 0x03;
  candrvLF_writeCanControl();

  _delay_ms(500);

  candrv_ClkPrescaler=(candrv_ClkPrescaler+1) & 0x03;
  candrvLF_writeCanControl();
  #endif

}
  
