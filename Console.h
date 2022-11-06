
extern void initConsole(void);

extern void printConsoleChar(unsigned char data);

extern void printConsoleDecimalU8(uint8_t value);
extern void printConsoleHexU8( uint8_t value );

extern void printConsoleDecimalU16( uint16_t value );
//extern void printConsoleDecimalU32( uint32_t value );
extern void printConsoleDecimalS16( uint16_t value );
extern void printConsoleHexU16( uint16_t value );

extern void printConsoleString_p(const char *stringImFlash);
extern void printConsoleString(char* s);
extern void printConsoleDecimalU32( uint32_t value );


#define ConsolePutUInt(a) printConsoleDecimalU16(a)
#define ConsolePutChar(a) printConsoleChar(a)
#define ConsolePutHex8(a) printConsoleHexU8(a)
#define ConsolePutHex16(a) printConsoleHexU16(a)

#define ConsoleWrite(a) printConsoleString_p(PSTR(a))

#define lowercase(a) (((a>='A') && (a<='Z'))?(a+32):a)
