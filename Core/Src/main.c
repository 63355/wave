/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "memorymap.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "button_user.h"
#include "my_adc.h"
#include "lcd.h"
#include <string.h>
#include "fft.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */



/**
 * @brief 显示波形（离散点或连线模式）
 * @param buffer 输入数据（范围0~79）
 * @param flag 0=离散点模式，1=连线模式
 * @param stop 是否停止绘制
 */
void show_wave(uint8_t* buffer, uint8_t flag, uint8_t stop,uint32_t color) {
    if (stop) return;

    // 清屏（全黑背景）
    ST7735_LCD_Driver.FillRect(&st7735_pObj, 0, 0, ST7735Ctx.Width, ST7735Ctx.Height, BLACK);

    if (flag) {
        //===== 连线模式（插值绘制线段）=====
        for (uint16_t num = 1; num < 160; num++) {
            int16_t y_prev = 79 - buffer[num - 1]; // 前一个点的Y坐标
            int16_t y_curr = 79 - buffer[num];     // 当前点的Y坐标
            
            // 计算两点间的垂直距离
            int16_t dy = y_curr - y_prev;
            
            // 如果距离>1，插值绘制中间点
            if (abs(dy) > 1) {
                // 插值步数（根据距离动态调整）
                uint8_t steps = abs(dy);
                for (uint8_t t = 1; t < steps; t++) {
                    int16_t y_interp = y_prev + (dy * t) / steps; // 线性插值
                    ST7735_SetPixel(&st7735_pObj, num - 1, y_interp, color);
                }
            }
            
            // 绘制当前点
            ST7735_SetPixel(&st7735_pObj, num, y_curr, color);
        }
    } else {
        //===== 离散点模式（直接绘制）=====
        for (uint16_t num = 0; num < 160; num++) {
            ST7735_SetPixel(&st7735_pObj, num, 79 - buffer[num], color);
        }
    }
}
void show_wave4(uint8_t* buffer, uint8_t flag, uint8_t stop,uint32_t color) {
    if (stop) return;

    // 清屏（全黑背景）
    ST7735_LCD_Driver.FillRect(&st7735_pObj, 0, 0, ST7735Ctx.Width, ST7735Ctx.Height, BLACK);

    if (flag) {
        //===== 连线模式（插值绘制线段）=====
        for (uint16_t num = 1; num < 160; num++) {
            int16_t y_prev = 79 - buffer[num - 1]/4; // 前一个点的Y坐标
            int16_t y_curr = 79 - buffer[num]/4;     // 当前点的Y坐标
            
            // 计算两点间的垂直距离
            int16_t dy = y_curr - y_prev;
            
            // 如果距离>1，插值绘制中间点
            if (abs(dy) > 1) {
                // 插值步数（根据距离动态调整）
                uint8_t steps = abs(dy);
                for (uint8_t t = 1; t < steps; t++) {
                    int16_t y_interp = y_prev + (dy * t) / steps; // 线性插值
                    ST7735_SetPixel(&st7735_pObj, num - 1, y_interp, color);
                }
            }
            
            // 绘制当前点
            ST7735_SetPixel(&st7735_pObj, num, y_curr, color);
        }
    } else {
        //===== 离散点模式（直接绘制）=====
        for (uint16_t num = 0; num < 160; num++) {
            ST7735_SetPixel(&st7735_pObj, num, 79 - buffer[num]/4, color);
        }
    }
}


/**
 * @brief 将uint32_t转换为十进制字符串
 * @param num 待转换的数字
 * @param str 输出缓冲区（至少11字节，包含结束符'\0'）
 * @return 字符串长度（不含结束符）
 */
uint8_t uint32_to_str_dec(uint32_t num, char* str) {
    char* p = str;
    uint32_t divisor = 1000000000; // 10^9

    // 处理0的特殊情况
    if (num == 0) {
        *p++ = '0';
        *p = '\0';
        return 1;
    }

    // 从最高位开始逐位提取
    uint8_t started = 0;
    while (divisor > 0) {
        uint8_t digit = num / divisor;
        if (digit > 0 || started) {
            *p++ = '0' + digit;
            started = 1;
        }
        num %= divisor;
        divisor /= 10;
    }

    *p = '\0';
    return p - str;
}
char buf[15];

