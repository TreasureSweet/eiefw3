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

static bool bSlaveRound = TRUE;
static bool bWhoFirst = TRUE;

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
	
	LedOff(GREEN);
	LedOff(BLUE);
	
	/* If good initialization, set state to Idle */
	if( 1 )
	{
		LedOn(YELLOW);
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
	if( (AT91C_BASE_US2->US_CSR & AT91C_US_TXEMPTY) == 0 )
	{
		CLR_SRDY();
	}
	
	if(RX_READY)
	{	
		SpiSlave_RX_CB();
	}
	
} /* end SpiMessageHandle() */

/* void SpiGameFinshCheck(void)
Check if one player has won
*/
void SpiGameFinshCheck(void)
{
	static u8 u8RoundTimes = 0;
	static bool bFinish = FALSE;
	u8 au8Result[8];
	
	u8RoundTimes++;
	
	au8Result[0] = au8GameInterface[4][8]  & au8GameInterface[4][24]  & au8GameInterface[4][40];
	au8Result[1] = au8GameInterface[11][8] & au8GameInterface[11][24] & au8GameInterface[11][40];
	au8Result[2] = au8GameInterface[18][8] & au8GameInterface[18][24] & au8GameInterface[18][40];
	au8Result[3] = au8GameInterface[4][8]  & au8GameInterface[11][8]  & au8GameInterface[18][8];
	au8Result[4] = au8GameInterface[4][24] & au8GameInterface[11][24] & au8GameInterface[18][24];
	au8Result[5] = au8GameInterface[4][40] & au8GameInterface[11][40] & au8GameInterface[18][40];
	au8Result[6] = au8GameInterface[4][8]  & au8GameInterface[11][24] & au8GameInterface[18][40];
	au8Result[7] = au8GameInterface[4][40] & au8GameInterface[11][24] & au8GameInterface[18][8];
	
	for(u8 i = sizeof(au8Result); i; i--)
	{
		if( (au8Result[i] == 'X') ) //Master
		{
			DebugPrintf("\n\rBLE win!\n\r");
			bFinish = TRUE;
		}
		
		if( (au8Result[i] == 'O') ) //Slave
		{
			DebugPrintf("\n\rYou win!\n\r");
			bFinish = TRUE;
		}
	}
	
	if(bFinish)
	{
		bFinish = FALSE;
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
	else if(u8RoundTimes == 9)
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
	else
	{
		if(!bSlaveRound)
		{
			DebugPrintf("\n\rWait Ble input\n\r");
		}
		else
		{
			DebugPrintf("\n\rBLE Finish\n\rYour input:");
		}
	}
	
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
	u8 *pPoint = NULL;
	
	if(bSlaveRound || bWhoFirst)
	{
		if(DebugScanf(au8DebugScanf))
		{
			switch(au8DebugScanf[0])
			{
				case '1':
				{
					pPoint = &au8GameInterface[4][8];
					break;
				}
				
				case '2':
				{
					pPoint = &au8GameInterface[4][24];
					break;
				}
				
				case '3':
				{
					pPoint = &au8GameInterface[4][40];
					break;
				}
				
				case '4':
				{
					pPoint = &au8GameInterface[11][8];
					break;
				}
				
				case '5':
				{
					pPoint = &au8GameInterface[11][24];
					break;
				}
				
				case '6':
				{
					pPoint = &au8GameInterface[11][40];
					break;
				}
				
				case '7':
				{
					pPoint = &au8GameInterface[18][8];
					break;
				}
				
				case '8':
				{
					pPoint = &au8GameInterface[18][24];
					break;
				}
				
				case '9':
				{
					pPoint = &au8GameInterface[18][40];
					break;
				}
				
				default:
				{
					DebugPrintf("\n\rInput command error!\n\rYour Input:");
					break;
				}
			}
		}
		
		if(pPoint != NULL)
		{
			if( (*pPoint != 'X') && (*pPoint != 'O') )
			{
					bSlaveRound = FALSE;
					*pPoint = 'O';
					DebugPrintGame();
					AT91C_BASE_US2->US_THR = au8DebugScanf[0];
					SpiGameFinshCheck();
			}
			else
			{
				DebugPrintf("\n\rInput command error!\n\rYour Input:");
			}
		}
	}
	
} /* end SpiSlaveSM_Idle() */


/* SpiSlaveSM_RX_CB */
static void SpiSlave_RX_CB(void)
{
	static u8 u8Test;
	u8 *pPoint = NULL;
	u8Test = SpiGetRHR();
	
	if(!bSlaveRound || bWhoFirst)
	{
		switch(u8Test)
		{
			/* Right commands */
			case 0x00:
			{
				pPoint = &au8GameInterface[4][8];
				break;
			}
			
			case 0x01:
			{
				pPoint = &au8GameInterface[4][24];
				break;
			}
			
			case 0x02:
			{

				pPoint = &au8GameInterface[4][40];
				break;
			}
			
			case 0x10:
			{

				pPoint = &au8GameInterface[11][8];
				break;
			}
			
			case 0x11:
			{

				pPoint = &au8GameInterface[11][24];
				break;
			}
			
			case 0x12:
			{
				pPoint = &au8GameInterface[11][40];
				break;
			}
			
			case 0x20:
			{
				pPoint = &au8GameInterface[18][8];
				break;
			}
			
			case 0x21:
			{
				pPoint = &au8GameInterface[18][24];
				break;
			}
			
			case 0x22:
			{
				pPoint = &au8GameInterface[18][40];
				break;
			}
			
			case 0xF0:
			{
				AT91C_BASE_US2->US_THR = 'N';
				break;
			}
				
			default:
			{
				break;
			}
		}
	}
	
	if(u8Test == 0x0F)
	{
		SET_SRDY();
	}
	
	if(pPoint != NULL)
	{
		if( (*pPoint != 'X') && (*pPoint != 'O') )
		{
				bSlaveRound = TRUE;
				*pPoint = 'X';
				DebugPrintGame();
				AT91C_BASE_US2->US_THR = 'Y';
				
				if(bWhoFirst)
				{
					bWhoFirst = FALSE;
				}
				
				SpiGameFinshCheck();
		}
		else
		{
			AT91C_BASE_US2->US_THR = 'N';
		}
	}
	
} /* end SpiSlaveSM_RX_CB() */

/* Wait for Spi slave sync */
static void SpiSlaveSM_Sync(void)
{
	static u8 u8Wait = 255;
	
	if(--u8Wait == 155)
	{
		CLR_SRDY();
		LedOn(BLUE);
		LedOff(YELLOW);
	}
	
	if(u8Wait == 0)
	{
		SET_SRDY();
	}
	
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
	
	if(WasButtonPressed(BUTTON0))
	{
		ButtonAcknowledge(BUTTON0);
		
		bWhoFirst = TRUE;
		u8TimeCount = 255;
		au8GameInterface[4][8]    = '1';
		au8GameInterface[4][24]   = '2';
		au8GameInterface[4][40]   = '3';
		au8GameInterface[11][8]   = '4';
		au8GameInterface[11][24]  = '5';
		au8GameInterface[11][40]  = '6';
		au8GameInterface[18][8]   = '7';
		au8GameInterface[18][24]  = '8';
		au8GameInterface[18][40]  = '9';
		LedOff(WHITE);
		LedOff(PURPLE);
		LedOff(BLUE);
		LedOff(YELLOW);
		LedOff(GREEN);
		LedOff(CYAN);
		LedOff(ORANGE);
		LedOff(RED);
		
		DebugLineFeed();
		DebugPrintGame();
		
		SpiSlave_pfStateMachine = SpiSlaveSM_Idle;
	}
	
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
	}
	
} /* end GameFinishSM */

/*-------------------------------------------------------------------------------------------------------------------*/
/* Handle an error */
static void SpiSlaveSM_Error(void)          
{
	
} /* end SpiSlaveSM_Error() */



/*--------------------------------------------------------------------------------------------------------------------*/
/* End of File                                                                                                        */
/*--------------------------------------------------------------------------------------------------------------------*/
