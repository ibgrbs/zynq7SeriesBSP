/*
 * GIC.h
 *
 *  Created on: Nov 13, 2022
 *      Author: bugra's PC
 */

#ifndef SRC_GIC_H_
#define SRC_GIC_H_

#include "CommonTypes.h"

#define GIC_MAX_NUMBER_OF_INTERRUPTS 96

typedef void (*CallbackFunction)(void* Argument);
typedef void (*InterruptHandlerFunc)(void);

typedef struct {
	CallbackFunction CallBack;
	void* CallBackArgumentSet;
}InterruptHandlerType;

typedef struct {
	RUINT32 GICBaseAddr;
	RUINT32 IsInitialized;
	InterruptHandlerType InterruptHandlerVector[GIC_MAX_NUMBER_OF_INTERRUPTS];
}GICInstanceType;

typedef enum {
	GIC_SUCCESS = 0,
	GIC_FAILURE = 1,
	GIC_UNKNOWN = 0xFFFFFFFF
}GICResultEnum;



GICResultEnum InitializeGIC(GICInstanceType* InstancePtr);
GICResultEnum GICInstanceInitializer(GICInstanceType* GICInstance);
GICResultEnum GICConnectInterruptHandler(GICInstanceType* GICInstance, RUINT32 InterruptID, void* InterruptHandlerFunction, void* InterruptHandlerFunctionArgument);
void ConnectInterruptHandler(RUINT32 IntId, InterruptHandlerFunc HandlerFunc);

#endif /* SRC_GIC_H_ */
