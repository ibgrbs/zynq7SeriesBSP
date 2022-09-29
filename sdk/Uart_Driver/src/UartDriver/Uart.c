/*
 * Uart.c
 *
 *  Created on: Aug 28, 2022
 *      Author: bugra's PC
 */


/*
 * This is Uart driver that would initialize PS uart controller
 * Document Structure :
 * Basic setter - getter functions
 * Basic register write / read operations
 * Hardware related register map : Map includes:
 * 	Base addresses of registers
 * 	Offsets of register to address
 * 	Default values of register
 * 	BitMasks to read or write spesific bits in various registers
 * Driver usage manual
 * Driver related type definitions and Structures
 * Initialization, Configurations and basic use case
 * internal Built in tests (BIT) such as loopback tests etc.
 * TODO-1 : This is currently compatible with PS uart blocks (Uart0 and Uart1),
 * Update source code to be compatible with UartLite (PL - Uart - MIO/EMIO)
 * TODO-2 : Update source code the be compatible with User Defined - Uart blocks so that it can be used with any
 * PL Design
 */

/**************Includes******************/
/*TODO : currently file inclusions are according to folder positions
 * project is being compiled with sdk provided makefile which causes folder locations being
 * crucial. Create a makefile document which would compile and include folder locations
 * so that location of folder when included is not important. Compile project from
 * user makefile.
 */
#include <stdio.h>
#include "../platform.h"
#include "Uart.h"
#include "../CommonTypes.h"
/**************Preprocessor******************/
/*System level control registers - SLCR base address*/
#define SLCR_BASE_ADDR 0xF8000000
#define UART_RST_CTRL_OFFSET 0x00000228
#define MIO_PIN_46_OFFSET 0x000007B8
#define MIO_PIN_47_OFFSET 0x000007BC
#define UART_CLK_CONTROL_OFFSET 0x00000154

/*UART Controller base addresses*/
#define XUARTPS_INST0_ADDR 0xE0000000
#define XUARTPS_INST1_ADDR 0xE0001000

/*Register Address definitions*/
#define XUARTPS_CR_OFFSET 0x00000000
#define XUARTPS_MR_OFFSET 0x00000004
#define XUARTPS_IER_OFFSET 0x00000008
#define XUARTPS_BRGR_OFFSET 0x00000018 // baud rate generator register offset
#define XUARTPS_BRDR_OFFSET 0x00000034 // baud rate divider register offset
#define XUARTPS_RXFIFO_TRIGGER_OFFSET 0x0000044 // RX fifo trigger register offset
#define XUARTPS_RXTOUT_OFFSET 0x0000001C // RX timeout register offset

//TODO do the rest
/*Control register bits positions*/
#define XUARTPS_CR_STOPBRK 8
#define XUARTPS_CR_STARTBRK 7
#define XUARTPS_CR_TORST 6
#define XUARTPS_CR_TX_DIS 5
#define XUARTPS_CR_TX_EN 4
#define XUARTPS_CR_RX_DIS 3
#define XUARTPS_CR_RX_EN 2
#define XUARTPS_CR_TXRST 1
#define XUARTPS_CR_RXRST 0

/*Mode register bits*/
#define MR_CHMOD 0b00 << 9
#define MR_NBSTOP 0b00 << 7
#define MR_PAR 0b100 << 5
#define MR_CHRL 0b00 << 2
#define MR_CLKSEL 0b0 << 0



#define UARTPS_MR_DEFAULT (MR_CHMOD | MR_NBSTOP | \
						   MR_PAR | MR_CHRL | MR_CLKSEL)

/**************Definitions******************/
static uartCfgType *sCfgInstance = NULL; // to be set by ConfigureUart Function


/**************Function Prototypes******************/
static void uartRegWrite (RUINT32 addr, RUINT32 value);
static void uartRegRead  (RUINT32 addr, RUINT32 *value);
static void uartReset (void);
ReturnType InitializeUartCfg (uartCfgType *cfgInstance);
//what this control register does is that - set bits of this register and then write this data to spesific register - instance config
static ReturnType ConfigureUart (uartCfgType cfgInstance);
static ReturnType uartTx (RUINT32 *TxBuffer, RUINT32 size);
static ReturnType uartRx (RUINT32 *RxBuffer, RUINT32 size);
/**************Function Prototypes******************/

/*
 * @param :
 * @ addr : address of the register that the value will be written
 * @ value : value that will be written
 * @ description :
 * 		this function writes value to addr.
 */
