/*
 * Uart.h
 *
 *  Created on: Aug 28, 2022
 *      Author: bugra's PC
 */

#ifndef SRC_UARTDRIVER_UART_H_
#define SRC_UARTDRIVER_UART_H_
/**************Inclusions******************/
#include "CommonTypes.h"
/**************Macros******************/
#define SHIFT_LEFT(x,y) x<<y
#define SHIFT_RIGHT(x,y) x>>y
/**************Definitions******************/
typedef enum {
	UART_PARITY_EVEN = 0,
	UART_PARITY_ODD,
	UART_PARITY_NONE
} uartParityType;

typedef enum {
	UART_BAUDRATE_115200 = 115200,
	UART_BAUDRATE_921600 = 921600
} uartBaudRateType;

typedef struct {
	RUINT32 u4ControlRegister;
	uartBaudRateType BaudRate;
	uartParityType Parity;
} uartCfgType;

typedef struct {
	RINT32 *TxDataFifo;
	RUINT32 *RxDataFifo;
} FIFOType;

typedef struct {
	RINT8 Start;
	RINT8 Size;
} BITType;
/**************Prototypes******************/
//static void configureUartregister(const uartConfigureType *cpConfig);
ReturnType ConfigureUart(uartCfgType cfgInstance);

#endif /* SRC_UARTDRIVER_UART_H_ */