

/* CAN receiver with 500kBaud, output on serial with 9600k */
/* detail see readme.md */
 
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h> /* for dtostrf */
#include <stdint.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <string.h>
#include <avr/interrupt.h>

#include "board.h"
#include "spi.h"
#include "console.h"

#include "CanDriverMCP2515.h"
#include "can_def.h"
#include "can_appl.h"
#include "globals.h"
#include "TimeCalculations.h"
#include <util/crc16.h>


/*-------------------------------------------------------*/


#define ENABLE_INTERRUPTS() sei()
#define DISABLE_INTERRUPTS() cli()

#define TCNT1_STEP_TIME_us 2



/**********************************************************
 Prototypen 
*/


/**********************************************************
 Variablen 
*/
uint8_t  isrToggle;
volatile uint8_t isrFlag5ms;
volatile uint8_t isrFlagAdc;

volatile uint16_t isrT1Overflows;

volatile uint32_t isrMikrosekunden;

volatile uint16_t isrSekundenAkku1;
volatile uint16_t isrFreeRunning;
volatile uint8_t  isrSekDivider;
volatile uint8_t  isrFlagMinute;
uint16_t isrTimer1OverflowCounter;

uint16_t tmpSekundenAkku1;
uint16_t timerLast250ms=0;
uint32_t uptime_s;

uint32_t isrtmpu32;

uint16_t mycounter16;
uint8_t tmpu8;
uint16_t tmpu16;
uint32_t tmpu32;
char strtmp[22];

uint8_t divider1s;
uint8_t displayIndex;

uint8_t trigger, nCycles;
uint8_t blIsStartedUp = 0;
uint8_t blTriggerEepromWrite = 0;


#define EEPROM_SIZE 16 /* used size in bytes */

union {
  uint8_t bytes[EEPROM_SIZE];
  struct {
    uint16_t marker;
    uint16_t writeCount;
    int32_t EDrive;
    int32_t ECharge;
    uint16_t marker2;
	uint16_t aux;
  } fields;
} safestruct;

uint8_t eePlaceholder[EEPROM_SIZE] EEMEM; /* to have an symbolic name for the eeprom */
#define EEPROM_ADDRESS (eePlaceholder+0x10) /* try another address */
#define EE_MARKER 0x55CC

/***********************************************************/
/* For Ri calculation */
#define RI_LIST_TRANSMISSION_TIME_S 30 /* after we have an Ri measured, transmit the Ri list for some time. */
#define RI_NUMBER_OF_RESULTS 6 /* for a typical test sequence of 3 pulses, we have 6 edges and want to know their results. All are packed into one list for logging. */
#define RI_N_QUEUE_LEN 7
int16_t ri_voltages[RI_N_QUEUE_LEN];
int16_t ri_currents[RI_N_QUEUE_LEN];
uint8_t ri_writeIndex = 0; /* at this index we will write. It means, it points to the oldest element. */
uint8_t ri_inhibitTimer;
uint8_t blIsNewSampleAvailable; /* flag set by the interrupt and consumed by the task */
int32_t tmpI, tmpU, ri_0mOhm1;
uint8_t ri_resultCounter;
int16_t ri_results_0mOhm1[RI_NUMBER_OF_RESULTS]; /* list of the calculation results */
char strRiList[100];
uint8_t ri_transmitEnableCounter_s;


/* adding a sample (voltage, current) to the queue. This function is called by the CAN receive interrupt. */
void ri_addSampleToQueue(void) {
  ri_voltages[ri_writeIndex]=message_595.s.UBatt_0V1;
  ri_currents[ri_writeIndex]=message_595.s.IBatt_0A1;
  ri_writeIndex++;
  if (ri_writeIndex>=RI_N_QUEUE_LEN) {
    ri_writeIndex=0;
  }
  blIsNewSampleAvailable = 1;
}


void ri_addResultToResultList(void) {
 uint8_t i;
 //if (ri_mOhm<10) return; /* implausible range */
 //if (ri_mOhm>500) return; /* implausible range */
 ri_resultCounter++; /* count the result, independent of the list handling, just for statistics */
 for (i=RI_NUMBER_OF_RESULTS-1; i>=1; i--) {
   ri_results_0mOhm1[i] = ri_results_0mOhm1[i-1]; /* shift the array one to the right, so that we have space to add the latest value in element 0. */
 }
 ri_results_0mOhm1[0] = ri_0mOhm1;
}

