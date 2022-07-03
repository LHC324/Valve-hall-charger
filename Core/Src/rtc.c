/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    rtc.c
 * @brief   This file provides code for the configuration
 *          of the RTC instances.
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
/* Includes ------------------------------------------------------------------*/
#include "rtc.h"

/* USER CODE BEGIN 0 */
RTC_TimeTypeDef stimestructure = {0};
RTC_DateTypeDef sdatestructure = {0};
/* USER CODE END 0 */

RTC_HandleTypeDef hrtc;

/* RTC init function */
void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef DateToUpdate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
   */
  hrtc.Instance = RTC;
  hrtc.Init.AsynchPrediv = 32767;
  hrtc.Init.OutPut = RTC_OUTPUTSOURCE_ALARM;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */
  /*Check whether RTC is configured for the first time*/
  if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1) == 0x5050)
  {
    /*Update time*/
    myRtc_Get_DateTime(&sdatestructure, &stimestructure);
    return;
  }
  else
  {
    /* USER CODE END Check_RTC_BKUP */

    /** Initialize RTC and set the Time and Date
     */
    sTime.Hours = 12;
    sTime.Minutes = 30;
    sTime.Seconds = 0;

    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
    {
      Error_Handler();
    }
    DateToUpdate.WeekDay = RTC_WEEKDAY_MONDAY;
    DateToUpdate.Month = RTC_MONTH_JUNE;
    DateToUpdate.Date = 23;
    DateToUpdate.Year = 22;

    if (HAL_RTC_SetDate(&hrtc, &DateToUpdate, RTC_FORMAT_BIN) != HAL_OK)
    {
      Error_Handler();
    }
    /* USER CODE BEGIN RTC_Init 2 */
    SetRtcCount(time2Stamp(DateToUpdate, sTime));
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0x5050);
  }
  /* USER CODE END RTC_Init 2 */
}

void HAL_RTC_MspInit(RTC_HandleTypeDef *rtcHandle)
{

  if (rtcHandle->Instance == RTC)
  {
    /* USER CODE BEGIN RTC_MspInit 0 */

    /* USER CODE END RTC_MspInit 0 */
    HAL_PWR_EnableBkUpAccess();
    /* Enable BKP CLK enable for backup registers */
    __HAL_RCC_BKP_CLK_ENABLE();
    /* RTC clock enable */
    __HAL_RCC_RTC_ENABLE();
    /* USER CODE BEGIN RTC_MspInit 1 */

    /* USER CODE END RTC_MspInit 1 */
  }
}

