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
#include "platform.h"
#include "Uart.h"
#include "xil_io.h"
#include "CommonTypes.h"
/**************Preprocessor******************/
/*System level control registers - SLCR base address*/
#define SLCR_BASE_ADDR 0xF8000000
#define UART_RST_CTRL_OFFSET 0x00000228
#define MIO_PIN_48_OFFSET 0x000007C0
#define MIO_PIN_49_OFFSET 0x000007C4
#define UART_CLK_CONTROL_OFFSET 0x00000154

/*UART Controller base addresses*/
#define XUARTPS_INST_ADDR 0xE0001000 // currently designed for uart 1 - soaped

/*Register Address definitions*/
#define XUARTPS_CR_OFFSET 0x00000000
#define XUARTPS_MR_OFFSET 0x00000004
#define XUARTPS_IER_OFFSET 0x00000008
#define XUARTPS_BRGR_OFFSET 0x00000018 // baud rate generator register offset
#define XUARTPS_BRDR_OFFSET 0x00000034 // baud rate divider register offset
#define XUARTPS_RXFIFO_TRIGGER_OFFSET 0x0000044 // RX fifo trigger register offset
#define XUARTPS_RXTOUT_OFFSET 0x0000001C // RX timeout register offset
#define XUARTPS_SR_OFFSET 0x0000002C // Channel status register offset
#define XUARTPS_FIFO_OFFSET 0x00000030 // UART TX/RX FIFO

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

/*Channel Status register bits*/
#define XUARTPS_SR_TNFUL 14
#define XUARTPS_SR_TTRIG 13
#define XUARTPS_SR_FLOWDEL 12
#define XUARTPS_SR_TACTIVE 11
#define XUARTPS_SR_RACTIVE 10
/*important bit positions for tx and rx*/
#define XUARTPS_SR_TXFULL 4 // indicate TX full bit
#define XUARTPS_SR_TXEMPTY 3 // indicate TX empty bit !!!
#define XUARTPS_SR_RXFULL 2 // indicate RX full bit !!!
#define XUARTPS_SR_RXEMPTY 1 // indicate RX empty bit !!!
#define XUARTPS_SR_RXOVR 0 // indicate RX FIFO trigger


/********************************************************************************/

#define UARTPS_MR_DEFAULT (MR_CHMOD | MR_NBSTOP | \
						   MR_PAR | MR_CHRL | MR_CLKSEL)

/**************Definitions******************/
static uartCfgType *sCfgInstance = NULL; // to be set by ConfigureUart Function


/**************Function Prototypes******************/
static void uartRegWrite (RUINT32 addr, RUINT32 value);
static void uartRegRead  (RUINT32 addr, RUINT32 *value);
static void uartReset (void);
static void regWrite (RUINT32 addr, RUINT32 value);
static void regRead(RUINT32 addr, RUINT32 *value);
ReturnType InitializeUartCfg (uartCfgType *cfgInstance);
static ReturnType TxDataPolling (RUINT8 *pu1Data, RUINT32 u4Size);
//what this control register does is that - set bits of this register and then write this data to spesific register - instance config

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
static void regWrite (RUINT32 addr, RUINT32 value){
	RUINT32 *tempAddr = (RUINT32 *)(addr) ; // typecast addr val to pointer
	*tempAddr = value; // set data pointed by tempAddr to input value
	return;
}


/*
 * @param :
 * 	@addr : address of the register that the will be read
 * 	@value : variable that data will be read to
 * @description :
 * 		this function reads register value by the address addr.
 */
static void regRead(RUINT32 addr, RUINT32 *value){
	RUINT32 *tempAddr = (RUINT32 *)(addr); // typecast addr val to pointer
	*value = *tempAddr; // read data and set to pointed by value
	return;
}

/*
 * @param :
 * @ addr : address of the register that the value will be written
 * @ value : value that will be written
 * @ description :
 * 		this function writes value to addr.
 */
static void uartRegWrite (RUINT32 addr, RUINT32 value){
	volatile RUINT32 *tempAddr = (volatile RUINT32 *)(addr + XUARTPS_INST_ADDR); // typecast addr val to pointer
	*tempAddr = value; // set data pointed by tempAddr to input value
	return;
}


/*
 * sets specific bit position to logic true (1)
 */
static void uartSetRegBit (RUINT32 u4RegAddr, RUINT8 u1BitPos)
{
	RUINT32 *pu4TempReg = (RUINT32 *)(u4RegAddr + XUARTPS_INST_ADDR);
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
	RUINT32 *pu4TempReg = (RUINT32 *)(u4RegAddr + XUARTPS_INST_ADDR);
	RUINT32 u4TempValue = ((RUINT32)0x1 << u1BitPos);
	u4TempValue = ~u4TempValue;
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
	RUINT32 *tempAddr = (RUINT32 *)(addr + XUARTPS_INST_ADDR); // typecast addr val to pointer
	*value = *tempAddr; // read data and set to pointed by value
	return;
}

