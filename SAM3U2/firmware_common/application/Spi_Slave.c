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

static u8 au8GameInterface[][51] =
{
	"012345678901234567890123456789012345678901234567\n\r",
	"1               |               |               \n\r",
	"2               |               |               \n\r",
	"3               |               |               \n\r",
	"4       1       |       2       |       3       \n\r",
	"5               |               |               \n\r",
	"6               |               |               \n\r",
	"7---------------|---------------|---------------\n\r",
	"8               |               |               \n\r",
	"9               |               |               \n\r",
	"0               |               |               \n\r",
	"1       4       |       5       |       6       \n\r",
	"2               |               |               \n\r",
	"3               |               |               \n\r",
	"4---------------|---------------|---------------\n\r",
	"5               |               |               \n\r",
	"6               |               |               \n\r",
	"7               |               |               \n\r",
	"8       7       |       8       |       9       \n\r",
	"9               |               |               \n\r",
	"0               |               |               \n\r"
};

static u8 *pau8GameAddr[] = 
{
	&au8GameInterface[4][8],
	&au8GameInterface[4][24],
	&au8GameInterface[4][40],
	&au8GameInterface[11][8],
	&au8GameInterface[11][24],
	&au8GameInterface[11][40],
	&au8GameInterface[18][8],
	&au8GameInterface[18][24],
	&au8GameInterface[18][40],
};

static bool bSlaveRound = TRUE;
static bool bWhoFirst = TRUE;
static u8 *pGameSelectAddr = NULL;

/*--------------------------------------------------------------------------------------------------------------------*/
/* Existing variables (defined in other files -- should all contain the "extern" keyword) */
extern volatile u32 G_u32SystemTime1ms;                   /*!< @brief From main.c */
extern volatile u32 G_u32SystemTime1s;                    /*!< @brief From main.c */
extern volatile u32 G_u32SystemFlags;                     /*!< @brief From main.c */
extern volatile u32 G_u32ApplicationFlags;                /*!< @brief From main.c */
extern u8 G_au8DebugScanfBuffer[DEBUG_SCANF_BUFFER_SIZE]; /* From debug.c */
extern u8 G_u8DebugScanfCharCount;                    /* From debug.c */


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


/*
- Name: DebugPrintGame(void)
- Function:
   Print Game InterFace
*/
static void DebugPrintGame(void)
{
	/* Print the Two-dimensional game array */
	for(u8 i = sizeof(au8GameInterface) / 51, *pu8Point = au8GameInterface[0]; i; i--, pu8Point += 51)
	{
		DebugPrintf(pu8Point);
	}
	
} /* end DebugPrintGame() */


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
	AT91C_BASE_US2->US_MR = ( ANT_US_MR_INIT | 0x100 );
	AT91C_BASE_US2->US_IER = ANT_US_IER_INIT;
	AT91C_BASE_US2->US_IDR = ANT_US_IDR_INIT;
	AT91C_BASE_US2->US_BRGR = ANT_US_BRGR_INIT;
	AT91C_BASE_PMC->PMC_PCER |= (1 << AT91C_ID_US2);
	
	/* MRDY & SRDY INIT */
	AT91C_BASE_PIOB->PIO_SODR = ( PB_24_ANT_SRDY | PB_23_ANT_MRDY);
	
	/* LED INIT */
	LedOff(GREEN);
	LedOff(BLUE);
	
	/* If good initialization, set state to Idle */
	if( 1 )
	{
		LedOn(YELLOW); //Wait for sync
		SpiSlave_pfStateMachine = SpiSlaveSM_Sync;
	}
	else
	{
		/* The task isn't properly initialized, so shut it down and don't run */
		SpiSlave_pfStateMachine = SpiSlaveSM_Error;
	}

} /* end SpiSlaveInitialize() */


/* void SpiMessageHandle(void)
Do if get rx or need tx
*/
void SpiMessageHandle(void)
{
	// New data has been storaged in THR register
	if( (AT91C_BASE_US2->US_CSR & AT91C_US_TXEMPTY) == 0 )
	{
		CLR_SRDY();
	}
	
	// Get new data from spi
	if(RX_READY)
	{	
		SpiSlave_RX_CB();
	}
	
	// Handle the select address command for game
	if( (*pGameSelectAddr >= '1') && (*pGameSelectAddr <= '9') )
	{
		AT91C_BASE_US2->US_THR = *pGameSelectAddr;
		
		if(bSlaveRound)
		{
			*pGameSelectAddr = 'O';
		}
		else
		{
			*pGameSelectAddr = 'X';
		}
		
		SpiGameFinshCheck();
	}
	
} /* end SpiMessageHandle() */

