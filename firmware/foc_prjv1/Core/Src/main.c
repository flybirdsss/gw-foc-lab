/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "cmsis_os.h"
#include "adc.h"
#include "comp.h"
#include "fdcan.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "drv8323.h"
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
HAL_StatusTypeDef write_status;

uint16_t test_read;

uint16_t drv_tx_debug;

uint16_t test_read1;
uint16_t test_read2;

uint8_t fault_state;



uint16_t test_read3;
uint16_t test_read4;
uint16_t test_read5;


HAL_StatusTypeDef write_status;
uint16_t write_readback;
uint16_t restore_readback;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void Fix_Boot_OptionBytes(void)
{
    FLASH_OBProgramInitTypeDef ob = {0};

    /* 已经是目标配置就不再写 */
    if (((FLASH->OPTR & FLASH_OPTR_nSWBOOT0) == 0U) &&
        ((FLASH->OPTR & FLASH_OPTR_nBOOT0) != 0U))
    {
        return;
    }

    if (HAL_FLASH_Unlock() != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_FLASH_OB_Unlock() != HAL_OK)
    {
        Error_Handler();
    }

    ob.OptionType = OPTIONBYTE_USER;

    ob.USERType =
        OB_USER_nSWBOOT0 |
        OB_USER_nBOOT0;

    ob.USERConfig =
        OB_BOOT0_FROM_OB |
        OB_nBOOT0_SET;

			volatile HAL_StatusTypeDef ob_status;

			ob_status = HAL_FLASHEx_OBProgram(&ob);

			__NOP();   // 在这里打断点

			if (ob_status != HAL_OK)
			{
					Error_Handler();
			}

    /*
     * 重新加载 Option Bytes。
     * 这里会触发系统复位，正常情况下不会返回。
     */
    HAL_FLASH_OB_Launch();

    while (1)
    {
    }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

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
  MX_SPI1_Init();
  MX_TIM1_Init();
  MX_I2C3_Init();
  MX_SPI3_Init();
  MX_USART1_UART_Init();
  MX_ADC2_Init();
  MX_ADC1_Init();
//  MX_FDCAN1_Init();
//  MX_USB_PCD_Init();
  MX_COMP1_Init();
  MX_COMP2_Init();
  MX_COMP3_Init();
	GPIO_InitTypeDef GPIO_InitStruct = {0};

GPIO_InitStruct.Pin = GPIO_PIN_4;
GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
GPIO_InitStruct.Pull = GPIO_PULLUP;
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;

HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

/* USER CODE BEGIN 2 */

/* SPI片选默认保持高电平 */
HAL_GPIO_WritePin(SPI1_CS_GPIO_Port,
                  SPI1_CS_Pin,
                  GPIO_PIN_SET);

/* 关闭电流采样放大器校准 */
HAL_GPIO_WritePin(CAL_GPIO_Port,
                  CAL_Pin,
                  GPIO_PIN_RESET);

/* 使能DRV8323 */
HAL_GPIO_WritePin(DRV_EN_GPIO_Port,
                  DRV_EN_Pin,
                  GPIO_PIN_SET);

/* 等待DRV8323启动稳定 */
HAL_Delay(20);


/* ================= SPI读取测试 ================= */

DRV8323_ReadReg(DRV8323_REG_CTRL, &test_read1);
HAL_Delay(1);

DRV8323_ReadReg(DRV8323_REG_GATE_HS, &test_read2);
HAL_Delay(1);

DRV8323_ReadReg(DRV8323_REG_GATE_LS, &test_read3);
HAL_Delay(1);

DRV8323_ReadReg(DRV8323_REG_OCP, &test_read4);
HAL_Delay(1);

DRV8323_ReadReg(DRV8323_REG_CSA, &test_read5);
HAL_Delay(1);

/* 只保留低11位 */
test_read1 &= DRV8323_DATA_MASK;
test_read2 &= DRV8323_DATA_MASK;
test_read3 &= DRV8323_DATA_MASK;
test_read4 &= DRV8323_DATA_MASK;
test_read5 &= DRV8323_DATA_MASK;


__NOP();    // 在这里打断点
/* ========== SPI写入/读回测试 ========== */

/* 1. 临时把 GATE_HS 从 0x3FF 改成 0x3FE */
write_status = DRV8323_WriteReg(DRV8323_REG_GATE_HS,
                                0x3FE);

HAL_Delay(1);

/* 2. 读回来 */
DRV8323_ReadReg(DRV8323_REG_GATE_HS,
                &write_readback);

write_readback &= DRV8323_DATA_MASK;


/* 3. 恢复原来的 0x3FF */
DRV8323_WriteReg(DRV8323_REG_GATE_HS,
                 0x3FF);

HAL_Delay(1);

/* 4. 再读一次确认恢复 */
DRV8323_ReadReg(DRV8323_REG_GATE_HS,
                &restore_readback);

restore_readback &= DRV8323_DATA_MASK;

__NOP();     /* 这里打断点 */
/* =================================================
 * 无电机：三相同相 PWM 短时间测试
 * ================================================= */

/* 先关闭DRV输出 */
HAL_GPIO_WritePin(DRV_EN_GPIO_Port,
                  DRV_EN_Pin,
                  GPIO_PIN_RESET);

/* 三相约50%占空比 */
__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 2125);
__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 2125);
__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 2125);

/* 启动三个主通道 */
HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);

/* 启动三个互补通道 */
HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);
HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2);
HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_3);

/* PWM已经稳定，再打开DRV */
HAL_Delay(1);

HAL_GPIO_WritePin(DRV_EN_GPIO_Port,
                  DRV_EN_Pin,
                  GPIO_PIN_SET);

/* 实际驱动5ms */
HAL_Delay(5);


/* PWM仍在运行时读取故障 */
DRV8323_ReadReg(DRV8323_REG_FAULT1, &test_read1);
DRV8323_ReadReg(DRV8323_REG_FAULT2, &test_read2);

test_read1 &= DRV8323_DATA_MASK;
test_read2 &= DRV8323_DATA_MASK;


/* 先关DRV功率输出 */
HAL_GPIO_WritePin(DRV_EN_GPIO_Port,
                  DRV_EN_Pin,
                  GPIO_PIN_RESET);

/* 再停止六路PWM */
HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2);
HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_3);

HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_1);
HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_2);
HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_3);

__NOP();     /* 这里打断点 */

while (1)
{
}
/* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();  /* Call init function for freertos objects (in freertos.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV6;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV6;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
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
