#include "button_user.h"

button_t button1;

uint8_t single_flag,double_flag,long_falg;

static uint8_t getButtonVal(void)
{
    uint8_t pinState = (GPIOC->IDR & GPIO_IDR_ID13) ? 1 : 0;
    return pinState; 
}

void singleClickHandler(void)
{
	single_flag++;
	single_flag%=2;
}
void doubleClickHandler(void)
{
	double_flag++;
	double_flag%=2;
}
void longPressHandler(void)
{
	long_falg++;
	long_falg%=2;
}

void button_init(void)
{
	buttonInit(&button1,IDLE_LEVEL_LOW,20,100,getButtonVal);
	buttonLink(&button1,SINGLE_CLICK,singleClickHandler);
	buttonLink(&button1,DOUBLE_CLICK,doubleClickHandler);
	buttonLink(&button1,LONG_PRESS,longPressHandler);
	HAL_TIM_Base_Start_IT(&htim7);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM7)
    {
        buttonScan(&button1);
    }
}