/* Example result string
"RIL 4.470.485.500.490.0.0."
means:
 RIL: identifier for "Resistance Internal List"
 4: Total number of results since startup
 470: Ri is 47.0 mOhms (newest calculated value)
 ..
 490: Ri is 49.0 mOhms (oldest calculated value)
 0.0.: there are two fields still empty
*/
void ri_createResultString(void) {
 uint8_t i;
  itoa(ri_resultCounter, strtmp, 10);
  sprintf(strRiList, "RIL %s.", strtmp);
  for (i=0; i<RI_NUMBER_OF_RESULTS; i++) {
    itoa(ri_results_0mOhm1[i], strtmp, 10);
	strcat(strRiList, strtmp);
	if (i<RI_NUMBER_OF_RESULTS-1) strcat(strRiList, ".");
  }

}

uint8_t inrange(int16_t x, int16_t min, int16_t max) {
 return ((min<=x) && (x<=max));
}

/* retrieve the elements from the queue */
/* element 0 means "oldest".
   element max-1 means "newest".

   examples
   writeIdx  element   effectiveIndex
      0         0          0           oldest is the most left element
	  6         0          6           oldest is the most right element
	  6         6         12-7=5       we want the newest. This is at 5.
   
*/
int16_t getCurrent(uint8_t element) {
 uint8_t i;
 i = ri_writeIndex; /* points to the oldest element */
 i += element;
 if (i>=RI_N_QUEUE_LEN) i-=RI_N_QUEUE_LEN;
 return ri_currents[i];
}

int16_t getVoltage(uint8_t element) {
 uint8_t i;
 i = ri_writeIndex; /* points to the oldest element */
 i += element;
 if (i>=RI_N_QUEUE_LEN) i-=RI_N_QUEUE_LEN;
 return ri_voltages[i];
}

int16_t getPower_kW(uint8_t element) {
 uint16_t u,i;
 int32_t p;
  i = getCurrent(element); /* in 0.1A */
  u = getVoltage(element); /* in 0.1V */
  p= u*i;
  p/=100;
  p/=1000; /* Watt to kW */
  return p; /* in kW */
}

#define LOW_CURRENT 20*10 /* 20A in LSB=0.1A */

/* for the high-current, the range 210A to 320A works fine for a warm (>10°C) battery. But e.g. for 5°C and 10% SOC we have power limitation,
so the 210A are not reached.
For e.g. 50kW and 330V we have 151A.
For e.g. 60kW and 330V we have 181A.
*/
#define HIGH_CURRENT_MIN 150*10 /* 150A */
#define HIGH_CURRENT_MAX 320*10 /* 320A */
#define POWER_TOLERANCE 10 /* kW tolerated deviation between the two high-power samples */


/* based on the queue content, calculate the internal resistance of the battery. */
/* This function is called on task level. 
   Data consistency note: Since the queue is filled on interrupt level, there is the risk of data corruption while the
   function is calculating. We prevent this, by calling the function at higher rate (e.g. 10ms to 50ms) than the CAN message rate (100ms).
   So the function has the chance to process the data after complete reception of one message and sure before the reception of the next message. */
