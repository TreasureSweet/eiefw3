/*!*********************************************************************************************************************
@file Spi_Master.h                                                                
@brief Header file for Spi_Master
**********************************************************************************************************************/

#ifndef __SPI_MASTER_H
#define __SPI_MASTER_H

/**********************************************************************************************************************
Type Definitions
**********************************************************************************************************************/
#define ANT_MR_AND_SR_STAT           (u32)( NRF_GPIO->IN & ANT_MRH_SRH )
#define ANT_MRH_SRH                  (u32)0x00000300
#define ANT_MRH_SRL                  (u32)0x00000100
#define ANT_MRL_SRH                  (u32)0x00000200
#define ANT_MRL_SRL                  (u32)0x00000000

#define P0_13_ANT_USPI2_MOSI_CNF     ( (GPIO_PIN_CNF_DIR_Output       << GPIO_PIN_CNF_DIR_Pos)   | \
									   (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos) | \
									   (GPIO_PIN_CNF_PULL_Disabled    << GPIO_PIN_CNF_PULL_Pos)  | \
									   (GPIO_PIN_CNF_DRIVE_S0S1       << GPIO_PIN_CNF_DRIVE_Pos) | \
									   (GPIO_PIN_CNF_SENSE_Disabled   << GPIO_PIN_CNF_SENSE_Pos) )

#define P0_12_ANT_USPI2_MISO_CNF     ( (GPIO_PIN_CNF_DIR_Input        << GPIO_PIN_CNF_DIR_Pos)   | \
									   (GPIO_PIN_CNF_INPUT_Connect    << GPIO_PIN_CNF_INPUT_Pos) | \
									   (GPIO_PIN_CNF_PULL_Disabled    << GPIO_PIN_CNF_PULL_Pos)  | \
									   (GPIO_PIN_CNF_DRIVE_S0S1       << GPIO_PIN_CNF_DRIVE_Pos) | \
									   (GPIO_PIN_CNF_SENSE_Disabled   << GPIO_PIN_CNF_SENSE_Pos) )

#define P0_11_ANT_USPI2_SCK_CNF      ( (GPIO_PIN_CNF_DIR_Output       << GPIO_PIN_CNF_DIR_Pos)   | \
									   (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos) | \
								       (GPIO_PIN_CNF_PULL_Disabled    << GPIO_PIN_CNF_PULL_Pos)  | \
									   (GPIO_PIN_CNF_DRIVE_S0S1       << GPIO_PIN_CNF_DRIVE_Pos) | \
									   (GPIO_PIN_CNF_SENSE_Disabled   << GPIO_PIN_CNF_SENSE_Pos) )

#define P0_09_ANT_SRDY_CNF           ( (GPIO_PIN_CNF_DIR_Input        << GPIO_PIN_CNF_DIR_Pos)   | \
									   (GPIO_PIN_CNF_INPUT_Connect    << GPIO_PIN_CNF_INPUT_Pos) | \
									   (GPIO_PIN_CNF_PULL_Disabled    << GPIO_PIN_CNF_PULL_Pos)  | \
									   (GPIO_PIN_CNF_DRIVE_S0S1       << GPIO_PIN_CNF_DRIVE_Pos) | \
									   (GPIO_PIN_CNF_SENSE_Disabled   << GPIO_PIN_CNF_SENSE_Pos) )

#define P0_08_ANT_MRDY_CNF           ( (GPIO_PIN_CNF_DIR_Input        << GPIO_PIN_CNF_DIR_Pos)   | \
									   (GPIO_PIN_CNF_INPUT_Connect    << GPIO_PIN_CNF_INPUT_Pos) | \
									   (GPIO_PIN_CNF_PULL_Disabled    << GPIO_PIN_CNF_PULL_Pos)  | \
									   (GPIO_PIN_CNF_DRIVE_S0S1       << GPIO_PIN_CNF_DRIVE_Pos) | \
									   (GPIO_PIN_CNF_SENSE_Disabled   << GPIO_PIN_CNF_SENSE_Pos) )

#define P0_10_ANT_CS_CNF             ( (GPIO_PIN_CNF_DIR_Output       << GPIO_PIN_CNF_DIR_Pos)   | \
									   (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos) | \
								       (GPIO_PIN_CNF_PULL_Disabled    << GPIO_PIN_CNF_PULL_Pos)  | \
									   (GPIO_PIN_CNF_DRIVE_S0S1       << GPIO_PIN_CNF_DRIVE_Pos) | \
									   (GPIO_PIN_CNF_SENSE_Disabled   << GPIO_PIN_CNF_SENSE_Pos) )

#define SPI0_CONFIG_CNF              ( (SPI_CONFIG_CPOL_ActiveLow    <<   SPI_CONFIG_CPOL_Pos)  | \
                                       (SPI_CONFIG_CPHA_Leading       <<   SPI_CONFIG_CPHA_Pos)  | \
									   (SPI_CONFIG_ORDER_MsbFirst     <<   SPI_CONFIG_ORDER_Pos) )

#define SPI0_FREQUENCY_CNF           (  SPI_FREQUENCY_FREQUENCY_K125 << SPI_FREQUENCY_FREQUENCY_Pos  )

//#define RX_READY ( (AT91C_BASE_US2->US_IMR & AT91C_US_RXRDY) && \
//                   (AT91C_BASE_US2->US_CSR & AT91C_US_RXRDY)       )
//
//#define TX_READY ( (AT91C_BASE_US2->US_IMR & AT91C_US_TXRDY) && \
//                   (AT91C_BASE_US2->US_CSR & AT91C_US_TXRDY)       )
/**********************************************************************************************************************
Function Declarations
**********************************************************************************************************************/


/*------------------------------------------------------------------------------------------------------------------*/
/*! @publicsection */                                                                                            
/*--------------------------------------------------------------------------------------------------------------------*/
u8 SpiGetRXD(void);


/*------------------------------------------------------------------------------------------------------------------*/
/*! @protectedsection */                                                                                            
/*--------------------------------------------------------------------------------------------------------------------*/
void SpiMasterInitialize(void);
void SpiMasterRunActiveState(void);
void SpiMasterCB_Handle(void);


/*------------------------------------------------------------------------------------------------------------------*/
/*! @privatesection */                                                                                            
/*--------------------------------------------------------------------------------------------------------------------*/
static void SpiMaster_CB(void);


/***********************************************************************************************************************
State Machine Declarations
***********************************************************************************************************************/
static void SpiMasterSM_Idle(void);
static void SpiMasterSM_Error(void);
static void SpiMasterSM_Sync(void);


/**********************************************************************************************************************
Constants / Definitions
**********************************************************************************************************************/


#endif /* __SPI_MASTER_H */
/*--------------------------------------------------------------------------------------------------------------------*/
/* End of File                                                                                                        */
/*--------------------------------------------------------------------------------------------------------------------*/
