//*****************************************************************************
//! @file       sigfox_demo.c
//! @brief      Main file for sigfox protocol Uplink/Downlink demo
//!
//****************************************************************************/


/**************************************************************************//**
 * @addtogroup Demo
 * @{
 ******************************************************************************/


/******************************************************************************
 * INCLUDES
 */
#include "hal_spi_rf_trxeb.h"
#include "device_config.h"
#include "msp430.h"
#include "cc112x_spi.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "math.h"
#include "assert.h"
#include "bsp.h"
#include "host_cmd.h"
#include "timer.h"
#include "transmission.h"
#include "radio.h"
#include "sigfox_demo.h"
#include "bsp_led.h"
#include "bsp_key.h"
#include "uart_drv.h"
#include "../sigfox_library_api/sigfox.h"
#include "../sigfox_library_api/sigfox_types.h"

#include "driverlib.h"

#ifdef __MSP430F5438A__
#include "lcd_dogm128_6.h"
#endif


/******************************************************************************
 * DEFINES
 */

volatile unsigned short doorSensor = 0;
volatile unsigned short doorStatus = 0;

/******************************************************************************
 * STATIC FUNCTIONS PROTOTYPES
 */
static void initMCU(void);
void resetCF(void);
#ifdef __MSP430F5438A__
#if defined(PB_KEY) && defined (__MSP430F5438A__)
static void updateLCD(void);
#endif
static void welcomeLCD(void);
#endif


/******************************************************************************
 * GLOBAL VARIABLES
 */

/* \brief id and key need to be placed into NON VOLATILE MEMORY */
#pragma DATA_SECTION(id, ".infoA");
u32 id;		/*!< product Id : need to be in Non Volatile memory */
#pragma DATA_SECTION(key, ".infoA");
u8 key[16];	/*!< AES product key : No write access to the user, only for library */

/* Library External Parameters to be initialized */
#pragma DATA_SECTION(TxFrequency, ".infoC");
u32 TxFrequency; /*!< Init of the Transmit central frequency */
#pragma DATA_SECTION(RxFrequency, ".infoB");
u32 RxFrequency; /*!< Init of the Receive central frequency */

u8  TxRep = 2;               /*!< Init of the number of repetition of the Transmit frame which initiate the Downlink */
#if defined(MODE_ETSI)||defined(MODE_ETSI_OPT)
SFX_std_t standard = SFX_STD_ETSI;
#else
SFX_std_t standard = SFX_STD_FCC;
#endif

/* Pointers used by the SigFox library : Do not modify */
u8* key_ptr  = (u8*)&key; /*!< Library key pointer init */
u8* id_ptr   = (u8*)&id;  /*!< Library id pointer init */
u32* TxCF    = (u32*)&TxFrequency; /*!< Library Transmit frequency pointer init */
u32* RxCF    = (u32*)&RxFrequency; /*!< Library Receive frequency pointer init */
u8* TxRepeat = (u8*)&TxRep;

SFX_std_t  *Standard = (SFX_std_t *)&standard;


/******************************************************************************
* LOCAL VARIABLES
*/

/*!
 * \brief Payload message for UL demo
 */
uint8 message[12] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
//uint8 message[12] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xFF};



/*!
 * \brief Return Payload message for DL demo
 */