static void uartRegWrite (RUINT32 addr, RUINT32 value){
	RUINT32 *tempAddr = (RUINT32 *)(addr + XUARTPS_INST0_ADDR) ; // typecast addr val to pointer
	*tempAddr = value; // set data pointed by tempAddr to input value
	return;
}

/*
 * sets specific bit position to logic true (1)
 */
static void uartSetRegBit (RUINT32 u4RegAddr, RUINT8 u1BitPos)
{
	RUINT32 *pu4TempReg = (RUINT32 *)(u4RegAddr + XUARTPS_INST0_ADDR);
	RUINT32 u4TempValue = ((RUINT32)0x01U << u1BitPos);
	if(u1BitPos < 32)
	{
		// sets specific bit to 1
		*pu4TempReg |= u4TempValue;
	}
}

/*
 * sets specific bit position to logic false (0)
 */
static void uartClearRegBit (RUINT32 u4RegAddr, RUINT8 u1BitPos)
{
	RUINT32 *pu4TempReg = (RUINT32 *)(u4RegAddr + XUARTPS_INST0_ADDR);
	RUINT32 u4TempValue = ((RUINT32)0xFFFFFFFEU << u1BitPos);
	if(u1BitPos < 32)
	{
		// sets specific bit to 1
		*pu4TempReg &= u4TempValue;
	}
}

/*
 * @param :
 * 	@addr : address of the register that the will be read
 * 	@value : variable that data will be read to
 * @description :
 * 		this function reads register value by the address addr.
 */
static void uartRegRead (RUINT32 addr, RUINT32 *value){
	RUINT32 *tempAddr = (RUINT32 *)(addr + XUARTPS_INST0_ADDR); // typecast addr val to pointer
	*value = *tempAddr; // read data and set to pointed by value
	return;
}

static void uartReset (void){
	RUINT32 UART_0_reset_addr = SLCR_BASE_ADDR + UART_RST_CTRL_OFFSET;
	RUINT32 UART_0_reset_val = 0x04; // 0100

	uartRegWrite(UART_0_reset_addr, UART_0_reset_val);
}
/*
 * @param :
 * 	@cfgInstance : input instance that will be used to configure uart controller
 * @ description : cfg has controller register value, baudrate and parity information to configure uart controller
 * currently this is soaped - works only for uart instance 0
 * TODO : after uart0 operations are verified to be working correctly - update : uart 0 and 1 can be configured at any time
 */
ReturnType initializeUartCfg (uartCfgType *cfgInstance){
	sCfgInstance = cfgInstance;
	/* input config is connected to static driver config so that changing from outside of driver will affect statically*/
}

static void setIORouting(void)
{
	RUINT32 MIO46Addr = SLCR_BASE_ADDR + MIO_PIN_46_OFFSET;
	RUINT32 MIO47Addr = SLCR_BASE_ADDR + MIO_PIN_47_OFFSET;
	RUINT32 rxMIOsetDefault = 0x000012E1U;
	RUINT32 txMIOsetDefault = 0x000012E0U;

	uartRegWrite(MIO46Addr, rxMIOsetDefault);
	uartRegWrite(MIO47Addr, txMIOsetDefault);

}

static void configureUartClock(void)
{
	RUINT32 uartClksetDefault = 0x00001401U;
	RUINT32 clkCtrlAddr = SLCR_BASE_ADDR + UART_CLK_CONTROL_OFFSET;

	// configure UART reference clock
	uartRegWrite(clkCtrlAddr, uartClksetDefault); // 50 MHz clock for the UART  - UG585 : p598
}

static void configuraUartCtrlReg(void)
{
	RUINT32 ModeRegAddr = SLCR_BASE_ADDR + XUARTPS_MR_OFFSET;

	uartRegWrite(ModeRegAddr, UARTPS_MR_DEFAULT);
}