/* void SpiGameFinshCheck(void)
Check if one player has won
*/
void SpiGameFinshCheck(void)
{
	static u8 u8RoundTimes = 0;   // If game has run 9 rounds and no one win, that's draw
	static bool bFinish = FALSE;  // Check if someone win
	u8 au8Result[8];              // 8 win probability
	
	/* Print */
	DebugLineFeed();
	DebugPrintGame();
	
	/* Every time run this api, game round add 1 */
	u8RoundTimes++;
	
	/*------If three variable <&> and the result is 'X' or 'O', that means someone win----------*/
	// Get the <&> result
	au8Result[0] = (*pau8GameAddr[0]) & (*pau8GameAddr[1]) & (*pau8GameAddr[2]);
	au8Result[1] = (*pau8GameAddr[3]) & (*pau8GameAddr[4]) & (*pau8GameAddr[5]);
	au8Result[2] = (*pau8GameAddr[6]) & (*pau8GameAddr[7]) & (*pau8GameAddr[8]);
	au8Result[3] = (*pau8GameAddr[0]) & (*pau8GameAddr[3]) & (*pau8GameAddr[6]);
	au8Result[4] = (*pau8GameAddr[1]) & (*pau8GameAddr[4]) & (*pau8GameAddr[7]);
	au8Result[5] = (*pau8GameAddr[2]) & (*pau8GameAddr[5]) & (*pau8GameAddr[8]);
	au8Result[6] = (*pau8GameAddr[0]) & (*pau8GameAddr[4]) & (*pau8GameAddr[8]);
	au8Result[7] = (*pau8GameAddr[2]) & (*pau8GameAddr[4]) & (*pau8GameAddr[6]);
	
	// Check the <&> result
	for(u8 i = sizeof(au8Result); i; i--)
	{
		if( (au8Result[i-1] == 'X') ) //Master win
		{
			DebugPrintf("\n\rBLE win!\n\r");
			bFinish = TRUE;
		}
		
		if( (au8Result[i-1] == 'O') ) //Slave win
		{
			DebugPrintf("\n\rYou win!\n\r");
			bFinish = TRUE;
		}
	}
	/*-----------------------------------------Finish--------------------------------------------*/
	
	/* Game finish or not finish, 4 probability */
	if(bFinish) //Finish, someone win
	{
		bFinish = FALSE;
		u8RoundTimes = 0;
		LedOff(WHITE);
		LedOff(PURPLE);
		LedOff(BLUE);
		LedOff(YELLOW);
		LedOff(GREEN);
		LedOff(CYAN);
		LedOff(ORANGE);
		LedOff(RED);
		SpiSlave_pfStateMachine = GameFinishSM;
	}
	else if(u8RoundTimes == 9) //Finish, draw
	{
		LedOff(WHITE);
		LedOff(PURPLE);
		LedOff(BLUE);
		LedOff(YELLOW);
		LedOff(GREEN);
		LedOff(CYAN);
		LedOff(ORANGE);
		LedOff(RED);
		DebugPrintf("\n\rDraw!\n\r");
		SpiSlave_pfStateMachine = GameFinishSM;
	}
	else //Not finish, print notes
	{
		/* Change the round so that another one can't input */
		if(bSlaveRound)
		{
			bSlaveRound = FALSE;
			DebugPrintf("\n\rWait Ble input\n\r");
		}
		else
		{
			bSlaveRound = TRUE;
			DebugPrintf("\n\rBLE Finish\n\rYour input:");
		} /* end if(bSlaveRound) */
		
	} /* end if(bFinish) */
	
} /* end SpiGameFinshCheck() */


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
	SpiMessageHandle();
	
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
	u8 au8DebugScanf[3];
	
	/* Check if this is the slave round */
	if(bSlaveRound || bWhoFirst)
	{
		if(DebugScanf(au8DebugScanf))
		{
			if( (au8DebugScanf[0] >= '1') && (au8DebugScanf[0] <= '9') )
			{
				pGameSelectAddr = pau8GameAddr[au8DebugScanf[0] - 49];
				
				/* If this is the fist input, change the round bool varibable */
				if(bWhoFirst)
				{
					bWhoFirst = FALSE;
					bSlaveRound = TRUE;
				}
			}
			
			if( (*pGameSelectAddr == 'X') || (*pGameSelectAddr == 'O') || (pGameSelectAddr == NULL) )
			{
				DebugPrintf("\n\rInput command error!\n\rYour Input:");
			}
			
		} /* end if(DebugScanf(au8DebugScanf) */
		
	} /* end if(bSlaveRound || bWhoFirst) */
	
} /* end SpiSlaveSM_Idle() */


