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
#define MIO_PIN_46_OFFSET 0x000007B4
#define MIO_PIN_47_OFFSET 0x000007B8
#define MIO_PIN_48_OFFSET 0x000007C0
#define MIO_PIN_49_OFFSET 0x000007C4
#define UART_CLK_CONTROL_OFFSET 0x00000154

/*Interrupt Enable/Disable registers bit offsets*/
#define TOVR 12
#define TNFUL 11
#define TTRIG 10
#define XUARTPS_IXR_DMS 9
#define XUARTPS_IXR_TOUT 8
#define XUARTPS_IXR_PARITY 7
#define XUARTPS_IXR_FRAMING 6
#define XUARTPS_IXR_OVER 5
#define XUARTPS_IXR_TXFULL 4
#define XUARTPS_IXR_TXEMPTY 3
#define XUARTPS_IXR_RXFULL 2
#define XUARTPS_IXR_RXEMPTY 1
#define XUARTPS_IXR_RXOVR 0

/*UART Controller base addresses*/
#define XUARTPS_BASE_ADDR 0xE0000000 // UART devices base addresses without device specification
#define UARTPS_DEVICE_ADDR_CONST 0x1000 // uart device address specification address

/*Register Address definitions*/
#define XUARTPS_CR_OFFSET 0x00000000
#define XUARTPS_MR_OFFSET 0x00000004
#define XUARTPS_IER_OFFSET 0x00000008
#define XUARTPS_IDR_OFFSET 0x0000000C
#define XUARTPS_ISR_OFFSET 0x00000014
#define XUARTPS_IMR_OFFSET 0x00000010
#define XUARTPS_BRGR_OFFSET 0x00000018 // baud rate generator register offset
#define XUARTPS_BRDR_OFFSET 0x00000034 // baud rate divider register offset
#define XUARTPS_RXFIFO_TRIGGER_OFFSET 0x0000044 // RX fifo trigger register offset
#define XUARTPS_RXTOUT_OFFSET 0x0000001C // RX timeout register offset
#define XUARTPS_SR_OFFSET 0x0000002C // Channel status register offset
#define XUARTPS_FIFO_OFFSET 0x00000030 // UART TX/RX FIFO
#define XUARTPS_RXWM_OFFSET 0x00000020 // UART RX FIFO trigger level register

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
static uartCfgType sCfgInstance_Device0; // to be set by ConfigureUart Function
static uartCfgType sCfgInstance_Device1; // to be set by ConfigureUart Function

/**************Function Prototypes******************/
static void uartRegWrite(RUINT32 addr, RUINT32 value, uartCfgType *Instance);
static void uartRegRead(RUINT32 addr, RUINT32 *value, uartCfgType *Instance);
static void uartReset(uartCfgType *Instance);
static void regWrite(RUINT32 addr, RUINT32 value);
static void regRead(RUINT32 addr, RUINT32 *value);
ReturnType InitializeUartCfg(uartCfgType *cfgInstance);
static ReturnType TxDataPolling(RUINT8 *pu1Data, RUINT32 u4Size, uartCfgType *pCfgInstance);
static ReturnType RxDataPolling(RUINT8 *pu1Data, RUINT32 u4Size, uartCfgType *pCfgInstance);
static ReturnType isTxFifoEmpty(uartCfgType *pCfgInstance);
static ReturnType isRxFifoFull(uartCfgType *pCfgInstance);
//what this control register does is that - set bits of this register and then write this data to spesific register - instance config
static ReturnType uartTx(RUINT32 *TxBuffer, RUINT32 size);
static ReturnType uartRx(RUINT32 *RxBuffer, RUINT32 size);
static ReturnType enableInterrupt(uartCfgType *pCfgInstance);


RUINT8 a1UartRxArray[1000];

/*
 * @param :
 * @ addr : address of the register that the value will be written
 * @ value : value that will be written
 * @ description :
 * 		this function writes value to addr.
 */