void HAL_RTC_MspDeInit(RTC_HandleTypeDef *rtcHandle)
{

  if (rtcHandle->Instance == RTC)
  {
    /* USER CODE BEGIN RTC_MspDeInit 0 */

    /* USER CODE END RTC_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_RTC_DISABLE();
    /* USER CODE BEGIN RTC_MspDeInit 1 */

    /* USER CODE END RTC_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
/*
 * Judge leap year and average year
 */
bool IsLeapYear(uint16_t year)
{
  return ((bool)(((year) % 4 == 0 && (year) % 100 != 0) || (year) % 400 == 0));
}

/*
 *  Function function: get the week according to the specific date
 */
void Get_Weak(RTC_DateTypeDef *time)
{
  uint16_t YY = 0;
  uint8_t MM = 0;

  if (time->Month == 1 || time->Month == 2)
  {
    MM = time->Month + 12;
    YY = time->Year - 1 + 2000;
  }
  else
  {
    MM = time->Month;
    YY = time->Year + 2000;
  }

  time->WeekDay = ((time->Date + 2 * MM + 3 * (MM + 1) / 5 + YY + YY / 4 - YY / 100 + YY / 400) % 7) + 1;
} //(29 + 16 + 5 + 2018 +2018/4 - 2018/100 + 2018/400)%7
//(29 + 16 + 5 + 18 +18/4 - 18/100 + 18/400)%7

/*
 *  Beijing time to timestamp
 */
uint32_t time2Stamp(RTC_DateTypeDef date, RTC_TimeTypeDef time)
{
  uint32_t result;
  uint16_t monDays[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
  uint16_t Year = date.Year + 2000;
  result = (Year - 1970) * 365 * 24 * 3600 + (monDays[date.Month - 1] + date.Date - 1) * 24 * 3600 + (time.Hours - 8) * 3600 + time.Minutes * 60 + time.Seconds;

  result += (date.Month > 2 && (Year % 4 == 0) && (Year % 100 != 0 || Year % 400 == 0)) * 24 * 3600; //闰月

  Year -= 1970;
  result += (Year / 4 - Year / 100 + Year / 400) * 24 * 3600; //闰年
  return result;
}

/*
 *    Timestamp to time
 */
void Stamp2time(RTC_DateTypeDef *pdate, RTC_TimeTypeDef *ptime, uint32_t Timestamp, int timezone)
{

  uint16_t year = 1970;
  uint32_t Counter = 0, CounterTemp; //随着年份迭加，Counter记录￿??1970 ￿?? 1 ￿?? 1 日（00:00:00 GMT）到累加到的年份的最后一天的秒数
  uint8_t Month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  uint8_t i;

  Timestamp = Timestamp + timezone * 3600; //修正时区

  while (Counter <= Timestamp) //假设今天￿??2018年某￿??天，则时间戳应小于等￿??1970-1-1 0:0:0 ￿?? 2018-12-31 23:59:59的�?�秒￿??
  {
    CounterTemp = Counter; // CounterTemp记录完全1970-1-1 0:0:0 ￿?? 2017-12-31 23:59:59的�?�秒数后￿??出循￿??
    Counter += 31536000;   //加上今年（平年）的秒￿??

    if (IsLeapYear(year))
    {
      Counter += 86400; //闰年多加￿??￿??
    }

    year++;
  }

  pdate->Year = year - 1 - 2000; //跳出循环即表示到达计数�?�当前年
  Month[1] = (IsLeapYear(year - 1) ? 29 : 28);
  Counter = Timestamp - CounterTemp;    // Counter = Timestamp - CounterTemp  记录2018年已走的总秒￿??
  CounterTemp = Counter / 86400;        // CounterTemp = Counter/(24*3600)  记录2018年已【过去�?�天￿??
  Counter -= CounterTemp * 86400;       //记录今天已走的�?�秒￿??
  ptime->Hours = Counter / 3600;        //￿??                       得到今天的小￿??
  ptime->Minutes = Counter % 3600 / 60; //￿??
  ptime->Seconds = Counter % 60;        //￿??

  for (i = 0; i < 12; i++)
  {
    if (CounterTemp < Month[i]) //不能包含相等的情况，相等会导致最后一天切换到下一个月第一天时
    {
      //（即CounterTemp已走天数刚好为n个月完整天数相加时（31+28+31...））￿??
      pdate->Month = i + 1;          // 月份不加1，日期溢出（如：出现32号）
      pdate->Date = CounterTemp + 1; //应不作处理，CounterTemp = Month[i] = 31时，会继续循环，月份加一￿??
      break;                         //日期变为1，刚好符合实际日￿??
    }

    CounterTemp -= Month[i];
  }

  Get_Weak(pdate);
}

/*
 *     获取当前时间
 */
void myRtc_Get_DateTime(RTC_DateTypeDef *pdate, RTC_TimeTypeDef *ptime)
{
  uint32_t timecount = 0;
  timecount = RTC->CNTH; //得到计数器中的�??(秒钟￿?)
  timecount <<= 16;
  timecount += RTC->CNTL;

  Stamp2time(pdate, ptime, timecount, 8);
}

/*
 * 设置时钟计数器忿
 */
void SetRtcCount(uint32_t tramp)
{
  //设置时钟
  RCC->APB1ENR |= 1 << 28; //使能电源时钟
  RCC->APB1ENR |= 1 << 27; //使能备份时钟
  PWR->CR |= 1 << 8;       //取消备份区写保护
  //上面三步是必须的!
  RTC->CRL |= 1 << 4; //允许配置
  RTC->CNTL = tramp & 0xffff;
  RTC->CNTH = tramp >> 16;
  RTC->CRL &= ~(1 << 4); //配置更新

  while (!(RTC->CRL & (1 << 5)))
    ; //等待RTC寄存器操作完￿??
}
/* USER CODE END 1 */
