

 
extern void candrvF_cyclic(void);
extern void candrvF_init(void);
extern void candrvF_sendMessage(uint16_t id, 
      uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3,
      uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7);

extern void candrvF_sendMessageBuffer(uint16_t id, uint8_t *buffer);