void calculateRi(void) {
  uint8_t blCalculationTrigger;
  int16_t pCompare_kW;

  if (!blIsNewSampleAvailable) return; /* directly leave, if there is nothing to do. */
  blIsNewSampleAvailable=0; /* consume the data */
  /* we have new data in the queue. We will come here after each received message, means 100ms cycle. To reach this goal on task level, 
  we must call this function more often, to not miss any sample. */
  if (ri_inhibitTimer>0) {
    ri_inhibitTimer--; /* we just calculated an Ri value based on the actual queue. Wait until sufficiently new values are collected. */
	return;
  }

  blCalculationTrigger = 0;
  /* for the rising edge of the current */
  if (inrange(getCurrent(0), -LOW_CURRENT, LOW_CURRENT)) { /* first and ... */
      if (inrange(getCurrent(1), -LOW_CURRENT, LOW_CURRENT)) { /* second sample must be low current. */
        if (inrange(getCurrent(RI_N_QUEUE_LEN-1), HIGH_CURRENT_MIN, HIGH_CURRENT_MAX)) {
          if (inrange(getCurrent(RI_N_QUEUE_LEN-2), HIGH_CURRENT_MIN, HIGH_CURRENT_MAX)) { /* last two samples must be high current */
		    pCompare_kW = getPower_kW(RI_N_QUEUE_LEN-1);
		    if (inrange(getPower_kW(RI_N_QUEUE_LEN-2), pCompare_kW - POWER_TOLERANCE, pCompare_kW + POWER_TOLERANCE)) { /* both high-power-samples must be similar */
			  blCalculationTrigger=1;
            }
		  }
	    }
	  }
  }

  /* for the falling edge of the current */
  if (inrange(getCurrent(0), HIGH_CURRENT_MIN, HIGH_CURRENT_MAX)) { /* first and ... */
      if (inrange(getCurrent(1), HIGH_CURRENT_MIN, HIGH_CURRENT_MAX)) { /* second sample must be high current. */
        if (inrange(getCurrent(RI_N_QUEUE_LEN-1), -LOW_CURRENT, LOW_CURRENT)) {
          if (inrange(getCurrent(RI_N_QUEUE_LEN-2), -LOW_CURRENT, LOW_CURRENT)) { /* last two samples must be low current */
		    pCompare_kW = getPower_kW(0);
		    if (inrange(getPower_kW(1), pCompare_kW - POWER_TOLERANCE, pCompare_kW + POWER_TOLERANCE)) { /* both high-power-samples must be similar */
			  blCalculationTrigger=1;
            }
		  }
	    }
	  }
  }
		    
  if (!blCalculationTrigger) {
    /* the input data is not suitable to calculate an Ri */
    return;
  }

  /* data is plausible. Now lets calculate the Ri. */

  /* averaging over two oldest samples and over two newest samples, and calculating the difference between new and old */
  tmpI =  getCurrent(RI_N_QUEUE_LEN-1);
  tmpI += getCurrent(RI_N_QUEUE_LEN-2);
  tmpI -= getCurrent(0);
  tmpI -= getCurrent(1);
  tmpU =  getVoltage(RI_N_QUEUE_LEN-1);
  tmpU += getVoltage(RI_N_QUEUE_LEN-2);
  tmpU -= getVoltage(0);
  tmpU -= getVoltage(1);
  /* Resolutions:
     - On CAN and in the queue, the current is in 0.1A and the voltage is in 0.1V.
	 - By averaging without division, the tmpI has 0.05A and tmpU has 0.05V.
	 - Dividing R = U / I directly leads to ohms, because 1 ohm = 1V/1A = 0.05V/0.05A.
	 - We want tenth of milli-ohms, so we multiply with 10000.
  */
  ri_0mOhm1 = 10000;
  ri_0mOhm1*=tmpU;
  ri_0mOhm1/=tmpI;
  ri_0mOhm1 = -ri_0mOhm1; /* discharge current, which leads to voltage drop, is negative. That's why we invert the sign here, to get a positive Ri. */
  ri_addResultToResultList();
  ri_inhibitTimer = 5; /* inhibit further evaluations of the same edge (within 500ms) */
  ri_transmitEnableCounter_s = RI_LIST_TRANSMISSION_TIME_S; /* The new result list shall be sent for 60s, to have the chance that it is logged to web space. */

}

/**********************************************************
 Funktionen 
*/
#define sleep() asm volatile ("sleep")

char x;

// Interruptroutine für Timer1 overflow
// 
ISR (TIMER1_OVF_vect)
{

}

// Interruptroutine für Timer1 output compare
// 
ISR (TIMER1_COMPA_vect)
{
  /* Bei 8MHz XTAL:
     Dieser Zweig läuft alle 50µs.
  */
  isrTimer1OverflowCounter++;
  isrMikrosekunden+=50U;
  if (isrMikrosekunden>=5000U) {
    isrMikrosekunden-=5000U;
    isrFlag5ms=1;
  }
  isrToggle++;
  if (isrToggle & 1) {
  	/* Dieser Zweig läuft alle 200µs = 5kHz */
  	isrFlagAdc=1;
  }
}


/****************************************************************/



ISR (INT1_vect)
{
}


/*******************************************************************/

void countTimeouts(void) {
  if (timeoutcounter_WHLSPD11) timeoutcounter_WHLSPD11--;
  if (timeoutcounter_542) timeoutcounter_542--;
  if (timeoutcounter_595) timeoutcounter_595--;
  if (timeoutcounter_5D0) timeoutcounter_5D0--;
  if (timeoutcounter_BMS2) timeoutcounter_BMS2--;
}

