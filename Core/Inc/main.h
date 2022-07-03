/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2022 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
#define DEBUGGING 0
// #define USING_FREERTOS
#define USING_DMA
/*Custom memory management*/
#define CUSTOM_MALLOC pvPortMalloc
#define CUSTOM_FREE vPortFree
#define CURRENT_SOFT_VERSION 100
#define UPDATE_FLAG_FLASH_PAGE 125U
#define UPDATE_SAVE_ADDRESS (FLASH_BASE + UPDATE_FLAG_FLASH_PAGE * FLASH_PAGE_SIZE)
/*注意：此芯片flash大小�?256KB，定义地�?存在越界风险*/
// #define CHARGE_SAVE_ADDRESS                     0x0807F800
#define CHARGE_SAVE_ADDRESS (STM32FLASH_BASE + 126U * FLASH_PAGE_SIZE) //充电数据存盘地址(126�?)
/*ADC和DAC参数存储�?*/
#define CLIBRATION_SAVE_ADDR (STM32FLASH_BASE + 127U * FLASH_PAGE_SIZE)

#define UPDATE_CMD 0x1234
#define UPDATE_APP1 0x5AA5
#define UPDATE_APP2 0xA55A
#define ESC_CODE 0x1B
#define BACKSPACE_CODE 0x08
#define ENTER_CODE 0x0D
#define SPOT_CODE 0x2E
#define COMMA_CODE 0x2C
  /* USER CODE END ET */

  /* Exported constants --------------------------------------------------------*/
  /* USER CODE BEGIN EC */

  /* USER CODE END EC */

  /* Exported macro ------------------------------------------------------------*/
  /* USER CODE BEGIN EM */

  /* USER CODE END EM */

  /* Exported functions prototypes ---------------------------------------------*/
  void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define POWER_OK_Pin GPIO_PIN_12
#define POWER_OK_GPIO_Port GPIOD
#define LTE_NET_Pin GPIO_PIN_9
#define LTE_NET_GPIO_Port GPIOC
#define LTE_RESET_Pin GPIO_PIN_8
#define LTE_RESET_GPIO_Port GPIOA
#define LTE_RELOAD_Pin GPIO_PIN_11
#define LTE_RELOAD_GPIO_Port GPIOA
#define LTE_LINK_Pin GPIO_PIN_12
#define LTE_LINK_GPIO_Port GPIOA
#define POWER_ON_Pin GPIO_PIN_11
#define POWER_ON_GPIO_Port GPIOC
#define CHARGING_GO_Pin GPIO_PIN_12
#define CHARGING_GO_GPIO_Port GPIOC
#define FANS_POWER_Pin GPIO_PIN_0
#define FANS_POWER_GPIO_Port GPIOD
#define CHARGING_READY_Pin GPIO_PIN_1
#define CHARGING_READY_GPIO_Port GPIOD
#define Q04_Pin GPIO_PIN_2
#define Q04_GPIO_Port GPIOD
#define Q0_5_Pin GPIO_PIN_3
#define Q0_5_GPIO_Port GPIOD
#define DISCHARGING_SWITCH_Pin GPIO_PIN_4
#define DISCHARGING_SWITCH_GPIO_Port GPIOB
  /* USER CODE BEGIN Private defines */

  /* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
