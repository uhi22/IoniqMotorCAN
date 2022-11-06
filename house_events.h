/*************************************************************************************************************
    Haus-Event-Definition
	
	Wegen Universalität sollen neue Ausgänge NICHT über neue Definitionen hier festgelegt werden, sondern
	grundsätzlich über OUTPUTCONTROL_EXT angesteuert werden!
**************************************************************************************************************/
#define HOUSEEVENT_ROT_AN    1 /* !!! Deprecated !!! */
#define HOUSEEVENT_ROT_AUS   2 /* !!! Deprecated !!! */
#define HOUSEEVENT_GRUEN_AN  3 /* !!! Deprecated !!! */
#define HOUSEEVENT_GRUEN_AUS 4 /* !!! Deprecated !!! */

#define HOUSEEVENT_XMAS_FRONTDOOR_AN   5 /* !!! Deprecated !!! */
#define HOUSEEVENT_XMAS_FRONTDOOR_AUS  6 /* !!! Deprecated !!! */
#define HOUSEEVENT_XMAS_FRONTDOOR_AUTO 7 /* !!! Deprecated !!! */

#define HOUSEEVENT_HEIZUNG_WARMWASSER_NORMAL 8 /* !!! Deprecated !!! */
#define HOUSEEVENT_HEIZUNG_WARMWASSER_COOL   9 /* !!! Deprecated !!! */

#define HOUSEEVENT_LUEFTUNG_AUS_DEPRECATED   10 /* !!! Deprecated !!! */
#define HOUSEEVENT_LUEFTUNG_MIN_DEPRECATED   11 /* !!! Deprecated !!! */
#define HOUSEEVENT_LUEFTUNG_MID_DEPRECATED   12 /* !!! Deprecated !!! */
#define HOUSEEVENT_LUEFTUNG_MAX_DEPRECATED   13 /* !!! Deprecated !!! */
#define HOUSEEVENT_LUEFTUNG_AUTO_DEPRECATED  14 /* !!! Deprecated !!! */

//#define HOUSEEVENT_HEIZUNG_MODE_NORMAL 15 /* !!! Deprecated !!! */
//#define HOUSEEVENT_HEIZUNG_MODE_OFEN   16 /* !!! Deprecated !!! */

#define HOUSEEVENT_OUTPUTCHANNELCONTROL  17     /* output control 256 Kanäle mit je 256 Werten */  /* !!! Deprecated !!! */
#define HOUSEEVENT_OUTPUTCHANNELCONTROL_EXT  18 /* output control 65536 Kanäle mit je 4 Byte Daten */

/*************************************************************************************************************
	Kanaldefinition für OUTPUTCONTROL_EXT

	Jeder Knoten hat eine Basis-Adresse und kann ab dieser Adresse bis 255 Kanäle auswerten.
	
	!!! Deprecated !!! Neu: Die Basisadresse soll direkt aus der Diag-Node-Id abgeleitet werden. Siehe can_def.h und board.h.
	
*************************************************************************************************************/

#define OUTPUTCHANNEL_BASE_LIGHT_OVERALL 0x101 /* overall light control. Knoten LichtCAN. 
         Bit 31: 1 --> niederwertigere Bits steuern die Lichter.
                 0 --> Steuerung durch DMX */
				 
#define OUTPUTCHANNEL_BASE_TEMP_SENS_CAN 0x201 /* Temperatursteuerung Wohnzimmer. Knoten TempSensCAN */

#define OUTPUTCHANNEL_BASE_ZENTRALHEIZUNG 0x301 /* Steuerung Zentralheizung. Knoten ZentralheizungCAN */
  // [0] Solltemp. 
  // [1] Solltemp.
  // [2] Anforderung Zirku-Pumpe in Sekunden (0 bis 600 Sekunden = 0 bis 10 Minuten)
  // [3] ToDo: Anforderung Warmwasser-Speicher laden
  
#define OUTPUTCHANNEL_BASE_DMX 0xF000 /* theoretisch 512 Bytes DMX-Daten auf 128 OUTPUTCHANNELs * 4 Byte */
