/*
===============================================================================
 Name        : LPC43xx_M4_AnalogToDigital.c
 Author      : Li Alex Zhang
 Version     :
 Copyright   :
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
static ADC_CLOCK_SETUP_T ADCSetup;
static uint16_t dataADC;
static uint16_t dataADCMax=0x0000;
static uint16_t dataADCMin=0xFFFF;
static float thresholdMinRatio=0.3;
static float thresholdMaxRatio=0.7;
static uint16_t thresholdMin;
static uint16_t thresholdMax;

#define _GPIO_PORT 0x00
#define _GPIO_PIN 0x0A
static bool DigitalOut=false;
static bool IsAutoThres=true;

#define SYSTICKRATE_HZ (1000)	/* 1000 ticks per second, 1ms systick interrupt */
static uint32_t autoThresDur = 5000; // 5000ms auto threshold period
static uint32_t tick_ct = 0;


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

/* Polling DAC */
static void Polling_DAC(void)
{
	Chip_DAC_UpdateValue(LPC_DAC, dataADC);
}

/* Check Analog Input with Thresholds, Update Input Extreme Values */
static void Auto_Threshold_Output(void)
{
	if(IsAutoThres)
	{
		if(dataADC>dataADCMax)
		{
			dataADCMax = dataADC;
			return;
		}
		if(dataADC<dataADCMin)
		{
			dataADCMin = dataADC;
			return;
		}
	}
	else
	{
		if(dataADC>=thresholdMax)
		{
			DigitalOut = true;
			Chip_GPIO_SetPinState(LPC_GPIO_PORT, _GPIO_PORT, _GPIO_PIN, DigitalOut);
			Board_LED_Set(0, DigitalOut);
			return;
		}
		if(dataADC<=thresholdMin)
		{
			DigitalOut = false;
			Chip_GPIO_SetPinState(LPC_GPIO_PORT, _GPIO_PORT, _GPIO_PIN, DigitalOut);
			Board_LED_Set(0, DigitalOut);
			return;
		}
	}
}

/* Disable SysTick IRQ and SysTick Timer */
STATIC INLINE void SysTick_Disable(void)
{
	SysTick->CTRL  &= 0xFFFFFFFF << 2;
}

/* System Timer Interrupt Handler */
void SysTick_Handler(void)
{
	tick_ct += 1;
	if(tick_ct>autoThresDur)
	{
		IsAutoThres = false;
		// Update thresholds after auto threshold period
		uint16_t dataRange = dataADCMax-dataADCMin;
		thresholdMax = dataADCMin+ dataRange*thresholdMaxRatio;
		thresholdMin = dataADCMin+ dataRange*thresholdMinRatio;

		SysTick_Disable();
	}
}


int main(void)
{
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

    Board_DAC_Init(LPC_DAC);
    Chip_DAC_Init(LPC_DAC);

    Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, _GPIO_PORT, _GPIO_PIN);

    SysTick_Config(SystemCoreClock / SYSTICKRATE_HZ);
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
        Polling_DAC();
        Auto_Threshold_Output();

    }
    return 0 ;
}