static void uartReset (void){
	RUINT32 UART_0_reset_addr = SLCR_BASE_ADDR + UART_RST_CTRL_OFFSET;
	RUINT32 UART_0_reset_val = 0x07; // 1000 -> uart 1 reference software reset value (bit pos 3)

	regWrite(UART_0_reset_addr, UART_0_reset_val); // assert reset
	usleep(10000); // wait for 10ms
	regWrite(UART_0_reset_addr, 0x00); // deassert reset to disable write protection
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
	RUINT32 MIO49Addr = SLCR_BASE_ADDR + MIO_PIN_48_OFFSET;
	RUINT32 MIO48Addr = SLCR_BASE_ADDR + MIO_PIN_49_OFFSET;
	RUINT32 rxMIOsetDefault = 0x000012E1U;
	RUINT32 txMIOsetDefault = 0x000012E0U;

	regWrite(MIO48Addr, rxMIOsetDefault);
	regWrite(MIO49Addr, txMIOsetDefault);

}

static void configureUartClock(void)
{
	RUINT32 uartClksetDefault = 0x00001402U; // uart 0 disabled - uart 1 enabled
	RUINT32 clkCtrlAddr = SLCR_BASE_ADDR + UART_CLK_CONTROL_OFFSET;

	// configure UART reference clock
	regWrite(clkCtrlAddr, uartClksetDefault); // 50 MHz clock for the UART  - UG585 : p598
}

static void configuraUartCtrlReg(void)
{
	RUINT32 ModeRegAddr = XUARTPS_MR_OFFSET;
	RUINT32 MRConfigVal = 0x00000020U;

	uartRegWrite(ModeRegAddr, MRConfigVal);
}


