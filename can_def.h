
/* Hinweise
- Der AVR-GCC und der GCC auf dem Raspberry legen die Strukturen gleich an, außer dass
  der linux-GCC auf dem Raspberry die uint16 aligned. Daher immer die Strukturen so anlegen,
  dass die uint16 alined sind! Oder PACKED verwenden.

  Change history
  2022-08-30 Uwe:
   - neu für Ioniq Motor-CAN angelegt
*/

/***************************************************/
/* common definitions */
/***************************************************/
#ifdef __AVR__
 /* Der AVR-GCC packt allein auf Bytegrenzen und würde nur unnötig Warnungen werfen. Daher leeres Define. */
 #define PACKED
#else
 /* Für den Linux-GCC auf dem Raspberry: auf Bytegrenzen packen: */
 #define PACKED __attribute__((packed))
#endif

typedef struct {
 uint8_t c[8];
} t_array8;
/***************************************************/
/***** message definitions *************************/
/***************************************************/

/*********************** WHLSPD11 *************/
#define MESSAGE_ID_WHLSPD11 0x386
typedef struct  {
 int16_t x; /* */
} t_struct_WHLSPD11;
typedef union  {
 t_struct_WHLSPD11 s;
 t_array8 c;
} t_union_WHLSPD11;
extern t_union_WHLSPD11 message_WHLSPD11;

/*********************** 542 *************/
#define MESSAGE_ID_542 0x542
typedef struct  {
 uint8_t SOC_0prz5; /* SOC display in 0.5% */
} t_struct_542;
typedef union  {
 t_struct_542 s;
 t_array8 c;
} t_union_542;
extern t_union_542 message_542;


/*********************** 595 *************/
#define MESSAGE_ID_595 0x595
typedef struct  {
 int16_t undef01; 
 int16_t undef23;
 int16_t IBatt_0A1; /* battery current in 0.1A */
 int16_t UBatt_0V1; /* battery voltage in 0.1V */
} t_struct_595;
typedef union  {
 t_struct_595 s;
 t_array8 c;
} t_union_595;
extern t_union_595 message_595;

/*********************** BMS2 *************/
#define MESSAGE_ID_BMS2 0x596
typedef struct  {
 uint8_t undef0;
 uint8_t UKL30_0V1;
 uint16_t remChgTime_min;
 uint16_t remChgTimeHPC_min;
 uint8_t TBatMin_C; /* battery min temperature in 1°C */
 uint8_t TBatMax_C; /* battery max temperature in 1°C */
} t_struct_BMS2;
typedef union  {
 t_struct_BMS2 s;
 t_array8 c;
} t_union_BMS2;
extern t_union_BMS2 message_BMS2;

/*********************** 5D0 *************/
#define MESSAGE_ID_5D0 0x5D0
typedef struct  {
 uint8_t ODO_L; 
 uint8_t ODO_M; 
 uint8_t ODO_H; 
} t_struct_5D0;
typedef union  {
 t_struct_5D0 s;
 t_array8 c;
} t_union_5D0;
extern t_union_5D0 message_5D0;


/*********************** TEST *************/
#define MESSAGE_ID_TEST 0x777
typedef struct  {
 int16_t T0; /*LSB0.1C*/
 int16_t T1; /*LSB0.1C*/
 int16_t T2; /*LSB0.1C*/
 int16_t T3; /*LSB0.1C*/
} t_struct_TEST;
typedef union  {
 t_struct_TEST s;
 t_array8 c;
} t_union_TEST;
extern t_union_TEST message_TEST;

