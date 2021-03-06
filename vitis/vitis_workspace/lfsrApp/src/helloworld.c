#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "fsl.h"
#include <stdlib.h>
#include "xparameters.h"
#include "xtmrctr_l.h"

#include "sleep.h"		// contains Microblaze specific sleep related APIs
#include "xgpio_l.h"	// identifiers and driver functions (or macros) that can be used to access the device (XGpio_WriteReg, XGpio_ReadReg,...)


#include "stdbool.h" ///C99

void ResetPerformanceTimer()
{
	// Disable a timer counter such that it stops running (base address of the device, the specific timer counter within the device)
	XTmrCtr_Disable(XPAR_TMRCTR_0_BASEADDR, 0);
	/*
	 * Set the value that is loaded into the timer counter and cause it to
	 * be loaded into the timer counter
	 */
//	Set the Load Register of a timer counter to the specified value.
//	(the base address of the device, specific timer counter within the device, 32 bit value to be written to the register)
	XTmrCtr_SetLoadReg(XPAR_TMRCTR_0_BASEADDR, 0, 0);
//	Cause the timer counter to load it's Timer Counter Register with the value in the Load Register.
//	(the base address of the device, the specific timer counter within the device)
	XTmrCtr_LoadTimerCounterReg(XPAR_TMRCTR_0_BASEADDR, 0);
//	Set the Control Status Register of a timer counter to the specified value.
//	(base address of the device, specific timer counter within the device, 32 bit value to be written to the register)
	XTmrCtr_SetControlStatusReg(XPAR_TMRCTR_0_BASEADDR, 0, 0x00000000);
}

void RestartPerformanceTimer()
{
	ResetPerformanceTimer();
	XTmrCtr_Enable(XPAR_TMRCTR_0_BASEADDR, 0);
}

unsigned int GetPerformanceTimer()
{
	return XTmrCtr_GetTimerCounterReg(XPAR_TMRCTR_0_BASEADDR, 0);
}

unsigned int StopAndGetPerformanceTimer()
{
	XTmrCtr_Disable(XPAR_TMRCTR_0_BASEADDR, 0);
	return GetPerformanceTimer();
}

// 7 segment decoder
unsigned char Hex2Seg(unsigned int value, bool dp) //converts a 4-bit number [0..15] to 7-segments; dp is the decimal point
{
	static const char Hex2SegLUT[] = {0x40, 0x79, 0x24, 0x30, 0x19, 0x12, 0x02, 0x78,
									  0x00, 0x10, 0x08, 0x03, 0x46, 0x21, 0x06, 0x0E};
	return dp ? Hex2SegLUT[value] : (0x80 | Hex2SegLUT[value]);
}

unsigned short softwareLFSR(unsigned short lfsr, unsigned int period)
{
        unsigned short bit;
        while(period != 0) {
        	//  bit/tap 16, 15, 13, 4, 1
        	bit = ( (lfsr >> 0) ^ (lfsr >> 1) ^ (lfsr >> 3) ^ (lfsr >> 12) ) & 1;
        	// shifting
        	lfsr = (lfsr >> 1) | (bit << 15);
        	period--;
        }
        //
        return lfsr;
}

int main()
{
    init_platform();

    xil_printf("Linear-Feedback Shift Register Calculator\n\r");

    unsigned short a = 0xACE1;
    unsigned short period = 0xFFFF;
    unsigned short result_sw = 0;
    unsigned short result_hw = 0;
    unsigned int send2hw = (period << 16) | a;
    unsigned int timeElapsed;

    xil_printf("Input value: %x\nPeriod: %d", a, period);

    // SOFTWARE
    RestartPerformanceTimer();
    result_sw = softwareLFSR(a, period);
    timeElapsed = StopAndGetPerformanceTimer();
    xil_printf("\nSoftware only LFSR time: %d microseconds\nSoftware Result: %x\n", timeElapsed / (XPAR_CPU_M_AXI_DP_FREQ_HZ / 1000000), result_sw); //  ,

    //HARDWARE
    RestartPerformanceTimer();
    putfslx(send2hw, 0, FSL_DEFAULT);
    getfslx(result_hw, 0, FSL_DEFAULT);
    timeElapsed = StopAndGetPerformanceTimer();
    xil_printf("\nHardware only LFSR time: %d microseconds\nHardware Result: %x\n", timeElapsed / (XPAR_CPU_M_AXI_DP_FREQ_HZ / 1000000), result_hw);
    xil_printf("\nChecking result: %s\n\r", result_sw==result_hw ? "OK" : "Error");

    XGpio_WriteReg(XPAR_AXI_GPIO_DISPLAYS_BASEADDR,  XGPIO_TRI_OFFSET,  0xFFFFFF00);
    XGpio_WriteReg(XPAR_AXI_GPIO_DISPLAYS_BASEADDR,  XGPIO_TRI2_OFFSET, 0xFFFFFF00);

	while(1)
	{
		XGpio_WriteReg(XPAR_AXI_GPIO_DISPLAYS_BASEADDR, XGPIO_DATA_OFFSET,  0xFE);
		XGpio_WriteReg(XPAR_AXI_GPIO_DISPLAYS_BASEADDR, XGPIO_DATA2_OFFSET, Hex2Seg(result_hw & 0x000F, false));

		usleep(1250); //delay in usec

		XGpio_WriteReg(XPAR_AXI_GPIO_DISPLAYS_BASEADDR, XGPIO_DATA_OFFSET,  0xFD);
		XGpio_WriteReg(XPAR_AXI_GPIO_DISPLAYS_BASEADDR, XGPIO_DATA2_OFFSET, Hex2Seg((result_hw >> 4) & 0x000F, false));

		usleep(1250); //delay in usec


		XGpio_WriteReg(XPAR_AXI_GPIO_DISPLAYS_BASEADDR, XGPIO_DATA_OFFSET,  0xFB);
		XGpio_WriteReg(XPAR_AXI_GPIO_DISPLAYS_BASEADDR, XGPIO_DATA2_OFFSET, Hex2Seg((result_hw >> 8) & 0x000F, false));

		usleep(1250);


		XGpio_WriteReg(XPAR_AXI_GPIO_DISPLAYS_BASEADDR, XGPIO_DATA_OFFSET,  0xF7);
		XGpio_WriteReg(XPAR_AXI_GPIO_DISPLAYS_BASEADDR, XGPIO_DATA2_OFFSET, Hex2Seg((result_hw >> 12) & 0x000F, false));


		usleep(1250);
		//TODO: try showing other than 1 and 0 values on the displays
	}

    cleanup_platform();
    return 0;
}

//''

//