/*******************************************************************
 *
 * 
 *
 *******************************************************************/
void readEeprom(void) {
  eeprom_read_block(safestruct.bytes, EEPROM_ADDRESS, EEPROM_SIZE);
  /*
  Serial.print("EE marker ");
  Serial.println(safestruct.fields.marker);
  Serial.print("writeCount ");
  Serial.println(safestruct.fields.writeCount);
  Serial.print("startupCount ");
  Serial.println(safestruct.fields.startupCount);
  Serial.print("EDrive ");
  Serial.println(safestruct.fields.EDrive);
  Serial.print("ECharge ");
  Serial.println(safestruct.fields.ECharge);
  */
  printConsoleString_p(PSTR("WRC "));
  printConsoleDecimalU16(safestruct.fields.writeCount);
  printConsoleString_p(PSTR("\x0D\x0A"));
}

void checkEepromConsistency(void) {
  if ((safestruct.fields.marker != EE_MARKER) || (safestruct.fields.marker2 != EE_MARKER)) {
    //Serial.println("EE is invalid. Initializing.");
	printConsoleString_p(PSTR("EE is invalid.\x0D\x0A"));
    safestruct.fields.marker = EE_MARKER;
    safestruct.fields.writeCount=0;
    safestruct.fields.EDrive = 0;
    safestruct.fields.ECharge = 0;
	safestruct.fields.marker2 = EE_MARKER;
	safestruct.fields.aux = 0xdead;
    blTriggerEepromWrite = 1;
  }  
}

void transferEepromToApplication(void) {
  PIntegralDrive_Wh = safestruct.fields.EDrive;
  PIntegralCharge_Wh = safestruct.fields.ECharge;
}

void writeEeprom(void) {
  /* Attention: this may take some milliseconds */
  eeprom_write_block(safestruct.bytes, EEPROM_ADDRESS, EEPROM_SIZE);
  //Serial.println("EE persisted");
  printConsoleString_p(PSTR("persisted\x0D\x0A"));
}

void findFreeLogPlaceAndLog(void) {
 /* For debugging, add the EE data into a list, as long as we find free places. */
 uint16_t offset; /* The address in the EEPROM. */
 uint16_t tempU16;
 for (offset=0x20; offset<0x3E0; offset+=0x10) {
 	eeprom_read_block(&tempU16, eePlaceholder+offset, 2);
	if (tempU16!=EE_MARKER) {
		/* We found an unused entry. */
		/* Write the data into this 16-byte-entry: */
  		/* Attention: this may take some milliseconds */
  		eeprom_write_block(safestruct.bytes, eePlaceholder+offset, EEPROM_SIZE);
		return; /* stop the search loop */
	}
 }
}

void careForEepromWrite(void) {
  if (blIsStartedUp) {
      blIsStartedUp = 0;
      //safestruct.fields.startupCount++;
	  /* we could trigger the writing directly in case of startup. But this has the risk,
	  that due to a power-bounce we lose the power during startup, and destroy the
	  EEPROM content. That's why we do not set blTriggerEepromWrite = 1 here. Instead,
	  the write happens only in case of timeout of the BMS message, where we still have
	  sufficient time until the power drops. (The delay between CAN sleep and power down is
	  controlled by hardware circuit). */
  }

  if (timeoutcounter_595==0) {
    /* The BMS message stopped. This is a good point in time to store the counters non-volatile. The power will drop-off in some seconds. */
    if ((safestruct.fields.EDrive != PIntegralDrive_Wh) ||
         (safestruct.fields.ECharge != PIntegralCharge_Wh)) {
      safestruct.fields.EDrive = PIntegralDrive_Wh;
      safestruct.fields.ECharge = PIntegralCharge_Wh;
      blTriggerEepromWrite = 1;
    }
  }
  
  if (blTriggerEepromWrite) {
    safestruct.fields.writeCount++;
    findFreeLogPlaceAndLog();
    writeEeprom();
    blTriggerEepromWrite = 0;
  }
}

/*******************************************************************/

void printTempStringAndNewLine(void) {
  printConsoleString(strtmp); printConsoleString_p(PSTR("\x0D\x0A"));
}