static ReturnType configureUart (uartCfgType cfgInstance){
	/*
	 * procedure :
	 *		1 - reset controller from SLCR register. there is a spesific uart_reset_ctrl register in SLCR register
	 *		2 - configure I/O signal routing : can be routed either MIO or EMIO (multiplexed IO or extended Multiplexed IO)
	 *		from UG585 19.5.1 : MIO programming -> basically from SLCR register (system level control register - the register that
	 *		controls general specifications of zynq architecture - controls group of modules or operations between modules.
	 *		General structure is that module registers are for controlling modules within zynq. SLCR is to configure and control system
	 *		instead of modules which are sub systems of zynq system)
	 *		3 - Configure UART reference clock : from UG585 19.4.1 clocks
	 *		4 - Configure controller functions : set uart controller register
	 *		5 - Configure Interrupts for interrupt driven comm - can be neglected in case of polling.
	 *			5.1 - Configure modem control - polling and interrupt driven comm.
	 *		6 - Now ready to manage communications - transver and receive message via shift register, Tx , Rx registers and
	 *			control registers spesific flags regarding communications
	 *
	 */

	// TODO : this sets to 115200, make it configurable

	RUINT32 u4ReadCtrlReg;
	RUINT32 u4TempValue;
	RUINT32 CD_115200 = (RUINT32)62U;
	RUINT32 BIDV_115200 = (RUINT32)6U;

	// reset uart controller
	uartReset();
	// set IO routings - UART 0 - MIO46 RX - MIO47 T
	setIORouting();
	// configure UART reference clock
	configureUartClock();
	/*Configure Uart control register*/
	//uart character frame - mode register
	configuraUartCtrlReg();
	//control reg, baud rate reg and baud rate divider reg options for baud rate


	// write to three registers : control reg, baud_ge_reg, baud_rate_divider_reg

	//set control register - disable rx and tx paths
	uartRegRead(XUARTPS_CR_OFFSET, &u4ReadCtrlReg);

	uartClearRegBit(XUARTPS_CR_OFFSET, XUARTPS_CR_TX_EN);// set 0 to TX enable
	uartClearRegBit(XUARTPS_CR_OFFSET, XUARTPS_CR_RX_EN);// set 0 to RX enable
	u4TempValue = u4ReadCtrlReg | ((RUINT32)0x01 << XUARTPS_CR_TX_DIS) | ((RUINT32)0x01 << XUARTPS_CR_RX_DIS);
	uartRegWrite(XUARTPS_CR_OFFSET, u4TempValue); // enable RX and TX disables

	// write calculated CD value to baud rate generator register - CD bit field
	uartRegWrite(XUARTPS_BRGR_OFFSET, CD_115200);
	// write BIDV value to baud rate divider register
	uartRegWrite(XUARTPS_BRDR_OFFSET, BIDV_115200);

	// Reset TX and RX paths
	uartRegRead(XUARTPS_CR_OFFSET, &u4ReadCtrlReg);
	u4TempValue = (RUINT32)u4ReadCtrlReg | ((RUINT32)0x01 << XUARTPS_CR_TXRST) | ((RUINT32)0x01 << XUARTPS_CR_RXRST);
	uartRegWrite(XUARTPS_CR_OFFSET, u4TempValue); // reset TX and RX paths

	//enable TX and RX paths
	uartSetRegBit(XUARTPS_CR_OFFSET, XUARTPS_CR_TX_EN);// set 1 to TX enable
	uartSetRegBit(XUARTPS_CR_OFFSET, XUARTPS_CR_RX_EN);// set 1 to RX enable
	uartClearRegBit(XUARTPS_CR_OFFSET, XUARTPS_CR_TX_EN);// set 0 to TX disable
	uartClearRegBit(XUARTPS_CR_OFFSET, XUARTPS_CR_RX_EN);// set 0 to RX disable

	// set RxFIFO levels - disable
	uartRegWrite(XUARTPS_RXFIFO_TRIGGER_OFFSET, (RUINT32)0x00);

	//Enable Controller - Write 0x00000117 to control registers - this value comming from datasheet. TODO : make it configurable
	uartRegWrite(XUARTPS_CR_OFFSET, 0x00000117); // controller enabled

	// disable program receive timeout mechanism - write 0 to RSTTO bit field
	uartRegWrite(XUARTPS_RXTOUT_OFFSET, 0x00); // receiver timeout disabled

	//READY TO COMMUNICATE !!!
	// SO FAR THIS IS DIRECT SOAPING. EVERYTHING IS DIRECTLY SET. TODO : MAKE IT CONFUGIRABLE

}

static ReturnType isTxFifoEmpty(void)
{
	RUINT32 u4ReadTxFifo;

}

/*
 * pu1Data : pointer that holds data that will be transmitted
 * u4size	 : size of bytes that will be transmitted
 */
static ReturnType TXData (RUINT8 *pu1Data, RUINT32 u4Size)
{
	/*procedure :
	 * wait for tx fifo to be empty (only than new byte can be send)
	 * fill the tx fifo with data
	 * write new data if tx fifo is empty until number of transmissions is done */
}



