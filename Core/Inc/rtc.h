/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    rtc.h
 * @brief   This file contains all the function prototypes for
 *          the rtc.c file
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
#ifndef __RTC_H__
#define __RTC_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

  /* USER CODE BEGIN Includes */

  /* USER CODE END Includes */

  extern RTC_HandleTypeDef hrtc;

  /* USER CODE BEGIN Private defines */
  extern RTC_DateTypeDef sdatestructure;
  extern RTC_TimeTypeDef stimestructure;
  /* USER CODE END Private defines */

  void MX_RTC_Init(void);

  /* USER CODE BEGIN Prototypes */
  uint32_t time2Stamp(RTC_DateTypeDef date, RTC_TimeTypeDef time);
  void Stamp2time(RTC_DateTypeDef *pdate, RTC_TimeTypeDef *ptime, uint32_t Stamp, int timezone);
  void myRtc_Get_DateTime(RTC_DateTypeDef *pdate, RTC_TimeTypeDef *ptime);
  void SetRtcCount(uint32_t tramp);
  /* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __RTC_H__ */
