/* 

  Application specific part of CAN Driver

*/ 

//#include <avr/interrupt.h>  
//#include <avr/io.h>
#include <avr/pgmspace.h>
//#include <util/delay.h>
//#include "spi.h"
#include "board.h"
#include "CanDriverMCP2515.h"
#include "can_def.h" /* message definitions */
#include "globals.h"


/******** interface - variables ********************************/

t_union_WHLSPD11      message_WHLSPD11;
t_union_542      message_542;
t_union_595      message_595;
t_union_5D0      message_5D0;
t_union_BMS2          message_BMS2;

uint8_t timeoutcounter_WHLSPD11;
uint8_t timeoutcounter_542;
uint8_t timeoutcounter_595;
uint8_t timeoutcounter_5D0;
uint8_t timeoutcounter_595;
uint8_t timeoutcounter_BMS2;

int32_t P_0W01;
int32_t PIntegralDrive_hiRes;
int32_t PIntegralDrive_Wh;
int32_t PIntegralCharge_hiRes;
int32_t PIntegralCharge_Wh;
uint16_t wheelspeed_FL;
uint32_t ODO_0km1;


/******** local variables *************************************/

uint8_t canAppl_divider10ms_to_1s;

/******** local functions *************************************/


/******** interface functions *********************************/
extern void ri_addSampleToQueue(void);

uint8_t* canApplF_provideRxBuffer(uint16_t RxMessageId) {
 uint8_t* dst;
  dst=0;
  if (RxMessageId==(MESSAGE_ID_WHLSPD11)) {
    dst=message_WHLSPD11.c.c;
	timeoutcounter_WHLSPD11=6; /* 6*250ms = 1.5s */
  } else
  if (RxMessageId==(MESSAGE_ID_542)) {
    dst=message_542.c.c;
	timeoutcounter_542=6; /* 6*250ms = 1.5s */
  } else
  if (RxMessageId==(MESSAGE_ID_595)) {
    dst=message_595.c.c;
	timeoutcounter_595=6; /* 6*250ms = 1.5s */
  } else
  if (RxMessageId==(MESSAGE_ID_5D0)) {
    dst=message_5D0.c.c;
	timeoutcounter_5D0=6; /* 6*250ms = 1.5s */
  } else
  if (RxMessageId==(MESSAGE_ID_BMS2)) {
    dst=message_BMS2.c.c;
	timeoutcounter_BMS2=6; /* 6*250ms = 1.5s */
  }   
  return dst;
}

#define PINTEGRAL_HIRES_SCALER (100*10*(int32_t)3600) /* Scale to Wh */

void canApplF_RxIndication(uint16_t RxMessageId) {
  if (RxMessageId==(MESSAGE_ID_WHLSPD11)) {
    wheelspeed_FL = message_WHLSPD11.c.c[1] & 0x3F;
    wheelspeed_FL<<=8;
    wheelspeed_FL += message_WHLSPD11.c.c[0];
  } else
  if (RxMessageId==(MESSAGE_ID_595)) {
    ri_addSampleToQueue();
    P_0W01 = ((int32_t)message_595.s.UBatt_0V1) * ((int32_t)message_595.s.IBatt_0A1);
    if ((wheelspeed_FL>10) || (P_0W01>0)) { /* 10 is 0.3125 km/h */
      /* driving or consuming during parking */
      PIntegralDrive_hiRes += P_0W01; /* integrate the power. Resolution 0.01W per 100ms */
      while (PIntegralDrive_hiRes > PINTEGRAL_HIRES_SCALER) {
        PIntegralDrive_hiRes -= PINTEGRAL_HIRES_SCALER;
        PIntegralDrive_Wh++;
      }
      while (PIntegralDrive_hiRes < -PINTEGRAL_HIRES_SCALER) {
        PIntegralDrive_hiRes += PINTEGRAL_HIRES_SCALER;
        PIntegralDrive_Wh--;
      }
    } else {    
      PIntegralCharge_hiRes += P_0W01; /* integrate the power. Resolution 0.01W per 100ms */
      while (PIntegralCharge_hiRes > PINTEGRAL_HIRES_SCALER) {
        PIntegralCharge_hiRes -= PINTEGRAL_HIRES_SCALER;
        PIntegralCharge_Wh++;
      }
      while (PIntegralCharge_hiRes < -PINTEGRAL_HIRES_SCALER) {
        PIntegralCharge_hiRes += PINTEGRAL_HIRES_SCALER;
        PIntegralCharge_Wh--;
      }
    } 
  }
}

void candrvF_cyclic(void) {
 //canAppl_divider10ms_to_1s++;
 //if (canAppl_divider10ms_to_1s==100) {
 //  canAppl_divider10ms_to_1s=0;
 //  if (timeoutcounter_DATETIME) timeoutcounter_DATETIME--;
 //}
}

  