void printForOLED_part1(void) {
  calculateRi(); /* the console printing is blocking a certain time, and in between we must give the calculation a chance. */
  printConsoleString_p(PSTR("WRC "));
  itoa(safestruct.fields.writeCount, strtmp, 10);
  printTempStringAndNewLine();

  printConsoleString_p(PSTR("UPT "));
  ltoa(uptime_s, strtmp, 10);
  printTempStringAndNewLine();

  calculateRi(); /* the console printing is blocking a certain time, and in between we must give the calculation a chance. */
  printConsoleString_p(PSTR("UBT "));
  if (timeoutcounter_595) {
	dtostrf( ((float)message_595.s.UBatt_0V1)/10, 5, 1, strtmp);
  } else {
    sprintf(strtmp, "na");
  }
  printTempStringAndNewLine();

  printConsoleString_p(PSTR("IBT "));
  if (timeoutcounter_595) {
	dtostrf( ((float)message_595.s.IBatt_0A1)/10, 5, 1, strtmp);
  } else {
    sprintf(strtmp, "na");
  }
  printTempStringAndNewLine();
}

void printForOLED_part2(void) {
  calculateRi(); /* the console printing is blocking a certain time, and in between we must give the calculation a chance. */

  printConsoleString_p(PSTR("SPD "));
  if (timeoutcounter_WHLSPD11) {
	dtostrf( ((float)wheelspeed_FL)*0.03125, 5, 1, strtmp);
  } else {
    sprintf(strtmp, "na");
  }
  printTempStringAndNewLine();

  printConsoleString_p(PSTR("ODO "));
  if (timeoutcounter_5D0) {
	dtostrf( ((float)ODO_0km1)/10, 5, 1, strtmp);
  } else {
    sprintf(strtmp, "na");
  }
  printTempStringAndNewLine();

  calculateRi(); /* the console printing is blocking a certain time, and in between we must give the calculation a chance. */
  printConsoleString_p(PSTR("P "));
  if (timeoutcounter_595) {
	dtostrf( ((float)P_0W01)/100/1000, 5, 2, strtmp);
  } else {
    sprintf(strtmp, "na");
  }	  
  printTempStringAndNewLine();

  printConsoleString_p(PSTR("SOC "));
  if (timeoutcounter_542) {
	dtostrf( ((float)message_542.s.SOC_0prz5)/2, 5, 1, strtmp);
  } else {
    sprintf(strtmp, "na");
  }
  printTempStringAndNewLine();
}

void printForOLED_part3(void) {
  calculateRi(); /* the console printing is blocking a certain time, and in between we must give the calculation a chance. */
  printConsoleString_p(PSTR("ECH "));
  sprintf(strtmp, "%ld", PIntegralCharge_Wh);
  printTempStringAndNewLine();

  printConsoleString_p(PSTR("EDR "));
  sprintf(strtmp, "%ld", PIntegralDrive_Wh);
  printTempStringAndNewLine();

  calculateRi(); /* the console printing is blocking a certain time, and in between we must give the calculation a chance. */
  printConsoleString_p(PSTR("TMX "));
  if (timeoutcounter_BMS2) {
    sprintf(strtmp, "%d", message_BMS2.s.TBatMax_C);
  } else {
    sprintf(strtmp, "na");
  }	
  printTempStringAndNewLine();
  if (ri_transmitEnableCounter_s) {
      ri_createResultString();
      printConsoleString(strRiList);
      printConsoleString_p(PSTR("\x0D\x0A"));
  } else {
    printConsoleString_p(PSTR("\x0D\x0A"));
  }
}

void printForOLED(void) {
 switch (displayIndex) {
   case 0:
    printForOLED_part1();
    printForOLED_part2();
    printForOLED_part3();
    break;
   case 1:
    printForOLED_part3();
    printForOLED_part1();
    printForOLED_part2();
    break;
   case 2:
    printForOLED_part2();
    printForOLED_part3();
    printForOLED_part1();
    break;
 }

 displayIndex++;
 if (displayIndex>=3) displayIndex=0;
}



/*******************************************************************/
void BackgroundTask(void) {
 //adcCyclic();
 calculateRi();
}

/*******************************************************************
 *
 * MAIN
 *
 *******************************************************************/