ReturnType configureUart(uartCfgType cfgInstance){
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
	RUINT32 u4TrialVal;
	RUINT32 CD_115200 = (RUINT32)62U;
	RUINT32 BIDV_115200 = (RUINT32)6U;

	// read lock slcr register and unlock slcr register
	// read slcr status register
	RUINT32 trial;
	regRead(0xF800000C, &trial); // register was 1 stating that slcr registers are write protected !!!
	// write lscr write protection unlock register
	regWrite(0xF8000008, (RUINT32)0xDF0D);
	// read slcr write protection status register again - expectation : reg to be 0
	regRead(0xF800000C, &trial); // register is 0 - write protection disabled !!!

	// reset uart controller
	uartReset();
	// set IO routings - UART 1 - MIO48 RX - MIO49 TX
	setIORouting();
	// configure UART reference clock
	configureUartClock();
	/*Configure Uart control register*/
	//uart character frame - mode register
	configuraUartCtrlReg();


	//control reg, baud rate reg and baud rate divider reg options for baud rate
	// write to three registers : control reg, baud_ge_reg, baud_rate_divider_reg

	//set control register - disable rx and tx paths
	// disable rx path - [RXEN] = 0 and [RXDIS] = 1.
	uartRegRead(XUARTPS_CR_OFFSET, &u4ReadCtrlReg);
	u4TempValue = u4ReadCtrlReg | ((RUINT32)0x01 << XUARTPS_CR_RX_DIS);
	u4TrialVal = 0x01 << XUARTPS_CR_RX_EN;
	u4TrialVal = ~u4TrialVal;
	u4TempValue &= u4TrialVal;
	uartRegWrite(XUARTPS_CR_OFFSET, u4TempValue);
	// disable tx path - [TXEN] = 0 and [TXDIS] = 1.
	uartRegRead(XUARTPS_CR_OFFSET, &u4ReadCtrlReg);
	u4TempValue = u4ReadCtrlReg | ((RUINT32)0x01 << XUARTPS_CR_TX_DIS);
	u4TrialVal = 0x01 << XUARTPS_CR_TX_EN;
	u4TrialVal = ~u4TrialVal;
	u4TempValue &= u4TrialVal;
	uartRegWrite(XUARTPS_CR_OFFSET, u4TempValue);

	// write calculated CD value to baud rate generator register - CD bit field
	uartRegWrite(XUARTPS_BRGR_OFFSET, CD_115200);
	// write BIDV value to baud rate divider register
	uartRegWrite(XUARTPS_BRDR_OFFSET, BIDV_115200);

	// Reset TX and RX paths
	uartRegRead(XUARTPS_CR_OFFSET, &u4ReadCtrlReg);
	u4TempValue = (RUINT32)u4ReadCtrlReg | ((RUINT32)0x01 << XUARTPS_CR_TXRST) | ((RUINT32)0x01 << XUARTPS_CR_RXRST);
	uartRegWrite(XUARTPS_CR_OFFSET, u4TempValue); // reset TX and RX paths

	//enable TX and RX paths
	//enable rx path - [RXEN] = 1 and [RXDIS] = 0.
	uartRegRead(XUARTPS_CR_OFFSET, &u4ReadCtrlReg);
	u4TempValue = u4ReadCtrlReg | ((RUINT32)0x01 << XUARTPS_CR_RX_EN);
	u4TrialVal = 0x01 << XUARTPS_CR_RX_DIS;
	u4TrialVal = ~u4TrialVal;
	u4TempValue &= u4TrialVal;
	uartRegWrite(XUARTPS_CR_OFFSET, u4TempValue);
	//enable tx path - [TXEN] = 1 and [TXDIS] = 0.
	uartRegRead(XUARTPS_CR_OFFSET, &u4ReadCtrlReg);
	u4TempValue = u4ReadCtrlReg | ((RUINT32)0x01 << XUARTPS_CR_TX_EN);
	u4TrialVal = 0x01 << XUARTPS_CR_TX_DIS;
	u4TrialVal = ~u4TrialVal;
	u4TempValue &= u4TrialVal;
	uartRegWrite(XUARTPS_CR_OFFSET, u4TempValue);

	// Reset TX and RX paths
	uartRegRead(XUARTPS_CR_OFFSET, &u4ReadCtrlReg);
	u4TempValue = (RUINT32)u4ReadCtrlReg | ((RUINT32)0x01 << XUARTPS_CR_TXRST) | ((RUINT32)0x01 << XUARTPS_CR_RXRST);
	uartRegWrite(XUARTPS_CR_OFFSET, u4TempValue); // reset TX and RX paths

	// set RxFIFO levels - 32
	uartRegWrite(XUARTPS_RXFIFO_TRIGGER_OFFSET, (RUINT32)0x20);

	//Enable Controller - Write 0x00000117 to control registers - this value coming from datasheet. TODO : make it configurable
	uartRegWrite(XUARTPS_CR_OFFSET, 0x00000117); // controller enabled

	// disable program receive timeout mechanism - write 0 to RSTTO bit field
	uartRegWrite(XUARTPS_RXTOUT_OFFSET, 0x00); // receiver timeout disabled

	//READY TO COMMUNICATE !!!
	// SO FAR THIS IS DIRECT SOAPING. EVERYTHING IS DIRECTLY SET. TODO : MAKE IT CONFUGIRABLE

	// first trial to send data
	RUINT8 a1TrialArray[] = "bugra.erbas";
	TxDataPolling(a1TrialArray, sizeof(a1TrialArray));
}

// returns success(0) in case of TX fifo is empty, if not, returns failure (1)
static ReturnType isTxFifoEmpty(void)
{
	RUINT32 u4RetVal = 0U;
	RUINT32 u4ReadTxFifo = 0U;
	RUINT32 u4FifoEmptyMask = 0x01 << XUARTPS_SR_TXEMPTY;
	RUINT32 u4Temp;
	uartRegRead(XUARTPS_SR_OFFSET, &u4ReadTxFifo);
	u4Temp = u4ReadTxFifo & u4FifoEmptyMask; // bitwise and for masking operation
	// fifo needs to be empty to transmit data thus, return 1 if empty
	if(u4Temp == u4FifoEmptyMask)
	{
		u4RetVal = 1U;
	}

	return (ReturnType)u4RetVal;
}

/*
 * pu1Data : pointer that holds data that will be transmitted
 * u4size	 : size of bytes that will be transmitted
 */
static ReturnType TxDataPolling (RUINT8 *pu1Data, RUINT32 u4Size)
{
	RUINT8 u1TempValue;
	RUINT32 u4TempAddr;
	RUINT32 u4Index = 0;

	u4TempAddr = pu1Data;

	for(; u4Index < u4Size; u4Index++)
	{
		u1TempValue = *(RUINT8 *)(u4TempAddr + u4Index);
		/*procedure :
		 * wait for tx fifo to be empty (only than new byte can be send)
		 * fill the tx fifo with data
		 * write new data if tx fifo is empty until number of transmissions is done */

		while(!isTxFifoEmpty()); // wait for tx fifo to be empty
		// fill TX fifo with 1 Byte of pu1Data with
		uartRegWrite(XUARTPS_FIFO_OFFSET,u1TempValue);
	}

}