#define BSP_LED_ON()     (GPIOE->BSRR = GPIO_BSRR_BS3)//高电平
#define BSP_LED_OFF()    (GPIOE->BSRR = GPIO_BSRR_BR3)//低电平	
#define BSP_LED_TOGGLE() (GPIOE->ODR ^= GPIO_ODR_OD3) 

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_TIM2_Init();
  MX_SPI4_Init();
  MX_TIM7_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
	button_init();
	LCD_Test();
	adc_init();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	if(adc_dma_finish_cnt >= 5)
	{
		BSP_LED_ON();
		
		adc_dma_finish_cnt = 0;
		

		
		//复制dma缓冲区到本地，防止被覆盖
		
        HAL_DMA_Start(&hdma_memtomem_dma2_stream1, (uint32_t)adc_buffer, (uint32_t)adc_buffer_copy, SAMPLING_DEPTH/4);
		hdma_memtomem_dma2_stream1.Lock = HAL_UNLOCKED;//解锁
		hdma_memtomem_dma2_stream1.State = HAL_DMA_STATE_READY;



//		for(uint32_t i=0; i<SAMPLING_DEPTH; i++)
//		adc_buffer_copy[i] = adc_buffer[i];
		

		//fft频谱数据分析
		fft_process(adc_buffer_copy, 1024, 2000000, &f, &a, db);
		
		if(long_falg)//切换显示信号频谱还是信号波形
		{
			show_wave4(db, double_flag, 0,RED);	
			uint32_to_str_dec(f, buf);
			LCD_ShowString(0, 0, 50, 40, 12, buf);
			LCD_ShowString(38, 0, 40, 40, 12, "HZ");
			uint32_to_str_dec(a*1000, buf);
			LCD_ShowString(60, 0, 40, 40, 12, buf);
			LCD_ShowString(85, 0, 40, 40, 12, "mV");			
		}
		else
		{
			if(single_flag)//auto算法
			{
//				num++;
//				static uint16_t ff;
//				if(num==1)
//				{
					uint16_t ff = (uint16_t)((50000.0f/f) + 1);
//					if(ff>=63)ff=63;
//				}

				static uint8_t temp[10240]={0};
				uint16_t num = 10240/ff;
				
				//根据频率和幅值，压缩原始信号，到buffer_show[160]
				for(uint16_t i=0;i<num;i++)
				{
					temp[i] = (uint8_t)((float)adc_buffer_copy[i*(uint16_t)ff]/(1+(a-0.7f)*1.2f));
				}
				for(uint16_t i=0; i<num; i++)
				{
					static uint8_t flag1 = 0;
					uint16_t i_temp = 0;
					
					if(flag1 == 0)
					{
						// 修复1：补全if条件括号
						if( ((temp[i+1] - temp[i]) > 10) || ((temp[i] > 38) && (temp[i] < 42) && (temp[i] < temp[i + 1])) ) // 找到触发点
						{
							flag1 = 1;
							i_temp = i;
						}
					}
					
					if(flag1 == 1) // 找到触发点，向右取值160个点
					{
						// 修复2：添加边界检查
						for(uint16_t j=0; j<160 && (i_temp+j)<num; j++)
						{
							buffer_show[j] = temp[j+i_temp];
						}
						flag1 = 0;
						break;
					}
				}
				
				show_wave(buffer_show, double_flag, 0,LIGHTBLUE);	
				uint32_to_str_dec(f, buf);
				LCD_ShowString(0, 0, 40, 40, 12, buf);
				LCD_ShowString(38, 0, 40, 40, 12, "HZ");
				uint32_to_str_dec(a*1000, buf);
				LCD_ShowString(60, 0, 40, 40, 12, buf);
				LCD_ShowString(85, 0, 40, 40, 12, "mV");	
			}
			else
			{
				show_wave(adc_buffer_copy, double_flag, 0,WHITE);	
				uint32_to_str_dec(f, buf);
				LCD_ShowString(0, 0, 40, 40, 12, buf);
				LCD_ShowString(38, 0, 40, 40, 12, "HZ");
				uint32_to_str_dec(a*1000, buf);
				LCD_ShowString(60, 0, 40, 40, 12, buf);
				LCD_ShowString(85, 0, 40, 40, 12, "mV");
			}
		}
		BSP_LED_OFF();
	}
		
	 // HAL_Delay(adc_dma_finish_cnt);

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