static void regWrite (RUINT32 addr, RUINT32 value)
{
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
static void regRead(RUINT32 addr, RUINT32 *value)
{
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
static void uartRegWrite (RUINT32 addr, RUINT32 value, uartCfgType *Instance)
{

	// typecast addr val to pointer
	volatile RUINT32 *tempAddr = (volatile RUINT32 *)(addr + XUARTPS_BASE_ADDR + (UARTPS_DEVICE_ADDR_CONST * Instance->DeviceNum));
	*tempAddr = value; // set data pointed by tempAddr to input value
	return;
}

/*
 * @param :
 * 	@addr : address of the register that the will be read from
 * 	@value : variable that data will be read and written to
 * @description :
 * 		this function reads register value by the address addr.
 */
static void uartRegRead(RUINT32 addr, RUINT32 *value, uartCfgType *Instance)
{
	RUINT32 *tempAddr = (RUINT32 *)(addr + XUARTPS_BASE_ADDR + (UARTPS_DEVICE_ADDR_CONST * Instance->DeviceNum)); // typecast addr val to pointer
	*value = *tempAddr; // read data and set to pointed by value
	return;
}

/*
 * sets specific bit position to logic true (1)
 */
static void uartSetRegBit (RUINT32 u4RegAddr, RUINT8 u1BitPos, uartCfgType *Instance)
{
	RUINT32 *pu4TempReg = (RUINT32 *)(u4RegAddr + XUARTPS_BASE_ADDR + (UARTPS_DEVICE_ADDR_CONST * Instance->DeviceNum));
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
static void uartClearRegBit (RUINT32 u4RegAddr, RUINT8 u1BitPos, uartCfgType *Instance)
{
	RUINT32 *pu4TempReg = (RUINT32 *)(u4RegAddr + XUARTPS_BASE_ADDR + (UARTPS_DEVICE_ADDR_CONST * Instance->DeviceNum));
	RUINT32 u4TempValue = ((RUINT32)0x1 << u1BitPos);
	u4TempValue = ~u4TempValue;
	if(u1BitPos < 32)
	{
		// sets specific bit to 1
		*pu4TempReg &= u4TempValue;
	}
}

static void uartReset (uartCfgType *Instance)
{

	RUINT32 UART_reset_addr = SLCR_BASE_ADDR + UART_RST_CTRL_OFFSET;
	RUINT32 UART_reset_val = 1U;
	RUINT32 UART_base_shifter = 2;
	RUINT32 u4TempValue = 0;
	RUINT32 u4Mask = 0x01;
	UART_reset_val = UART_reset_val << (UART_base_shifter + Instance->DeviceNum);

	regWrite(UART_reset_addr, UART_reset_val); // assert reset
	usleep(10000); // wait for 10ms
	regRead(UART_reset_addr, &u4TempValue);
	u4Mask = u4Mask << (UART_base_shifter + Instance->DeviceNum);
	u4Mask = ~u4Mask;
	u4TempValue &= u4Mask;
	regWrite(UART_reset_addr, u4TempValue); // deassert reset to disable write protection
	// safe version regWrite(UART_reset_addr, 0x00);
}

/*
 * @param :
 * 	@cfgInstance : input instance that will be used to configure uart controller
 * @ description : cfg has controller register value, baudrate and parity information to configure uart controller
 * currently this is soaped - works only for uart instance 0
 * TODO : after uart0 operations are verified to be working correctly - update : uart 0 and 1 can be configured at any time
 */

ReturnType initializeUartCfg (uartCfgType *cfgInstance)
{
	ReturnType retVal = XST_FAILURE;
	if(cfgInstance->DeviceNum == UART_INSTANCE_DEVICE_0)
	{
		sCfgInstance_Device0 = *cfgInstance;
		retVal = XST_SUCCESS;
	}
	else if(cfgInstance->DeviceNum == UART_INSTANCE_DEVICE_1)
	{
		sCfgInstance_Device1 = *cfgInstance;
		retVal = XST_SUCCESS;
	}

	return retVal;
	/* input config is connected to static driver config so that changing from outside of driver will affect statically*/
}

static void setIORouting(uartCfgType *cfgInstance)
{
	RUINT32 MIOTxAddr;
	RUINT32 MIORxAddr;
	RUINT32 rxMIOsetDefault = 0x000012E1U;
	RUINT32 txMIOsetDefault = 0x000012E0U;

	if((cfgInstance != NULL) && (cfgInstance->DeviceNum == UART_INSTANCE_DEVICE_1))
	{
		MIOTxAddr = SLCR_BASE_ADDR + MIO_PIN_48_OFFSET;
		MIORxAddr = SLCR_BASE_ADDR + MIO_PIN_49_OFFSET;
	}
	else if((cfgInstance != NULL) && (cfgInstance->DeviceNum == UART_INSTANCE_DEVICE_0))
	{
		MIOTxAddr = SLCR_BASE_ADDR + MIO_PIN_47_OFFSET;
		MIORxAddr = SLCR_BASE_ADDR + MIO_PIN_46_OFFSET;
	}
	else
	{
		// failure
		return;
	}

	regWrite(MIORxAddr, rxMIOsetDefault);
	regWrite(MIOTxAddr, txMIOsetDefault);

}

static void configureUartClock(uartCfgType *cfgInstance)
{
	RUINT32 u4TempVal = 1U;
	u4TempVal = u4TempVal << cfgInstance->DeviceNum ;
	RUINT32 uartClksetDefault = 0x00001400U; // uart 0 disabled - uart 0 enabled
	uartClksetDefault |= u4TempVal;
	RUINT32 clkCtrlAddr = SLCR_BASE_ADDR + UART_CLK_CONTROL_OFFSET;

	// configure UART reference clock
	regWrite(clkCtrlAddr, uartClksetDefault); // 50 MHz clock for the UART  - UG585 : p598
}

static void configuraUartCtrlReg(uartCfgType *cfgInstance)
{
	RUINT32 ModeRegAddr = XUARTPS_MR_OFFSET;
	RUINT32 MRConfigVal = 0x00000020U;
	if(cfgInstance != NULL)
	{
		uartRegWrite(ModeRegAddr, MRConfigVal, cfgInstance);
	}

}

static void RxTxPathControl(uartCfgType *pCfgInstance, uartStatusType option)
{
	RUINT32 u4ReadCtrlReg;
	RUINT32 u4TempValue;
	RUINT32 u4TrialVal;

	if(option == UART_ENABLE)
	{
		//enable TX and RX paths
		//enable rx path - [RXEN] = 1 and [RXDIS] = 0.
		uartRegRead(XUARTPS_CR_OFFSET, &u4ReadCtrlReg, pCfgInstance);
		u4TempValue = u4ReadCtrlReg | ((RUINT32)0x01 << XUARTPS_CR_RX_EN);
		u4TrialVal = 0x01 << XUARTPS_CR_RX_DIS;
		u4TrialVal = ~u4TrialVal;
		u4TempValue &= u4TrialVal;
		uartRegWrite(XUARTPS_CR_OFFSET, u4TempValue, pCfgInstance);
		//enable tx path - [TXEN] = 1 and [TXDIS] = 0.
		uartRegRead(XUARTPS_CR_OFFSET, &u4ReadCtrlReg, pCfgInstance);
		u4TempValue = u4ReadCtrlReg | ((RUINT32)0x01 << XUARTPS_CR_TX_EN);
		u4TrialVal = 0x01 << XUARTPS_CR_TX_DIS;
		u4TrialVal = ~u4TrialVal;
		u4TempValue &= u4TrialVal;
		uartRegWrite(XUARTPS_CR_OFFSET, u4TempValue, pCfgInstance);
	}
	else if(option == UART_DISABLE)
	{
		//set control register - disable rx and tx paths
		// disable rx path - [RXEN] = 0 and [RXDIS] = 1.
		uartRegRead(XUARTPS_CR_OFFSET, &u4ReadCtrlReg, pCfgInstance);
		u4TempValue = u4ReadCtrlReg | ((RUINT32)0x01 << XUARTPS_CR_RX_DIS);
		u4TrialVal = 0x01 << XUARTPS_CR_RX_EN;
		u4TrialVal = ~u4TrialVal;
		u4TempValue &= u4TrialVal;
		uartRegWrite(XUARTPS_CR_OFFSET, u4TempValue, pCfgInstance);
		// disable tx path - [TXEN] = 0 and [TXDIS] = 1.
		uartRegRead(XUARTPS_CR_OFFSET, &u4ReadCtrlReg, pCfgInstance);
		u4TempValue = u4ReadCtrlReg | ((RUINT32)0x01 << XUARTPS_CR_TX_DIS);
		u4TrialVal = 0x01 << XUARTPS_CR_TX_EN;
		u4TrialVal = ~u4TrialVal;
		u4TempValue &= u4TrialVal;
		uartRegWrite(XUARTPS_CR_OFFSET, u4TempValue, pCfgInstance);
	}
	else
	{
		// failure
	}

}


ReturnType InitializeUart(uartCfgType *pCfgInstance)
{
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
	RUINT32 trial;

	uartCfgType* pTempCfgInstance;

	if(pCfgInstance->DeviceNum == UART_INSTANCE_DEVICE_1){
		pTempCfgInstance = &sCfgInstance_Device1;
	}
	else if(pCfgInstance->DeviceNum == UART_INSTANCE_DEVICE_0)
	{
		pTempCfgInstance = &sCfgInstance_Device0;
	}
	else{
		// failure
		return XST_FAILURE;
	}
	initializeUartCfg (pTempCfgInstance);

	// read lock slcr register and unlock slcr register
	// read slcr status register
	regRead(0xF800000C, &trial); // register was 1 stating that slcr registers are write protected !!!
	// write lscr write protection unlock register
	regWrite(0xF8000008, (RUINT32)0xDF0D);
	// read slcr write protection status register again - expectation : reg to be 0
	regRead(0xF800000C, &trial); // register is 0 - write protection disabled !!!

	// reset uart controller
	uartReset(pTempCfgInstance);
	// set IO routings - UART 1 - MIO48 RX - MIO49 TX
	setIORouting(pTempCfgInstance);
	// configure UART reference clock
	configureUartClock(pTempCfgInstance);
	/*Configure Uart control register*/
	//uart character frame - mode register
	configuraUartCtrlReg(pTempCfgInstance);

	//control reg, baud rate reg and baud rate divider reg options for baud rate
	// write to three registers : control reg, baud_ge_reg, baud_rate_divider_reg

	RxTxPathControl(pTempCfgInstance, UART_DISABLE);

	// write calculated CD value to baud rate generator register - CD bit field
	uartRegWrite(XUARTPS_BRGR_OFFSET, CD_115200, pTempCfgInstance);
	// write BIDV value to baud rate divider register
	uartRegWrite(XUARTPS_BRDR_OFFSET, BIDV_115200, pTempCfgInstance);

	// Reset TX and RX paths
	uartRegRead(XUARTPS_CR_OFFSET, &u4ReadCtrlReg, pTempCfgInstance);
	u4TempValue = (RUINT32)u4ReadCtrlReg | ((RUINT32)0x01 << XUARTPS_CR_TXRST) | ((RUINT32)0x01 << XUARTPS_CR_RXRST);
	uartRegWrite(XUARTPS_CR_OFFSET, u4TempValue, pTempCfgInstance); // reset TX and RX paths

	RxTxPathControl(pCfgInstance, UART_ENABLE);

	// Reset TX and RX paths
	uartRegRead(XUARTPS_CR_OFFSET, &u4ReadCtrlReg, pTempCfgInstance);
	u4TempValue = (RUINT32)u4ReadCtrlReg | ((RUINT32)0x01 << XUARTPS_CR_TXRST) | ((RUINT32)0x01 << XUARTPS_CR_RXRST);
	uartRegWrite(XUARTPS_CR_OFFSET, u4TempValue, pTempCfgInstance); // reset TX and RX paths

	// set RxFIFO levels - 32
	uartRegWrite(XUARTPS_RXFIFO_TRIGGER_OFFSET, (RUINT32)0x01, pTempCfgInstance); // was 32 bytes (0x20)

	//Enable Controller - Write 0x00000117 to control registers - this value coming from datasheet. TODO : make it configurable
	uartRegWrite(XUARTPS_CR_OFFSET, 0x00000117, pTempCfgInstance); // controller enabled

	// disable program receive timeout mechanism - write 0 to RSTTO bit field
	uartRegWrite(XUARTPS_RXTOUT_OFFSET, 0x00, pTempCfgInstance); // receiver timeout disabled

	//READY TO COMMUNICATE !!!
	// SO FAR THIS IS DIRECT SOAPING. EVERYTHING IS DIRECTLY SET. TODO : MAKE IT CONFUGIRABLE

	// first trial to send data
	RUINT8 a1TrialArray[] = "bugra.erbas";
	TxDataPolling(a1TrialArray, sizeof(a1TrialArray), pTempCfgInstance);

	enableInterrupt(pCfgInstance);

	return XST_SUCCESS;
}

// returns success(0) in case of TX fifo is empty, if not, returns failure (1)
static ReturnType isTxFifoEmpty(uartCfgType *pCfgInstance)
{
	RUINT32 u4RetVal = 0U;
	RUINT32 u4ReadTxFifo = 0U;
	RUINT32 u4FifoEmptyMask = 0x01 << XUARTPS_SR_TXEMPTY;
	RUINT32 u4Temp;
	uartRegRead(XUARTPS_SR_OFFSET, &u4ReadTxFifo, pCfgInstance);
	u4Temp = u4ReadTxFifo & u4FifoEmptyMask; // bitwise and for masking operation
	// fifo needs to be empty to transmit data thus, return 1 if empty
	if(u4Temp == u4FifoEmptyMask)
	{
		u4RetVal = 1U;
	}

	return (ReturnType)u4RetVal;
}

// returns success(0) in case of TX fifo is not empty, if empty returns 1
static ReturnType isRxFifoEmpty(uartCfgType *pCfgInstance)
{
	RUINT32 u4RetVal = 0U;
	RUINT32 u4ReadRxFifo = 0U;
	RUINT32 u4FifoRxFullMask = 0x01 << XUARTPS_SR_RXEMPTY;
	RUINT32 u4Temp;

	uartRegRead(XUARTPS_SR_OFFSET, &u4ReadRxFifo, pCfgInstance);
	u4Temp = u4ReadRxFifo & u4FifoRxFullMask; // bitwise and for masking operation
	if(u4Temp == u4FifoRxFullMask)
	{
		u4RetVal = 1U; // Rx fifo is empty
	}

	return (ReturnType)u4RetVal;
}

/*
 * pu1Data : pointer that holds data that will be transmitted
 * u4size	 : size of bytes that will be transmitted
 */
static ReturnType TxDataPolling(RUINT8 *pu1Data, RUINT32 u4Size, uartCfgType *pCfgInstance)
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

		while(!isTxFifoEmpty(pCfgInstance)); // wait for tx fifo to be empty
		// fill TX fifo with 1 Byte of pu1Data with
		uartRegWrite(XUARTPS_FIFO_OFFSET, u1TempValue, pCfgInstance);
	}

}

/*
 * pu1Data : pointer that received values will be written to
 * u4size	: size of bytes to be received
 * pCfgInstance : pointer to the uart device instance hence there are two different instances
 */
static ReturnType RxDataPolling(RUINT8 *pu1Data, RUINT32 u4Size, uartCfgType *pCfgInstance)
{
	RUINT8* u1TempValue;
	RUINT32 u4TempAddr;
	RUINT32 u4Index = 0;

	/*
	 * wait RxFIFO to fill up to the trigger level
	 * read data from Rx FIFO
	 * repeat until required size is read
	 * TODO : clear if timeout*/

	u4TempAddr = pu1Data;
	u1TempValue = (RUINT8 *)(u4TempAddr);

	while(u4Index < u4Size){
		if(!isRxFifoEmpty(pCfgInstance)){
			uartRegRead(XUARTPS_FIFO_OFFSET, u1TempValue, pCfgInstance);// read if rx fifo is not empty
		}
		else
			break;

		u4Index++;
		u1TempValue = (RUINT8 *)(u4TempAddr + u4Index);

	}
}


ReturnType UartSendData(RUINT8 *pu1Data, uartCfgType *pCfgInstance, RUINT32 Size)
{
	TxDataPolling(pu1Data, Size, pCfgInstance);
}

ReturnType UartReceiveDataPolling(RUINT8 *pu1Data, uartCfgType *pCfgInstance, RUINT32 Size)
{
	RxDataPolling(pu1Data, Size, pCfgInstance);
}

/*
 * This function is interrupt driven approach for receiving data
 * pu1Data : pointer that received values will be written to
 * u4size	: size of bytes to be received
 * pCfgInstance : pointer to the uart device instance hence there are two different instances
 */
static ReturnType receiveData(RUINT8 *pu1Data, RUINT32 u4Size, uartCfgType *pCfgInstance)
{
	ReturnType retVal = XST_SUCCESS;
	/*
	 * approach :
	 * enable interrupts
	 * wait rxFIFO to fill or Rxtimeout
	 * read data from RXFIFO
	 * do this until FIFO empty
	 * clear interrupt bit if set
	 * */
	retVal += enableInterrupt(pCfgInstance);

}

/*
 * This function is interrupt driven approach for receiving data
 * pu1Data : pointer that received values will be written to
 * u4size	: size of bytes to be received
 * pCfgInstance : pointer to the uart device instance hence there are two different instances
 */
static ReturnType enableInterrupt(uartCfgType *pCfgInstance)
{

	RUINT32 u4TempRxTriggerLevel = 0x1U; // previously 32 byte data
	RUINT32 u4TempReadRegister = 0;
	RUINT32 u4TempHighVal = 0x01U;
	//program trigger level
	uartRegWrite(XUARTPS_RXWM_OFFSET, u4TempRxTriggerLevel, pCfgInstance);

	//enable interrupt
	// set IER RX FIFO trigger register
	// clear IDR RX FIFO trigger register
	// read IER RX FIFO trigger register to check settings
	uartSetRegBit (XUARTPS_IER_OFFSET, XUARTPS_IXR_RXOVR, pCfgInstance);
	uartClearRegBit(XUARTPS_IDR_OFFSET, XUARTPS_IXR_RXOVR, pCfgInstance);
	uartRegRead(XUARTPS_IMR_OFFSET, &u4TempReadRegister, pCfgInstance);
	if((u4TempReadRegister & (u4TempHighVal << XUARTPS_IXR_RXOVR)) == 0x01)
	{
		// interrupt enabled - proceed
		return XST_SUCCESS;
	}

	return XST_FAILURE;
}

void xUartPsReceiveDataHandler(uartCfgType *pCfgInstance)
{

}

void xUartPsInterruptHandler(uartCfgType *pCfgInstance)
{
    RUINT32 u4IsrStatus;
    RUINT32 u4TempMaskReg;
    RUINT32 u4TempAddr = a1UartRxArray;
    RUINT32 u4TempRead;
    RUINT32 u4TempLogicHigh = 0x1U;
    RUINT32 u4ReceivedDataSize = 0;

    /*
     * Read the interrupt ID register to determine which
     * interrupt is active
     */
	uartRegRead(XUARTPS_ISR_OFFSET, &u4IsrStatus, pCfgInstance);

    uartRegRead(XUARTPS_IMR_OFFSET, &u4TempMaskReg, pCfgInstance);
    u4IsrStatus &= u4TempMaskReg;

    /* Dispatch an appropriate handler. */
    if ((u4IsrStatus & ((u4TempLogicHigh << XUARTPS_IXR_RXOVR) | (u4TempLogicHigh <<XUARTPS_IXR_RXEMPTY) | (u4TempLogicHigh << XUARTPS_IXR_RXFULL))) != 0U)
    {
        /* Received data interrupt */
		while(!isRxFifoEmpty(pCfgInstance))
		{
			uartRegRead(XUARTPS_FIFO_OFFSET, (RUINT32 *)(u4TempAddr + u4ReceivedDataSize), pCfgInstance);// read if rx fifo is not empty
			u4ReceivedDataSize++;
		}
    }

    /* Clear the interrupt status since UART interrup status bits are sticky, meaning : unless they are cleared by writing 1 to the
     * position of the bit that is logic high which would result in '0' - logic low in that bit position, these interrupts would keep going */
    uartRegWrite(XUARTPS_ISR_OFFSET, u4IsrStatus, pCfgInstance);

    UartSendData(a1UartRxArray, pCfgInstance, u4ReceivedDataSize);

    //    TxDataPolling((RUINT8 *)u4TempAddr, u4ReceivedDataSize, pCfgInstance);
    //    u4ReceivedDataSize = 0;

    /*clear pending interrupt*/
    // read ICCIAR register(0xF8F0010C)(Interrupt acknowledge register) to figure out which interurpts are pending/active
    // write to ICCEOIR register(0xF8F00110)(end of interrupt register) to deassert/clear active or pending interrupts
    //    RUINT32 u4ICCIARRead = 0;
    //    regRead(0xF8F0010C, &u4ICCIARRead);
    //    regWrite(0xF8F00110, u4ICCIARRead);

    /*read interrup acknowledge register(ICCIAR) or write 1 to corresponding bit of ICDICPR (interrupt clear pending register)*/
    // trial - read ICCIAR
	//    regRead(0xf8f01288, &u4TempRead); // read value is 0x00040000 which is that 18th bit is 1 -> this means according to the calc below, interrupt ID #82 is pending
	//    // it requires clear pending operation - proceed clear pending
	//    regWrite(0xf8f01288, &u4TempRead);
	//
	//    u4TempRead = 0x0U;
	//    regRead(0xf8f01288, &u4TempRead);
    /*
	 * For interrupt ID N, when DIV and MOD are the integer division and modulo operations:
		� the corresponding ICDICPR number, M, is given by M = N DIV 32
		� the offset of the required ICDICPR is (0x280 + (4*M))
		� the bit number of the required Set-pending bit in this register is N MOD 32.

		-> for our case - ID is 82, thus :
		N = 82
		M = 82 / 32 = 2 (read ICDICPR2 - 0xf8f01288)
		bit pos : 82 mod 32 -> bit pos 18
*/

    // write 1 to the corresponding status register -> to do that first read register and write the read value back to the register
    /*
		The Chnl_int_sts_reg0 register can be read for status and generate an interrupt. The Channel_sts_reg0
		register can only be read for status.
		The Chnl_int_sts_reg0 register is sticky; once a bit is set, the bit stays set until software clears it. Write
		a 1 to clear a bit
     */

}
