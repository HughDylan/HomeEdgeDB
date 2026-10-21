#include "sensor.h"
#include "AD.h" 

static float current_temp = 0.0f;
static uint8_t current_light = 0;

void Sensor_Init(void) 
{
    AD_Init(); 
}

void Sensor_Update(void) 
{
    uint32_t sum_temp = 0;
    uint32_t sum_light = 0;
    uint8_t i;

    for (i = 0; i < 5; i++) 
    {
        sum_temp += AD_GetValue(ADC_Channel_0);  
        sum_light += AD_GetValue(ADC_Channel_1); 
    }

    uint16_t raw_temp = sum_temp / 5;
    // 简单映射到 15 - 45 度
    current_temp = 15.0f + ((float)raw_temp / 4095.0f) * 30.0f;
    current_light = (uint8_t)(((sum_light / 5) * 100) / 4095);
}

float Sensor_GetTemp(void) { return current_temp; }
uint8_t Sensor_GetLight(void) { return current_light; }
