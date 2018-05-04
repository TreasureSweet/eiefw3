/*!*********************************************************************************************************************
@file Spi_Master.c                                                                
@brief Functions for spi master

------------------------------------------------------------------------------------------------------------------------
GLOBALS
- NONE

CONSTANTS
- NONE

TYPES
- NONE

PUBLIC FUNCTIONS
- NONE

PROTECTED FUNCTIONS
- NONE


**********************************************************************************************************************/

#include "configuration.h"

/***********************************************************************************************************************
Global variable definitions with scope across entire project.
All Global variable names shall start with "G_<type>SpiMaster"
***********************************************************************************************************************/
/* New variables */
volatile u32 G_u32SpiMasterFlags;                          /*!< @brief Global state flags */

/*--------------------------------------------------------------------------------------------------------------------*/
/* Existing variables (defined in other files -- should all contain the "extern" keyword) */
extern volatile u32 G_u32SystemTime1ms;                   /*!< @brief From main.c */
extern volatile u32 G_u32SystemTime1s;                    /*!< @brief From main.c */

/***********************************************************************************************************************
Global variable definitions with scope limited to this local application.
Variable names shall start with "SpiMaster_<type>" and be declared as static.
***********************************************************************************************************************/
static fnCode_type SpiMaster_pfStateMachine;               /*!< @brief The state machine function pointer */


/**********************************************************************************************************************
Function Definitions
**********************************************************************************************************************/

/*--------------------------------------------------------------------------------------------------------------------*/
/*! @publicsection */                                                                                            
/*--------------------------------------------------------------------------------------------------------------------*/
/*
- Name: SpiGetRXD(void)
- Function:
   Get DATA in RXD register and return u8 data and set EVENTS_READY to 0

- Can only return one data at once

- If no data or occurs an error, return 0xFF.
   You need to check if the data is 0xFF or there occurs an error.
*/
u8 SpiGetRXD(void)
{
	if(NRF_SPI0->EVENTS_READY)
	{
		NRF_SPI0->EVENTS_READY = 0;
		return ( 0xFF & NRF_SPI0->RXD );
	}
	
	if(NRF_SPI1->EVENTS_READY)
	{
		NRF_SPI1->EVENTS_READY = 0;
		return ( 0xFF & NRF_SPI1->RXD );
	}
	
	return 0xFF;
} /* end SpiGetRXD(void) */


/*--------------------------------------------------------------------------------------------------------------------*/
/*! @protectedsection */                                                                                            
/*--------------------------------------------------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------------------------------------------------
@fn void SpiMasterInitialize(void)

@brief
Initializes the State Machine and its variables.

Should only be called once in main init section.

Requires:
- NONE

Promises:
- NONE

*/
void SpiMasterInitialize(void)
{
	/* GPIO INIT */
	NRF_GPIO->PIN_CNF[P0_13_INDEX_ANT_USPI2_MOSI] = P0_13_ANT_USPI2_MOSI_CNF;
	NRF_GPIO->PIN_CNF[P0_12_INDEX_ANT_USPI2_MISO] = P0_12_ANT_USPI2_MISO_CNF;
	NRF_GPIO->PIN_CNF[P0_11_INDEX_ANT_USPI2_SCK]  = P0_11_ANT_USPI2_SCK_CNF;
	NRF_GPIO->PIN_CNF[P0_09_INDEX_ANT_SRDY]       = P0_09_ANT_SRDY_CNF;
	NRF_GPIO->PIN_CNF[P0_08_INDEX_ANT_MRDY]       = P0_08_ANT_MRDY_CNF;
	NRF_GPIO->PIN_CNF[P0_10_INDEX_ANT_CS]         = P0_10_ANT_CS_CNF;
	
	/* NRF SPI0 INIT */
	NRF_SPI0->PSELSCK        =      P0_11_INDEX_ANT_USPI2_SCK;
	NRF_SPI0->PSELMISO       =      P0_12_INDEX_ANT_USPI2_MISO;
	NRF_SPI0->PSELMOSI       =      P0_13_INDEX_ANT_USPI2_MOSI;
	NRF_SPI0->FREQUENCY      =      SPI0_FREQUENCY_CNF;
	NRF_SPI0->CONFIG         =      SPI0_CONFIG_CNF;
//	NRF_SPI0->EVENTS_READY   =      0;
	
//	NRF_GPIO->OUTCLR         =      P0_10_ANT_CS;
	NRF_TWI0->ENABLE         =      TWI_ENABLE_ENABLE_Disabled << TWI_ENABLE_ENABLE_Pos;
	NRF_SPI0->ENABLE         =      SPI_ENABLE_ENABLE_Enabled << SPI_ENABLE_ENABLE_Pos;
	
	/* Clear MOSI as config */
//	NRF_GPIO->OUTCLR         =      P0_13_ANT_USPI2_MOSI;
	
	
	/* If good initialization, set state to Idle */
	if( 1 )
	{
		NRF_GPIO->OUTSET = P0_28_LED_YLW;
		SpiMaster_pfStateMachine = SpiMasterSM_Sync;
	}
	else
	{
		/* The task isn't properly initialized, so shut it down and don't run */
		SpiMaster_pfStateMachine = SpiMasterSM_Error;
	}

} /* end SpiMasterInitialize() */

  
/*!----------------------------------------------------------------------------------------------------------------------
@fn void SpiMasterRunActiveState(void)

@brief Selects and runs one iteration of the current state in the state machine.

All state machines have a TOTAL of 1ms to execute, so on average n state machines
may take 1ms / n to execute.

Requires:
- State machine function pointer points at current state

Promises:
- Calls the function to pointed by the state machine function pointer

*/
void SpiMasterRunActiveState(void)
{
	SpiMaster_pfStateMachine();

} /* end SpiMasterRunActiveState */