int main (void) {
	// Initialisierungen
	// (0) Takt festlegen: Prescaler auf 1 stellen, so dass 8 MHz Systemtakt
	//CLKPR = (1<<CLKPCE); // enable prescaler change
	//CLKPR = 0; // change prescaler

	// (a) globale Variablen
	mycounter16 = 0;


	// (b) I/O
	//DDRD  |= 0x02; // Port D1 für TXD
	DDRD  |= 0x80; // Port D7 für LED

	initConsole();
	spiF_initInterface();
	candrvF_init();

    /* Interrupt enable */
	//GICR |= (1<<INT1);  /* GICR: Enable  INT1 */

	
	//
	// Timer 0, zählt 0..OCR0A-1
	// Clock Source ist Pin T0, rising edge --> CS=0b111
	// Normal Mode: WGM=00 --> overrunning counter 0..255	
	//TCCR0 = (0<<WGM00) | (0<<WGM01) | (1<<CS02) | (1<<CS01) | (1<<CS00);
	//TIMSK |= (1<<TOIE0); //timer 0 overflow interrupt enable


	//
	// Timer 1
	// für Scheduler
	// 
	//
	// Clock Source
	// 001 is Prescaler 1
	// 010 is Prescaler 8   --> 8MHz/8  = 1 MHz, also 1µs.
	// 011 is Prescaler 64 
	// 100 is Prescaler 256
	TCCR1B = (0<<CS12) | (1<<CS11) | (0<<CS10);

	// Top of Counter.
	// f_oc = f_xtal / (prescaler*(1+OCR1A))
	//  with OCR1A=199:  f= 8MHz/(8*200) =  5 KHz
	//  with OCR1A= 99:  f= 8MHz/(8*100) = 10 KHz
	//  with OCR1A= 49:  f= 8MHz/(8* 50) = 20 KHz
	OCR1A=49;

	// Clear Timer On Compare: WGM12=1, WGM11=0, WGM10=0
	TCCR1B |= (1<<WGM12);


	// Timer Interrupt Mask	
	TIMSK |= (1<<OCIE1A); //enable output compare interrupt
	//TIMSK |= (1<<TOIE1); //enable timer 1 overflow interrupt

    readEeprom();
    checkEepromConsistency();
    transferEepromToApplication();
    blIsStartedUp=1;

	LED_an();
	_delay_ms(100);
	LED_aus();
	_delay_ms(100);
	//LED_an();
	//_delay_ms(300);
	//LED_aus();
	//_delay_ms(100);
	LED_an();
	_delay_ms(100);
	LED_aus();
	// Interrupts enable
	sei();
	while(1) {

		if (isrFlag5ms) {
			isrFlag5ms=0;
			
			//candrvF_cyclic();
			divider1s++;
			if (divider1s>=200) {
			  /* Timing remark: The timing is NOT exact in this project, because during transmission of the serial data,
			     we do not evaluate the isrFlag5ms, and not count the divider1s. This means, a certain number of
				 5ms ticks will be missed in the one-second-calculation, and the calculated 1s will be slower than the
				 real one second. In this project, this behavior does not matter, because the power-integral will be done
				 anyway based on the 100ms CAN message cycle. But if the one-second calculation will be used for other
				 precice things, we need to make an improvement here! */
			  /* once per second */

			  divider1s=0;
			  uptime_s++;
			  safestruct.fields.aux=uptime_s;
			  if (ri_transmitEnableCounter_s>0) ri_transmitEnableCounter_s--;
			  if ((uptime_s % 10)==0) {
			    /* every 10s */
				//integration test
			  	//ri_mOhm=uptime_s/10;
			    //ri_addResultToResultList();
				/* For testing, simulate updated energy: */
				//PIntegralCharge_Wh +=0x1100;
				//LED_an();
				//_delay_ms(50);
				//LED_aus();
			  }
			  //candrvF_sendMessageBuffer(MESSAGE_ID_TEST, message_TEST.c.c);
			}
			if ((divider1s % 100)==0) {
			  /* twice per second */
			  printForOLED();
              careForEepromWrite();
			}
			if ((divider1s % 50)==0) {
			  /* four times per second */
			  countTimeouts();
			  ODO_0km1 = message_5D0.s.ODO_H;
			  ODO_0km1 <<= 8;
			  ODO_0km1 += message_5D0.s.ODO_M;
			  ODO_0km1 <<= 8;
			  ODO_0km1 += message_5D0.s.ODO_L;
			}
		}
		BackgroundTask();

	}
 
	/* wird nie erreicht */
	return 0;
}
