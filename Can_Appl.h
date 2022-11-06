/* 

  Header for Application specific part of CAN Driver

*/ 

extern int32_t P_0W01;
extern int32_t PIntegralDrive_hiRes;
extern int32_t PIntegralDrive_Wh;
extern int32_t PIntegralCharge_hiRes;
extern int32_t PIntegralCharge_Wh;
extern uint16_t wheelspeed_FL;
extern uint32_t ODO_0km1;

extern uint8_t timeoutcounter_WHLSPD11;
extern uint8_t timeoutcounter_542;
extern uint8_t timeoutcounter_595;
extern uint8_t timeoutcounter_5D0;
extern uint8_t timeoutcounter_BMS2;

/******** interface functions ************************************/


extern uint8_t* canApplF_provideRxBuffer(uint16_t RxMessageId);
extern void     canApplF_RxIndication(uint16_t RxMessageId);
extern void     canApplF_cyclic(void);



  