u8  ReceivedPayload[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

#ifdef __MSP430F5438A__
/*!
 * \brief TI logo for lcd
 */
static const char pLcdTiLogo[1024] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x7E, 0x7E, 0x7E, 0x7E, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFE, 0xFE, 0x06, 0x00, 0xE0, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0x06, 0x00, 0xE0, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x8F, 0x81, 0x80, 0x80, 0x00, 0x00, 0x80, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0x1F, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0xF9, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x07, 0x07, 0x0F, 0x1F, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x03, 0x00, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, 0x00, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x07, 0x1F, 0x3F, 0x7F, 0x7F, 0x7F, 0x7F, 0x3F, 0x1F, 0x0F, 0x0F, 0x07, 0x0F, 0x0F, 0x1F, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xCF, 0x80, 0x00, 0x00, 0x03, 0x03, 0x03, 0x03, 0x03, 0x00, 0x00, 0x00, 0x03, 0x03, 0x83, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x7F, 0x3F, 0x3F, 0x1F, 0x1F, 0x0F, 0x0F, 0x07, 0x07, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x07, 0x1F, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0x7F, 0x1F, 0x07, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x0F, 0x1F, 0x1F, 0x3F, 0x7F, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
#endif


/******************************************************************************
* FUNCTIONS
*/
/***************************************************************************//**
 *   @brief		Initializes dynamic memory blocks
 *   @param 	nb_blocks is the number of blocks
 *   @param		size_block is the size of each block
 *   @param		start_memory_ptr is the pointer to the begining of the block
 *   @param		table_ptr is a pointer to the initialized memory
 ******************************************************************************/
static void
memory_block_init ( int nb_blocks, int size_block, u8 *start_memory_ptr, MemoryBlock *table_ptr )
{
	u8 i = 0;
	for ( i = 0; i < nb_blocks ; i++ )
	{
		table_ptr->memory_ptr = start_memory_ptr + i*size_block;
		table_ptr->allocated = FALSE;
		table_ptr++;
	}
}

/***************************************************************************//**
 *   @brief		Initializes the dynamic memory required for the demo
 ******************************************************************************/
static void
dynamic_memory_init ( void )
{
	memory_block_init(NUMBER_BLOCKS_64BYTES, BYTES_SIZE_64,(u8*)(DynamicMemoryTable + START_MEMORY_BLOCK_64BYTES), Table_64bytes );
	memory_block_init(NUMBER_BLOCKS_32BYTES, BYTES_SIZE_32,(u8*)(DynamicMemoryTable + START_MEMORY_BLOCK_32BYTES), Table_32bytes );
	memory_block_init(NUMBER_BLOCKS_30BYTES, BYTES_SIZE_30,(u8*)(DynamicMemoryTable + START_MEMORY_BLOCK_30BYTES), Table_30bytes );
	memory_block_init(NUMBER_BLOCKS_8BYTES, BYTES_SIZE_8,  (u8*)(DynamicMemoryTable + START_MEMORY_BLOCK_8BYTES) , Table_8bytes );
}


void blink_leds(void)
{
	int i;
	for(i = 0; i < 10; i++)
	{
		bspLedClear(BSP_LED_2);
		bspLedSet(BSP_LED_1);

		__delay_cycles(1600000/2);
		bspLedClear(BSP_LED_1);
		bspLedSet(BSP_LED_2);

		__delay_cycles(1600000/2);
	}
	bspLedClear(BSP_LED_1);
	bspLedClear(BSP_LED_2);
}


/***************************************************************************//**
 *   @brief      Runs the main routine
 *
 *               This function covers the main application code.
 *
 *   @note 		 Depending on the application, you can use either one or several
 *   			 calls from the following functions from the SIGFOX Library:
 *
 *	 @note 		\li SfxSendFrame(message, length, NULL, NULL) : Send an Uplink frame
 *   @note 		\li SfxSendFrame(message, length, ReceivedPayload, TRUE) : Downlink with ack
 *   @note 		\li SfxSendBit( 0, NULL, NULL) : Send a bit
 *   @note 		\li SfxSendOutOfBand() : Send OOB frame
 *   @note 		\li SfxTxTestMode(nb_frames, channel_number) : Tx TestModeFrame activation
 ******************************************************************************/
void
main(void)
{

	WDTCTL = WDTPW + WDTHOLD;                 // Stop Watchdog Timer


	SFX_error_t volatile err;		// error returned from sigfox api functions. (For debug purpose)
#if defined(AT_CMD)
	host_cmd_status_t hostCmdStatus;
	char cmd[40];
	unsigned char length;
#endif
#if defined(PB_KEY)
	unsigned char buttonPressed;
#endif

	//Initialize the memory
	dynamic_memory_init();

	// Initialize MCU and Peripherals
	initMCU();



#ifdef __MSP430F5438A__

	// Display welcome screen on LCD
	welcomeLCD();
#endif

	// Write the uplink and downlink frequencies if device is programmed for first time
	resetCF();

	// SIGFOX library init
	err = SfxInit();
	assert(SFX_ERR_NONE == err);


	///////////////START OF NEW CODE SECTION (1 OF 2)///////////////////////


		P2DIR &= ~0b00010000;  				//initialize P2.4 as input
		P1DIR |= 0b00100000;				//set P1.5 as output (for LED testing)
		P1OUT &= ~0b00100000;	//initialize P1.5 output as 0

		//GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P2, GPIO_PIN_4);	//this line performs the same function as the next 2 lines
		P2REN |= 0b00010000; //enable pull up/down resistor network for input P2.4
		P2OUT |= 0b00010000;	//make port P2.4 resistor network set to pull-up resistor (HIGH)

	///////////////END OF NEW  CODE (1 OF 2)////////////////////////



	// Infinite loop
	for(;;)
	{

///////////////////////START OF NEW CODE (2 OF 2)////////////////////////////////

		doorSensor = GPIO_getInputPinValue( GPIO_PORT_P2, GPIO_PIN4);		//read and store value of digital input pin 4 on port 2 (P2.4)

		if ( doorSensor == 0 ){		//if the state of the door has changed, or the door is open (regardless of state change), trigger transmission sequence

//			doorStatus = doorSensor;	//update previous door status to current door status
			blink_leds();

			message[11] = 0x00;		//door closed
			err = SfxSendFrame(message, sizeof(message), NULL, NULL);		//transmit message to sigfox network that the door has been closed
//			__delay_cycles(16000000); // sleep 10 seconds
//			if (err == SFX_ERR_NONE) {
//				P1OUT &= ~0b00100000;	//turn off LED connected to output port P1.5
//				__delay_cycles(160000000);	//delay door checking for ten seconds
//			}
			blink_leds();

//			if (doorSensor == GPIO_INPUT_PIN_LOW){		//if port P2.4 has LOW input (0), the door has been closed
//
//
//				P1OUT |= 0b00100000;		//turn on LED connected to output port P1.5
//
//
//				message[11] = 0x00;		//door closed
//				err = SfxSendFrame(message, sizeof(message), NULL, NULL);		//transmit message to sigfox network that the door has been closed
//				__delay_cycles(16000000);
//
//				if (err == SFX_ERR_NONE) {
//					P1OUT &= ~0b00100000;	//turn off LED connected to output port P1.5
//					__delay_cycles(160000000);	//delay door checking for ten seconds
//				}
//			}
//
//			if (doorSensor == GPIO_INPUT_PIN_HIGH) {		//if port P2.4 has LOW input (0), the door has been opened
//
//				P1OUT |= 0b00100000;		//turn on LED connected to output port P1.5
//
//				message[11] = 0x01;		//door open
//				err = SfxSendFrame(message, sizeof(message), NULL, NULL);
//				__delay_cycles(16000000);
//
//				if (err == SFX_ERR_NONE) {
//
//					P1OUT &= ~0b00100000;		//turn off LED connected to output port P1.5
//					__delay_cycles(160000000);
//				}
//			}
//
		}



//////////////////////////END OF NEW CODE (2 OF 2)////////////////////////////////////////////////

#if defined(AT_CMD)
		// Detect Carriage Return in the string
		if(uartGetRxEndOfStr() == END_OF_LINE_DETECTED)
		{
			// Reset end of string detection
			uartResetRxEndOfStr();

			// Get the input string and its length
			length = uartGetRxStrLength();
			uartGetStr(cmd, length);

			// Parse the string to find AT commands
			hostCmdStatus = parseHostCmd((unsigned char *)cmd, length);
			assert(HOST_CMD_NOT_FOUND != hostCmdStatus);
		}
#endif
#if defined(PB_KEY)
		buttonPressed = bspKeyPushed(BSP_KEY_ALL);

		// Detect button push to send the message
		if((buttonPressed == BSP_KEY_SELECT) || (buttonPressed == BSP_KEY_UP))
		{
#if defined(__MSP430F5438A__)

			// Update LCD
			updateLCD();

			// Toggle LED indicators
			bspLedClear(BSP_LED_3);
			bspLedClear(BSP_LED_4);
#endif
			bspLedClear(BSP_LED_2);
			bspLedSet(BSP_LED_1);


			if(buttonPressed == BSP_KEY_UP)
			{
				// Send uplink only frame
				err = SfxSendFrame(message, sizeof(message), NULL, NULL);

			}
			else if(buttonPressed == BSP_KEY_SELECT)
			{
				// Send a bi-directional frame
				err = SfxSendFrame(message, sizeof(message), ReceivedPayload, TRUE);
			}

			// Reset button status
			buttonPressed = 0;

#if defined (__MSP430F5438A__)

			// Clear LED1
			bspLedClear(BSP_LED_1);

			// LED indicator for sigfox API error status
			if (err == SFX_ERR_NONE) {
				bspLedSet(BSP_LED_3);
			}
			else {
				bspLedSet(BSP_LED_4);
			}

			// Update LCD
			updateLCD();

#elif defined (__MSP430F5529__)

			if (err == SFX_ERR_NONE) {
				bspLedSet(BSP_LED_2);
			}
			else {
				bspLedClear(BSP_LED_2);
			}
#endif

			// Increment in message
			message[11]++;

			//flip message for testing
			//message[11] ^= 1;
		}
		else
		{
#if defined (__MSP430F5438A__)

			// Set LED2
			bspLedSet(BSP_LED_2);
#elif defined (__MSP430F5529__)

			// Clear LED1
			bspLedClear(BSP_LED_1);
#endif	//MSP
		}
#endif //INTERFACE
	}
}



/***************************************************************************//**
 *   @brief      Initialize MCU and BOARD Peripherals
 *
 *	 @note 		This function initializes the following
 *	 @note 		\li \b MCU \b Clock
 *	 @note 		\li \b BSP \b keys
 *	 @note 		\li \b BSP \b LEDs
 *	 @note 		\li \b LCD display (only for TRXEB)
 *	 @note 		\li \b RF_SPI Interface
 *	 @note 		\li \b UART interface
 *	 @note 		\li \b PA_LNA controls
 *	 @note 		\li \b TIMER Bit Rate for the symbols
 *	 @note 		\li \b INTERRUPT enable service
 *******************************************************************************/
static void
initMCU(void)
{
	// Initialize clocks and I/O
	//bspInit(BSP_SYS_CLK_20MHZ);
	bspInit(BSP_SYS_CLK_24MHZ);

	// Initialize buttons
	bspKeyInit(BSP_KEY_MODE_POLL);

	// Initialize leds
	bspLedInit();

#ifdef __MSP430F5438A__
	// Initialize SPI interface to LCD (shared with SPI flash)
	bspIoSpiInit(BSP_FLASH_LCD_SPI, BSP_FLASH_LCD_SPI_SPD);

	// Initialize LCD
	lcdInit();
#endif
	// Instantiate transceiver RF SPI interface to SCLK ~ 8 MHz */
	/* Input parameter is clockDivider
	 * SCLK frequency = SMCLK/clockDivider
	 */
	//trxRfSpiInterfaceInit(2);
	trxRfSpiInterfaceInit(3);

#ifdef AT_CMD
	// Initialize the UART interface
	halUartInit();

	// Toggle UART Echo. Disabled by default. Might cause unwanted behaviour if enabled.
	// Note: Try enabling local echo on the host console instead!
	//uartDrvToggleEcho();
#endif

#ifdef __MSP430F5529__
	// remove the reset from the rf device
	RF_RESET_N_PORT_SEL &= ~RF_RESET_N_PIN;
	RF_RESET_N_PORT_DIR |= RF_RESET_N_PIN;
	RF_RESET_N_PORT_OUT |= RF_RESET_N_PIN;
#endif
#ifdef CC1190_PA_LNA
	// initialize the IO
	RF_PA_EN_PxDIR |= RF_PA_EN_PIN;
	RF_LNA_EN_PxDIR |= RF_LNA_EN_PIN;

	// configure idle
	RF_PA_EN_PxOUT &= ~RF_PA_EN_PIN;
	RF_LNA_EN_PxOUT &= ~RF_LNA_EN_PIN;
#endif

	// Init Bitrate Timer
	/* - FCC bit rate  = 1.66ms ( 600bps )
	 * - ETSI bit rate = 10 ms  ( 100bps )
	 */
	TIMER_bitrate_init();

	// Enable global interrupt
	_BIS_SR(GIE);
}


/***************************************************************************//**
 *   @brief      Initialize the uplink and downlink frequencies if no value stored
 *
 *				 This function will initialize following values
 *               \li \b TxFrequency = \b ftx
 *               \li \b RxFrequency = \b frx
 *******************************************************************************/
void resetCF(void)
{
	// Write TxFrequency
	if(TxFrequency == 0xFFFFFFFF)
	{
		volatile unsigned long * Flash_ptrC;
		Flash_ptrC = (unsigned long *) &TxFrequency;

		FCTL3 = FWKEY;				// Clear Lock bit
		FCTL1 = FWKEY + ERASE; 		// Set Erase bit
		*Flash_ptrC = 0;			// Dummy write to erase Flash seg
		FCTL1 = FWKEY + BLKWRT;		// Enable long-word write
		*Flash_ptrC = ftx;			// Write to flash
		FCTL1 = FWKEY;				// Clear WRT bit
		FCTL3 = FWKEY + LOCK;		// Set LOCK bit
	}
	// Write RxFrequency
	if(RxFrequency == 0xFFFFFFFF)
	{
		volatile unsigned long * Flash_ptrB;
		Flash_ptrB = (unsigned long *) &RxFrequency;

		FCTL3 = FWKEY;				// Clear Lock bit
		FCTL1 = FWKEY + ERASE; 		// Set Erase bit
		*Flash_ptrB = 0;			// Dummy write to erase Flash seg
		FCTL1 = FWKEY + BLKWRT;		// Enable long-word write
		*Flash_ptrB = frx;			// Write to flash
		FCTL1 = FWKEY;				// Clear WRT bit
		FCTL3 = FWKEY + LOCK;		// Set LOCK bit
	}
}


#ifdef __MSP430F5438A__
/***************************************************************************//**
 *   @brief      Loads initial LCD buffer and sends buffer to LCD module
 *******************************************************************************/
static void
welcomeLCD(void)
{
    // Clear LCD buffer
    lcdBufferClear(0);

    // TI logo
	lcdSendBuffer(pLcdTiLogo);

	// Project Text
	lcdBufferPrintString(0, "TI/Sigfox", 63, eLcdPage2);
    lcdBufferPrintString(0, "Demo", 63, eLcdPage3);
    lcdBufferPrintString(0, "SDK", 63, eLcdPage5);

    lcdSendBufferPart(0, 63, 127, eLcdPage2, eLcdPage6);
}
#endif

#if defined(PB_KEY) && defined(__MSP430F5438A__)
/***************************************************************************//**
 *   @brief      Updates LCD buffer and sends buffer to LCD module
 *
 *  		   	  +-----------------------------------------+
 *                |_____________  SIGFOX DEMO_______________|
 *                | UL:	  XX    XX    XX    XX    XX    XX  |
 *                |       XX    XX    XX    XX    XX    XX  |
 *                |                                         |
 *                | DL:	  XX    XX    XX    XX    XX    XX  |
 *                |       XX    XX                          |
 *                |_________________________________________|
 *                |			 (c) Texas Instruments			|
 *                +-----------------------------------------+
 *
 *******************************************************************************/
static void
updateLCD(void)
{
	char ulmsg[36] = {0};
	char dlmsg[24] = {0};

	char ulmsg1[18] = {0};
	char ulmsg2[18] = {0};
	char dlmsg1[18] = {0};
	char dlmsg2[6] = {0};

	dataToString((unsigned char*) message, ulmsg, 24);
	dataToString((unsigned char*) ReceivedPayload, dlmsg, 16);

	// Format the converted string to display on LCD
	unsigned char n;
	unsigned char m = 0;
	for(n=0;n<12;n++)
	{
		if(m % 3 == 0)
		{
			ulmsg1[m] = ' ';
			ulmsg2[m] = ' ';
			dlmsg1[m] = ' ';
			if(n<4)
			{
				dlmsg2[m] = ' ';
			}
			m++;
		}
			ulmsg1[m] = ulmsg[n];
			ulmsg2[m] = ulmsg[12+n];
			dlmsg1[m] = dlmsg[n];
			if(n<4)
			{
				dlmsg2[m] = dlmsg[12+n];
			}
			m++;
	}

    // Clear LCD
    lcdBufferClear(0);
    lcdSendBuffer(0);

    // Load status buffer
    lcdBufferPrintStringAligned(0, "Sigfox Demo", eLcdAlignCenter, eLcdPage0);
    lcdBufferSetHLine(0, 0, LCD_COLS-1, 7);
    lcdBufferInvertPage(0, 0, LCD_COLS, eLcdPage0);
    lcdSendBufferPart(0, 0, 127, eLcdPage0,eLcdPage0);

    lcdBufferClearPart(0, 0, 127, eLcdPage1, eLcdPage6);

    lcdBufferPrintString(0, "UL:", 0, eLcdPage1);
    lcdBufferPrintString(0, ulmsg1, 20, eLcdPage1);
    lcdSendBufferPart(0, 0, 127, eLcdPage1,eLcdPage1);

    lcdBufferPrintString(0, ulmsg2, 20, eLcdPage2);
    lcdSendBufferPart(0, 20, 127, eLcdPage2,eLcdPage2);

    lcdBufferClearPart(0, 0, 127, eLcdPage3, eLcdPage6);
    lcdBufferPrintString(0, "DL:", 0, eLcdPage4);
    lcdBufferPrintString(0, dlmsg1, 20, eLcdPage4);
    lcdSendBufferPart(0, 0, 127, eLcdPage4,eLcdPage4);

    lcdBufferPrintString(0, dlmsg2, 20, eLcdPage5);
    lcdSendBufferPart(0, 20, 55, eLcdPage5,eLcdPage5);

    lcdBufferPrintString(0, "(c) Texas Instruments" , 0, eLcdPage7);
    lcdBufferSetHLine(0, 0, LCD_COLS-1, 55);
    lcdBufferInvertPage(0, 0, LCD_COLS, eLcdPage7);

    // Send the buffer to the LCD screen
    lcdSendBufferPart(0, 0, 127, eLcdPage7, eLcdPage7);
}
#endif

/**************************************************************************//**
 * Close the Doxygen group.
 * @}
 ******************************************************************************/
