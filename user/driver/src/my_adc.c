#include "my_adc.h"

/*
note:

TIM + ADC + DMA

����2M�Ĳ�����
*/

//�ڴ�4�ֽڶ���
__ALIGNED(4)  uint8_t adc_buffer[SAMPLING_DEPTH*2];
volatile uint8_t adc_dma_finish_cnt;

__ALIGNED(4)  uint8_t adc_buffer_copy[SAMPLING_DEPTH];
uint8_t buffer_show[160];

void adc_init(void)
{
	HAL_TIM_Base_Start(&htim2);
	HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, SAMPLING_DEPTH*2);
}

// ADC DMA�봫����ɻص�������ǰ���buffer����
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc) 
{
    if (hadc->Instance == ADC1)
	{
		adc_dma_finish_cnt++;
    }
}




