/*!*********************************************************************************************************************
@file Spi_Slave.c                                                                
@brief Functions for spi slave

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
All Global variable names shall start with "G_<type>SpiSlave"
***********************************************************************************************************************/
/* New variables */
volatile u32 G_u32SpiSlaveFlags;                          /*!< @brief Global state flags */

/*--------------------------------------------------------------------------------------------------------------------*/
/* Existing variables (defined in other files -- should all contain the "extern" keyword) */
extern volatile u32 G_u32SystemTime1ms;                   /*!< @brief From main.c */
extern volatile u32 G_u32SystemTime1s;                    /*!< @brief From main.c */
extern volatile u32 G_u32SystemFlags;                     /*!< @brief From main.c */
extern volatile u32 G_u32ApplicationFlags;                /*!< @brief From main.c */


/***********************************************************************************************************************
Global variable definitions with scope limited to this local application.
Variable names shall start with "SpiSlave_<type>" and be declared as static.
***********************************************************************************************************************/
static fnCode_type SpiSlave_pfStateMachine;               /*!< @brief The state machine function pointer */


/**********************************************************************************************************************
Function Definitions
**********************************************************************************************************************/

/*--------------------------------------------------------------------------------------------------------------------*/
/*! @publicsection */                                                                                            
/*--------------------------------------------------------------------------------------------------------------------*/
/*
- Name: SpiGetRHR(void)
- Function:
   Get DATA in RHR register and return u8 data and set EVENTS_READY to 0

- Can only return one data at once

- If no data or occurs an error, return 0xFF.
   You need to check if the data is 0xFF or there occurs an error.
*/
u8 SpiGetRHR(void)
{
	if(RX_READY)
	{
		return ( 0xFF & AT91C_BASE_US2->US_RHR );
	}
	
	return 0xFF;
} /* end SpiGetRXD(void) */


/*--------------------------------------------------------------------------------------------------------------------*/
/*! @protectedsection */                                                                                            
/*--------------------------------------------------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------------------------------------------------
@fn void SpiSlaveInitialize(void)

@brief
Initializes the State Machine and its variables.

Should only be called once in main init section.

Requires:
- NONE

Promises:
- NONE

*/
void SpiSlaveInitialize(void)
{
	/* USART2 INIT */
	AT91C_BASE_US2->US_CR = ANT_US_CR_INIT;
	AT91C_BASE_US2->US_MR = ANT_US_MR_INIT;
	AT91C_BASE_US2->US_IER = ANT_US_IER_INIT;
	AT91C_BASE_US2->US_IDR = ANT_US_IDR_INIT;
	AT91C_BASE_US2->US_BRGR = ANT_US_BRGR_INIT;
	
	/* MRDY & SRDY INIT */
	AT91C_BASE_PIOB->PIO_SODR = (PB_23_ANT_MRDY | PB_24_ANT_SRDY);
	
	/* If good initialization, set state to Idle */
	if( 1 )
	{
		LedOn(YELLOW);
		LedOff(BLUE);
		SpiSlave_pfStateMachine = SpiSlaveSM_Sync;
	}
	else
	{
		/* The task isn't properly initialized, so shut it down and don't run */
		SpiSlave_pfStateMachine = SpiSlaveSM_Error;
	}

} /* end SpiSlaveInitialize() */

  
/*!----------------------------------------------------------------------------------------------------------------------
@fn void SpiSlaveRunActiveState(void)

@brief Selects and runs one iteration of the current state in the state machine.

All state machines have a TOTAL of 1ms to execute, so on average n state machines
may take 1ms / n to execute.

Requires:
- State machine function pointer points at current state

Promises:
- Calls the function to pointed by the state machine function pointer

*/
void SpiSlaveRunActiveState(void)
{
	SpiSlave_pfStateMachine();

} /* end SpiSlaveRunActiveState */


/*------------------------------------------------------------------------------------------------------------------*/
/*! @privatesection */                                                                                            
/*--------------------------------------------------------------------------------------------------------------------*/


/**********************************************************************************************************************
State Machine Function Definitions
**********************************************************************************************************************/
/*-------------------------------------------------------------------------------------------------------------------*/
/* What does this state do? */
static void SpiSlaveSM_Idle(void)
{
	AT91C_BASE_US2->US_THR = 0x16;
	
	if(RX_READY)
	{
		SpiSlave_pfStateMachine = UserApp2SM_RX_CB;
	}
} /* end SpiSlaveSM_Idle() */


/* UserApp2SM_RX_CB */
static void UserApp2SM_RX_CB(void)
{
	SpiGetRHR();
	
	LedToggle(BLUE);
	
	SpiSlave_pfStateMachine = SpiSlaveSM_Idle;
} /* end UserApp2SM_RX_CB() */

/* Wait for Spi slave sync */
static void SpiSlaveSM_Sync(void)
{
	static u8 u8Wait = 255;
	
	if(--u8Wait == 0)
	{
		AT91C_BASE_PIOB->PIO_SODR = PB_24_ANT_SRDY;
	}
	
	if(RX_READY)
	{
		SpiGetRHR();
		LedOff(YELLOW);
		AT91C_BASE_PIOB->PIO_CODR = PB_23_ANT_MRDY;
		SpiSlave_pfStateMachine = SpiSlaveSM_Idle;
	}
	
} /* end SpiSlaveSM_Sync() */


/*-------------------------------------------------------------------------------------------------------------------*/
/* Handle an error */
static void SpiSlaveSM_Error(void)          
{
	
} /* end SpiSlaveSM_Error() */



/*--------------------------------------------------------------------------------------------------------------------*/
/* End of File                                                                                                        */
/*--------------------------------------------------------------------------------------------------------------------*/
