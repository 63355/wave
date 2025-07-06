#ifndef MY_ADC_H
#define MY_ADC_H

#include "tim.h"
#include "adc.h"
#include "dma.h"

#define SAMPLING_SPEED 2000000 //采样率(Sa/s)
#define SAMPLING_DEPTH 10240   //采样深度(Sa)


extern uint8_t adc_buffer[SAMPLING_DEPTH*2];
extern volatile uint8_t adc_dma_finish_cnt;

extern uint8_t adc_buffer_copy[SAMPLING_DEPTH];
extern uint8_t buffer_show[160];

void adc_init(void);

#endif 
