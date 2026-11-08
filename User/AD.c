#include "stm32f10x.h"

void AD_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef ADC_InitStructure;

    // 1. ????
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div6); // 72/6 = 12MHz

    // 2. ??:??????????? (AIN)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1; // PA0 ? PA1
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;          // ????
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 3. ADC ???
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStructure);

    // 4. ?? ADC1
    ADC_Cmd(ADC1, ENABLE);

    // 5. ??(????,???)
    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1));
}

// ?? ?? channel ??:0 = PA0, 1 = PA1
uint16_t AD_GetValue(uint8_t channel)
{
    // ???????
    ADC_RegularChannelConfig(ADC1, channel, 1, ADC_SampleTime_55Cycles5);
    // ??????
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    // ??????
    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
    // ??????
    return ADC_GetConversionValue(ADC1);
}