/*------------------------------------------------------------------------------------------------------------------*/
/*! @privatesection */                                                                                            
/*--------------------------------------------------------------------------------------------------------------------*/


/**********************************************************************************************************************
State Machine Function Definitions
**********************************************************************************************************************/
/*-------------------------------------------------------------------------------------------------------------------*/
/* What does this state do? */
static void SpiMasterSM_Idle(void)
{
	static u16 u16Wait = 5000;
	
	if(NRF_GPIO->IN & P0_08_ANT_MRDY)
	{
		SpiGetRXD();
		NRF_GPIO->OUTCLR = P0_26_LED_BLU;
		NRF_GPIO->OUTSET = P0_28_LED_YLW;
		SpiMaster_pfStateMachine = SpiMasterSM_Sync;
	}
	else
	{
		if(u16Wait-- == 0)
		{
			u16Wait = 5000;
			NRF_SPI0->TXD    = 0x23;
		}
	}
	
	if(NRF_SPI0->EVENTS_READY)
	{
		SpiMaster_pfStateMachine = SpiMasterSM_CB;
	}
	
} /* end SpiMasterSM_Idle() */


/* Spi TXD byte sent and RXD byte received callback */
static void SpiMasterSM_CB(void)
{
	SpiGetRXD();
	
	if(NRF_GPIO->OUT & P0_26_LED_BLU)
	{
		NRF_GPIO->OUTCLR = P0_26_LED_BLU;
	}
	else
	{
		NRF_GPIO->OUTSET = P0_26_LED_BLU;
	}
	
	SpiMaster_pfStateMachine = SpiMasterSM_Idle;
	
} /* end SpiMasterSM_CB() */

/* Wait for Sync */
static void SpiMasterSM_Sync(void)
{
	static u16 u16Count = 1000;
	
	if(--u16Count == 0)
	{
		u16Count = 0;
		NRF_SPI0->TXD    = 0xFF;
	}
	
	if( !(NRF_GPIO->IN & P0_08_ANT_MRDY) )
	{
		SpiGetRXD();
		NRF_GPIO->OUTCLR = P0_28_LED_YLW;
		SpiMaster_pfStateMachine = SpiMasterSM_Idle;
	}
	
} /* end SpiMasterSM_Sync() */


/*-------------------------------------------------------------------------------------------------------------------*/
/* Handle an error */
static void SpiMasterSM_Error(void)          
{
	
} /* end SpiMasterSM_Error() */



/*--------------------------------------------------------------------------------------------------------------------*/
/* End of File                                                                                                        */
/*--------------------------------------------------------------------------------------------------------------------*/
