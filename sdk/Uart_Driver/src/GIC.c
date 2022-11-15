/*
 * GIC.c
 *
 *  Created on: Nov 13, 2022
 *      Author: bugra's PC
 */


#include "GIC.h"

/***********definitions**************/
//register offsets
#define MPCORE_ADDRESS 0xF8F00000 		// Applciation processing unit base address
#define ICCICR_OFFSET 0x00000100 		// CPU interface control register
#define ICCPMR_OFFSET 0x00000104 		// Interrupt Priority Mask Register
#define ICCBPR_OFFSET 0x00000108 		// Binary Point Register
#define ICCIAR_OFFSET 0x0000010C 		// Interrupt Acknowledge Register
#define ICCEOIR_OFFSET 0x00000110 		// End Of Interrupt Register
#define ICDDCR_OFFSET 0x00001000		// Distributor Control Register
#define ICDICTR_OFFSET 0x00001004		// Interrupt Controller Type Register
#define ICDIIDR_OFFSET 0x00001008		// Distributor Implementer Identification register
//Interrupt configuration registers // default values for ICDICFRs are 0xAAAAAAAA - each edge sensitive
#define ICDICFR0_OFFSET 0x00001C00 		// Interruot configuration register 0
#define ICDICFR1_OFFSET 0x00001C04 		// Interruot configuration register 1
#define ICDICFR2_OFFSET 0x00001C08 		// Interruot configuration register 2
#define ICDICFR3_OFFSET 0x00001C0C 		// Interruot configuration register 3
#define ICDICFR4_OFFSET 0x00001C10 		// Interruot configuration register 4
#define ICDICFR5_OFFSET 0x00001C14 		// Interruot configuration register 5



//typedefs and enums

//function prototypes

void readGICReg(RUINT32 *dataHolder, RUINT32 offset);
void writeGICReg(RUINT32 data, RUINT32 offset);
void InitDistributor();
void InitCPUInterface();
void GICInterruptHandler();
void setInterruptHandler();





/*general info :
 * distributor -> sends and signals CPU regarding interrupt
 * 	-> receives the interrupt signals
 * 	-> forwarding interrupt signals to CPU GIC controller
 * 	-> enabling / disabling interrupts
 * 	-> setting priorities to interrupts
 * 	-> setting target processor of the interrupts (which interrupt signal will be feed into which CPU)
 * 	-> setting interrupt level sensitivity or edge triggered (sensitivity) option
 * CPU interface -> acks interrupts and distributor
 * 	-> enabling signalling of interrupt requests by CPU
 * 	-> acknowledging interrupts
 * 	-> indicating compilation of processing of an interrupt
 * GIC controller*/

/*procedure :
 * initialize distributor
 * initalize CPU interface
 * initialize GIC instance
 * initialize and start GIC
 * initialize interrupts*/

/*after reset or Power on, Distributor and CPU interface controller is disabled.
 * It is the responsiblity of the software to initialize*/

/* OP - 1
 * ICD - Distributor initialization operations
 * specify each interrupts sensitivity (level sensitive vs edge triggered) from ICDICFRs registers
 * set ICDIPRs to specify each interrupts priority value
 * set which interrupt signal will be feed into which CPU (in case of multi processor such as zynq-7010)
 * using ICDIPTRs -> target processor list for each interrupt
 * write ICDISERs to enable interrupts (interrupt set enable registers)
 * */

/* OP - 2
 * ICC - CPU interface initialization operations
 * write to ICCPMR to set priority mask for interface
 * write to ICCBPR to set binary position register
 * write to ICCICR to enable signalling of interrupts by the interface to the CPUs
 * */

/* OP - 3
 * write to ICDDCR register to enable distributor (bit 0 shall be set to '1')
 * */

void readGICReg(RUINT32 *dataHolder, RUINT32 offset)
{
	RUINT32 fullReadAddr = MPCORE_ADDRESS + offset;
	*dataHolder = *(RUINT32 *)fullReadAddr;
}

void writeGICReg(RUINT32 data, RUINT32 offset)
{
	RUINT32 fullReadAddr = MPCORE_ADDRESS + offset;
	*(RUINT32 *)fullReadAddr = data;
}

void InitDistributor()
{
	// write all ICDICFRs to 0xAAAAAAAA - it is the reset value aswell
	// set interrupt priorities


}

