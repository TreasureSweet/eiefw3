/***********************************************************************************************************************
File: leds_anttt.c                                                                

Description:
***********************************************************************************************************************/

#include "configuration.h"


/***********************************************************************************************************************
Global variable definitions with scope across entire project.
All Global variable names shall start with "G_xxLed"
***********************************************************************************************************************/
/*--------------------------------------------------------------------------------------------------------------------*/
/* New variables (all shall start with G_xxLed*/
u32 au32LedsAdr[] = {P0_26_LED_BLU, P0_27_LED_GRN, P0_28_LED_YLW, P0_29_LED_RED};

bool bLedRemind = false;
eLedTypes eLedRemind;
u16 u16LedRemindTime;

/*--------------------------------------------------------------------------------------------------------------------*/
/* External global variables defined in other files (must indicate which file they are defined in) */
extern volatile u32 G_u32SystemTime1ms;                /* From board-specific source file */
extern volatile u32 G_u32SystemTime1s;                 /* From board-specific source file */

extern volatile u32 G_u32ApplicationFlags;             /* From main.c */


/***********************************************************************************************************************
Global variable definitions with scope limited to this local application.
Variable names shall start with "Led_" and be declared as static.
***********************************************************************************************************************/

 

/***********************************************************************************************************************
* Function Definitions
***********************************************************************************************************************/

/*--------------------------------------------------------------------------------------------------------------------*/
/* Public functions */
/*--------------------------------------------------------------------------------------------------------------------*/
/* Check if Led is on */
bool LedCheckOn(eLedTypes eLedType)
{
	if( NRF_GPIO->IN & au32LedsAdr[eLedType] )
	{
		return true;
	}
	else
	{
		return false;
	}
	
}/* end LedCheckOn */


/* Turn on one led */
void LedOn(eLedTypes eLedType)
{
	if( !LedCheckOn(eLedType) )
	{
		NRF_GPIO->OUTSET = au32LedsAdr[eLedType];
	}
	
} /* end void LedOn */


/* Turn off one led */
void LedOff(eLedTypes eLedType)
{
	if( LedCheckOn(eLedType) )
	{
		NRF_GPIO->OUTCLR = au32LedsAdr[eLedType];
	}
	
} /* end void LedOff */


/* Toggle one led */
void LedToggle(eLedTypes eLedType)
{
	if( LedCheckOn(eLedType) )
	{
		NRF_GPIO->OUTCLR = au32LedsAdr[eLedType];
	}
	else
	{
		NRF_GPIO->OUTSET = au32LedsAdr[eLedType];
	}
} /* end void LedToggle */


/* Blink Led once */
void LedRemind(eLedTypes eLedType)
{
	u16LedRemindTime = LedRemindTime;
	eLedRemind = eLedType;
	bLedRemind = true;
	
} /* end LedRemind */


/*--------------------------------------------------------------------------------------------------------------------*/
/* Protected functions */
/*--------------------------------------------------------------------------------------------------------------------*/
/* LedsInitialize(void)
   Do once to enable control of leds
*/
void LedsInitialize(void)
{
	NRF_GPIO->PIN_CNF[P0_29_INDEX_LED_RED] = P0_29_LED_RED_CNF;
	NRF_GPIO->PIN_CNF[P0_28_INDEX_LED_YLW] = P0_28_LED_YLW_CNF;
	NRF_GPIO->PIN_CNF[P0_27_INDEX_LED_GRN] = P0_27_LED_GRN_CNF;
	NRF_GPIO->PIN_CNF[P0_26_INDEX_LED_BLU] = P0_26_LED_BLU_CNF;
	NRF_GPIO->OUTCLR = P0_29_LED_RED;
	NRF_GPIO->OUTCLR = P0_28_LED_YLW;
	NRF_GPIO->OUTCLR = P0_27_LED_GRN;
	NRF_GPIO->OUTCLR = P0_26_LED_BLU;
	
} /* end LedsInitialize(void) */


/* LedsHandle(void)
   Handle led event needs time to finish
*/
void LedsHandle(void)
{
	/* Handle event in LedRemind() */
	if(bLedRemind)
	{
		if(u16LedRemindTime-- == LedRemindTime)
		{
			LedOn(eLedRemind);
		}
		
		if(u16LedRemindTime == 0)
		{
			LedOff(eLedRemind);
			bLedRemind = false;
		}
	} /* end event in LedRemind() */
	
} /* end LedsHandle(void) */


/*--------------------------------------------------------------------------------------------------------------------*/
/* Private functions */
/*--------------------------------------------------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------------------------------------------------*/
/* End of File */
/*--------------------------------------------------------------------------------------------------------------------*/


