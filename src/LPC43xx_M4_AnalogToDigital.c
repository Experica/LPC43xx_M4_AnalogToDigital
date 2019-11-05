/*
===============================================================================
 Name        : LPC43xx_M4_AnalogToDigital.c
 Author      : Li Alex Zhang
 Version     : 0.3
 Copyright   : Li Alex Zhang
 Description : Convert Analog(0-3.3V) Input(Ch2; Ch3) to Digital(0/3.3V, Active-High) Output(P1_3, GPIO0[10]; P1_4, GPIO0[11]), also send Digitized Analog Input to Analog Output(Ch0; Ch1).
 	 	 	   Relay Digital Input(P1_5, GPIO1[8]) to Output(P1_6, GPIO1[9])
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



#define _ADC0 LPC_ADC0
#define _ADC1 LPC_ADC1
#define _ADC_CHANNEL_0 ADC_CH2
#define _ADC_CHANNEL_1 ADC_CH3
static ADC_CLOCK_SETUP_T ADC0Setup;
static ADC_CLOCK_SETUP_T ADC1Setup;
static uint16_t data0ADC;
static uint16_t data1ADC;
static uint16_t data0ADCMax=0x0000;
static uint16_t data0ADCMin=0xFFFF;
static uint16_t data1ADCMax=0x0000;
static uint16_t data1ADCMin=0xFFFF;
const float thresholdMinRatio=0.3; // Noise Margin is 30%
const float thresholdMaxRatio=0.7;
static uint16_t threshold0Min;
static uint16_t threshold0Max;
static uint16_t threshold1Min;
static uint16_t threshold1Max;

#define _GPIO_PORT_0 0x00
#define _GPIO_PORT_1 0x01
#define _GPIO_ADOUTPUT_PIN_0 0x0A
#define _GPIO_ADOUTPUT_PIN_1 0x0B
static bool ADOutput0=false;
static bool ADOutput1=false;
static bool IsAutoADThreshold=true;

#define _GPIO_DRELAYINPUT_PIN 0x08
#define _GPIO_DRELAYOUTPUT_PIN 0x09
static bool DRelayIn = false;
static bool DRelayInLast = false;

#define SYSTICKRATE_HZ (1000)	// 1000 ticks per second, 1ms systick interrupt
const uint32_t AutoADThresholdDuration = 5000; // 5000ms auto threshold period
static uint32_t tick_n = 0;



static inline void Polling_ADC(void)
{
	/* Start A/D conversion */
	Chip_ADC_SetStartMode(_ADC0, ADC_START_NOW, ADC_TRIGGERMODE_RISING);
	/* Waiting for A/D conversion complete */
	while (Chip_ADC_ReadStatus(_ADC0, _ADC_CHANNEL_0, ADC_DR_DONE_STAT) != SET) {}
	/* Read ADC value */
	Chip_ADC_ReadValue(_ADC0, _ADC_CHANNEL_0, &data0ADC);

	Chip_ADC_SetStartMode(_ADC1, ADC_START_NOW, ADC_TRIGGERMODE_RISING);
	while (Chip_ADC_ReadStatus(_ADC1, _ADC_CHANNEL_1, ADC_DR_DONE_STAT) != SET) {}
	Chip_ADC_ReadValue(_ADC1, _ADC_CHANNEL_1, &data1ADC);
}

static inline void Polling_DAC(void)
{
	Chip_DAC_UpdateValue(LPC_DAC, data0ADC);
}