/* SpiSlaveSM_RX_CB */
static void SpiSlave_RX_CB(void)
{
	static u8 u8Test;
	
	u8Test = SpiGetRHR();
	
	/* Check if this is the master round */
	if(!bSlaveRound || bWhoFirst)
	{
		if( (u8Test >= '1') && (u8Test <= '9') )
		{
			pGameSelectAddr = pau8GameAddr[u8Test - 49];
			
			/* If this is the fist input, change the round bool varibable */
			if(bWhoFirst)
			{
				bWhoFirst = FALSE;
				bSlaveRound = FALSE;
			}
			
			if( (*pGameSelectAddr == 'X') || (*pGameSelectAddr == 'O') )
			{
				AT91C_BASE_US2->US_THR = 'N';
			}
		}
		
		if(u8Test == 0xF0)
		{
			AT91C_BASE_US2->US_THR = 'N';
		}
		
	}
	
	/* Slave THR send successfully, then turn down SRDY */
	if(u8Test == 0x0F)
	{
		SET_SRDY();
	}
	
} /* end SpiSlaveSM_RX_CB() */

/* Wait for Spi slave sync */
static void SpiSlaveSM_Sync(void)
{
	static u8 u8Wait = 255;
	
	/* Give time to nrf51 for check pin state */
	// First MRDY keeps high, SRDY turn to low. Ready for sync (In nrf51 means: ANT_MRH_SRL)
	// Led BLUE on
	if(--u8Wait == 155)
	{
		CLR_SRDY();
		LedOn(BLUE);
		LedOff(YELLOW);
	}
	
	// Next, MRDY keeps high, SRDY return to high. Start sync,
	// nrf51 start sending test message. (In nrf51 means: ANT_MRH_SRH)
	if(u8Wait == 0)
	{
		SET_SRDY();
	}
	/* End time count */
	
	/* RX_READY turns to 1 means slave get the test message,
	   so request to nrf51 that sync finish and jump out this statemachine,
	   LED BLUE off
	*/
	// Finally, MRDY turn to low, SRDY keeps high. Sync finish (In nrf51 means: ANT_MRL_SRH)
	if(RX_READY)
	{
		SpiGetRHR();
		LedOff(BLUE);
		CLR_MRDY();
		u8Wait = 255;
		DebugPrintGame();
		SpiSlave_pfStateMachine = SpiSlaveSM_Idle;
	}
	
} /* end SpiSlaveSM_Sync() */

/* Game finish, press button0 to start new round */
static void GameFinishSM(void)
{
	static u8 u8TimeCount = 255;
	
	/* Button0 to restart game */
	if(WasButtonPressed(BUTTON0))
	{
		ButtonAcknowledge(BUTTON0);
		
		/* variable init */
		bWhoFirst = TRUE;
		u8TimeCount = 255;
		pGameSelectAddr = NULL;
		
		/* game array init */
		*pau8GameAddr[0] = '1';
		*pau8GameAddr[1] = '2';
		*pau8GameAddr[2] = '3';
		*pau8GameAddr[3] = '4';
		*pau8GameAddr[4] = '5';
		*pau8GameAddr[5] = '6';
		*pau8GameAddr[6] = '7';
		*pau8GameAddr[7] = '8';
		*pau8GameAddr[8] = '9';
		
		/* led init */
		LedOff(WHITE);
		LedOff(PURPLE);
		LedOff(BLUE);
		LedOff(YELLOW);
		LedOff(GREEN);
		LedOff(CYAN);
		LedOff(ORANGE);
		LedOff(RED);
		
		/* print game interface */
		DebugLineFeed();
		DebugPrintGame();
		
		/* jump out of this statemachine */
		SpiSlave_pfStateMachine = SpiSlaveSM_Idle;
		
	} /* end Button0 */
	
	/* toggle leds every 255ms */
	if(--u8TimeCount == 0)
	{
		u8TimeCount = 255;
		LedToggle(WHITE);
		LedToggle(PURPLE);
		LedToggle(BLUE);
		LedToggle(YELLOW);
		LedToggle(GREEN);
		LedToggle(CYAN);
		LedToggle(ORANGE);
		LedToggle(RED);
		
	} /* end leds toggle */
	
} /* end GameFinishSM */

/*-------------------------------------------------------------------------------------------------------------------*/
/* Handle an error */
static void SpiSlaveSM_Error(void)          
{
	
} /* end SpiSlaveSM_Error() */



/*--------------------------------------------------------------------------------------------------------------------*/
/* End of File                                                                                                        */
/*--------------------------------------------------------------------------------------------------------------------*/
