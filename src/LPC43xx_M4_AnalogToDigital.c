/*
===============================================================================
 Name        : LPC43xx_M4_AnalogToDigital.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : Convert Analog(0-3.3V) Input(Ch3) to TTL(Active-High) Output(P1_3, GPIO0[10])
===============================================================================
*/

#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif

#include <cr_section_macros.h>

// TODO: insert other include files here

// TODO: insert other definitions and declarations here
#define _ADC_CHANNLE ADC_CH3
#define _LPC_ADC_ID LPC_ADC0
#define _LPC_ADC_IRQ ADC0_IRQn
#define _GPDMA_CONN_ADC GPDMA_CONN_ADC_0
static ADC_CLOCK_SETUP_T ADCSetup;
static uint16_t dataADC;
static uint16_t dataADCMax=10;
static uint16_t dataADCMin=0;
static float thresholdMin=0.3;
static float thresholdMax=0.7;

#define _GPIO_PORT 0x00
#define _GPIO_PIN 0x0A
static bool DigitalOut;


/* Polling ADC */
static void Polling_ADC(void)
{
	/* Start A/D conversion */
	Chip_ADC_SetStartMode(_LPC_ADC_ID, ADC_START_NOW, ADC_TRIGGERMODE_RISING);
	/* Waiting for A/D conversion complete */
	while (Chip_ADC_ReadStatus(_LPC_ADC_ID, _ADC_CHANNLE, ADC_DR_DONE_STAT) != SET) {}
	/* Read ADC value */
	Chip_ADC_ReadValue(_LPC_ADC_ID, _ADC_CHANNLE, &dataADC);
}

/* Check Analog Input with Thresholds and Update Input Extreme Values */
static void Auto_Threshold_Output(void)
{
	uint16_t dataRange = dataADCMax-dataADCMin;
	if(dataADC>=dataADCMin+ dataRange*thresholdMax)
	{
		DigitalOut = true;
		if(dataADC>dataADCMax)
		{
			dataADCMax = dataADC;
		}
		return;
	}
	if(dataADC<=dataADCMin+ dataRange*thresholdMin)
	{
		DigitalOut = false;
		if(dataADC<dataADCMin)
		{
			dataADCMin = dataADC;
		}
		return;
	}
}


int main(void) {

#if defined (__USE_LPCOPEN)
    // Read clock settings and update SystemCoreClock variable
    SystemCoreClockUpdate();
#if !defined(NO_BOARD_LIB)
#if defined (__MULTICORE_MASTER) || defined (__MULTICORE_NONE)
    // Set up and initialize all required blocks and
    // functions related to the board hardware
    Board_Init();

    Board_ADC_Init();
    Chip_ADC_Init(_LPC_ADC_ID, &ADCSetup);
    Chip_ADC_EnableChannel(_LPC_ADC_ID, _ADC_CHANNLE, ENABLE);

    Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, _GPIO_PORT, _GPIO_PIN);
#endif
    // Set the LED to the state of "On"
    Board_LED_Set(0, true);
#endif
#endif

    // TODO: insert code here

    // Force the counter to be placed into memory
    volatile static int i = 0 ;
    // Enter an infinite loop, just incrementing a counter
    while(1) {
        i++ ;
        Polling_ADC();
        Auto_Threshold_Output();
        Chip_GPIO_SetPinState(LPC_GPIO_PORT, _GPIO_PORT, _GPIO_PIN, DigitalOut);
        Board_LED_Set(0, DigitalOut);

        /*
        if((i%200000)<100000)
                {
                	//Chip_GPIO_SetPortValue(LPC_GPIO_PORT, _GPIO_PORT, 0xFFFFFFFF);
                	//Chip_GPIO_SetPinState(&GPIO, _GPIO_PORT, _GPIO_PIN, true);
                Chip_GPIO_SetPinOutHigh(LPC_GPIO_PORT, _GPIO_PORT, _GPIO_PIN);
                Board_LED_Set(0, true);
                //s= Chip_GPIO_GetPinState(LPC_GPIO_PORT, _GPIO_PORT, _GPIO_PIN);
                //s=Chip_GPIO_GetPinDIR(LPC_GPIO_PORT, _GPIO_PORT, _GPIO_PIN);
                }
                else
                {
                	//Chip_GPIO_SetPortValue(LPC_GPIO_PORT, _GPIO_PORT, 0x00000000);
                Chip_GPIO_SetPinOutLow(LPC_GPIO_PORT, _GPIO_PORT, _GPIO_PIN);
                Chip_GPIO_SetPinState(&GPIO, _GPIO_PORT, _GPIO_PIN, false);
                Board_LED_Set(0, false);
                //s= Chip_GPIO_GetPinState(LPC_GPIO_PORT, _GPIO_PORT, _GPIO_PIN);
                }
                */
    }
    return 0 ;
}
