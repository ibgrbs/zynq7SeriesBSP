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

typedef enum {
	UART_INSTANCE_DEVICE_0 = 0,
	UART_INSTANCE_DEVICE_1 = 1
} uartDeviceInstanceType;

typedef enum {
	UART_ENABLE = 0,
	UART_DISABLE = 1
} uartStatusType;

typedef struct {
	uartDeviceInstanceType DeviceNum;
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
ReturnType InitializeUart(uartCfgType *cfgInstance);
ReturnType UartSendData(RUINT8 *pu1Data, uartCfgType *pCfgInstance, RUINT32 Size);

#endif /* SRC_UARTDRIVER_UART_H_ */