/* Check Analog Input with Thresholds, Update Input Extreme Values, and Output Digital Based on Thresholds */
static inline void Auto_Threshold_ADOutput(void)
{
	if(IsAutoADThreshold)
	{
		if(data0ADC>data0ADCMax)
		{
			data0ADCMax = data0ADC;
		}
		if(data0ADC<data0ADCMin)
		{
			data0ADCMin = data0ADC;
		}

		if(data1ADC>data1ADCMax)
		{
			data1ADCMax = data1ADC;
		}
		if(data1ADC<data1ADCMin)
		{
			data1ADCMin = data1ADC;
		}
	}
	else
	{
		if(data0ADC>=threshold0Max)
		{
			ADOutput0 = true;
			Chip_GPIO_SetPinState(LPC_GPIO_PORT, _GPIO_PORT_0, _GPIO_ADOUTPUT_PIN_0, ADOutput0);
			Board_LED_Set(0, ADOutput0);
		}
		else if(data0ADC<=threshold0Min)
		{
			ADOutput0 = false;
			Chip_GPIO_SetPinState(LPC_GPIO_PORT, _GPIO_PORT_0, _GPIO_ADOUTPUT_PIN_0, ADOutput0);
			Board_LED_Set(0, ADOutput0);
		}

		if(data1ADC>=threshold1Max)
		{
			ADOutput1 = true;
			Chip_GPIO_SetPinState(LPC_GPIO_PORT, _GPIO_PORT_0, _GPIO_ADOUTPUT_PIN_1, ADOutput1);
			Board_LED_Set(1, ADOutput1);
		}
		else if(data1ADC<=threshold1Min)
		{
			ADOutput1 = false;
			Chip_GPIO_SetPinState(LPC_GPIO_PORT, _GPIO_PORT_0, _GPIO_ADOUTPUT_PIN_1, ADOutput1);
			Board_LED_Set(1, ADOutput1);
		}
	}
}

static inline void Relay_Digital(void)
{
	DRelayIn = Chip_GPIO_GetPinState(LPC_GPIO_PORT, _GPIO_PORT_1, _GPIO_DRELAYINPUT_PIN);
	if (DRelayIn != DRelayInLast)
	{
		Chip_GPIO_SetPinState(LPC_GPIO_PORT, _GPIO_PORT_1, _GPIO_DRELAYOUTPUT_PIN, DRelayIn);
		Board_LED_Set(3, DRelayIn);
		DRelayInLast = DRelayIn;
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
	tick_n += 1;
	if(tick_n>AutoADThresholdDuration)
	{
		// Update thresholds after auto threshold period
		uint16_t dataRange = data0ADCMax-data0ADCMin;
		threshold0Max = data0ADCMin+ dataRange*thresholdMaxRatio;
		threshold0Min = data0ADCMin+ dataRange*thresholdMinRatio;

		dataRange = data1ADCMax-data1ADCMin;
		threshold1Max = data1ADCMin+ dataRange*thresholdMaxRatio;
		threshold1Min = data1ADCMin+ dataRange*thresholdMinRatio;

		IsAutoADThreshold = false;
		SysTick_Disable();
	}
}



int main(void)
{
#if defined (__USE_LPCOPEN)
    SystemCoreClockUpdate();
#if !defined(NO_BOARD_LIB)
#if defined (__MULTICORE_MASTER) || defined (__MULTICORE_NONE)
    Board_Init();

    Board_ADC_Init();
    Chip_ADC_Init(_ADC0, &ADC0Setup);
    Chip_ADC_Init(_ADC1, &ADC1Setup);
    Chip_ADC_EnableChannel(_ADC0, _ADC_CHANNEL_0, ENABLE);
    Chip_ADC_EnableChannel(_ADC1, _ADC_CHANNEL_1, ENABLE);

    Board_DAC_Init(LPC_DAC);
    Chip_DAC_Init(LPC_DAC);
    Chip_DAC_ConfigDAConverterControl(LPC_DAC, (DAC_CNT_ENA | DAC_DMA_ENA));
    Chip_DAC_SetBias(LPC_DAC, DAC_MAX_UPDATE_RATE_400kHz);

    Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, _GPIO_PORT_0, _GPIO_ADOUTPUT_PIN_0);
    Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, _GPIO_PORT_0, _GPIO_ADOUTPUT_PIN_1);

    Chip_GPIO_SetPinDIRInput(LPC_GPIO_PORT, _GPIO_PORT_1, _GPIO_DRELAYINPUT_PIN);
    Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, _GPIO_PORT_1, _GPIO_DRELAYOUTPUT_PIN);

    SysTick_Config(SystemCoreClock / SYSTICKRATE_HZ);
#endif
    Board_LED_Set(0, true);
#endif
#endif

    while(1)
    {
        Relay_Digital();
        Polling_ADC();
        Polling_DAC();
        Auto_Threshold_ADOutput();
    }
    return 0;
}
