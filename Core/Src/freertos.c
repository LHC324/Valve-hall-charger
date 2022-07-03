/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "dac.h"
#include "iwdg.h"
#include "Flash.h"
#include "Dwin.h"
#include "mdrtuslave.h"
#include "ChargingHandle.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/**
 * @brief	Determine how the 4G module works
 * @details
 * @param	handler:modbus master/slave handle
 * @retval	true：MODBUS;fasle:shell
 */
bool Check_Mode(ModbusRTUSlaveHandler handler)
{
  ReceiveBufferHandle pB = handler->receiveBuffer;

  return ((pB->count == 1U) && (pB->buf[0] == ENTER_CODE));
}
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
osThreadId SamplingHandle;
osThreadId ChargerHandle;
osThreadId ReportHandle;
osThreadId UpdateHandle;
osThreadId IWDGHandle;
osThreadId DwinHandle;
osTimerId UserTimerHandle;
osTimerId ChargerTimeHandle;
osMutexId shellMutexHandle;
osSemaphoreId LTE_ReciveHandle;
osSemaphoreId Dwin_ReciveHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void Sampling_Task(void const *argument);
void Charger_Task(void const *argument);
void Report_Task(void const *argument);
void Update_Task(void const *argument);
void Iwdg_Task(void const *argument);
void Dwin_Task(void const *argument);
void User_Timer_Callback(void const *argument);
void ChargerTime_Callback(void const *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize);

/* GetTimerTaskMemory prototype (linked to static allocation support) */
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize);

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize)
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/* USER CODE BEGIN GET_TIMER_TASK_MEMORY */
static StaticTask_t xTimerTaskTCBBuffer;
static StackType_t xTimerStack[configTIMER_TASK_STACK_DEPTH];

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize)
{
  *ppxTimerTaskTCBBuffer = &xTimerTaskTCBBuffer;
  *ppxTimerTaskStackBuffer = &xTimerStack[0];
  *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
  /* place for user code */
}
/* USER CODE END GET_TIMER_TASK_MEMORY */

/**
 * @brief  FreeRTOS initialization
 * @param  None
 * @retval None
 */
void MX_FREERTOS_Init(void)
{
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* definition and creation of shellMutex */
  osMutexDef(shellMutex);
  shellMutexHandle = osMutexCreate(osMutex(shellMutex));

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* definition and creation of LTE_Recive */
  osSemaphoreDef(LTE_Recive);
  LTE_ReciveHandle = osSemaphoreCreate(osSemaphore(LTE_Recive), 1);

  /* definition and creation of Dwin_Recive */
  osSemaphoreDef(Dwin_Recive);
  Dwin_ReciveHandle = osSemaphoreCreate(osSemaphore(Dwin_Recive), 1);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* Create the timer(s) */
  /* definition and creation of UserTimer */
  osTimerDef(UserTimer, User_Timer_Callback);
  UserTimerHandle = osTimerCreate(osTimer(UserTimer), osTimerPeriodic, NULL);

  /* definition and creation of ChargerTime */
  osTimerDef(ChargerTime, ChargerTime_Callback);
  ChargerTimeHandle = osTimerCreate(osTimer(ChargerTime), osTimerPeriodic, NULL);

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  osTimerStart(UserTimerHandle, 1000);
  osTimerStart(ChargerTimeHandle, 1000);
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of Sampling */
  osThreadDef(Sampling, Sampling_Task, osPriorityNormal, 0, 256);
  SamplingHandle = osThreadCreate(osThread(Sampling), NULL);

  /* definition and creation of Charger */
  osThreadDef(Charger, Charger_Task, osPriorityAboveNormal, 0, 512);
  ChargerHandle = osThreadCreate(osThread(Charger), NULL);

  /* definition and creation of Report */
  osThreadDef(Report, Report_Task, osPriorityLow, 0, 256);
  ReportHandle = osThreadCreate(osThread(Report), NULL);

  /* definition and creation of Update */
  osThreadDef(Update, Update_Task, osPriorityHigh, 0, 256);
  UpdateHandle = osThreadCreate(osThread(Update), NULL);

  /* definition and creation of IWDG */
  osThreadDef(IWDG, Iwdg_Task, osPriorityRealtime, 0, 256);
  IWDGHandle = osThreadCreate(osThread(IWDG), NULL);

  /* definition and creation of Dwin */
  osThreadDef(Dwin, Dwin_Task, osPriorityNormal, 0, 256);
  DwinHandle = osThreadCreate(osThread(Dwin), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */
}

/* USER CODE BEGIN Header_Sampling_Task */
/**
 * @brief  Function implementing the Sampling thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_Sampling_Task */
void Sampling_Task(void const *argument)
{
  /* USER CODE BEGIN Sampling_Task */
  /* Infinite loop */
  for (;;)
  {
    Sampling_handle();
    osDelay(10);
  }
  /* USER CODE END Sampling_Task */
}

/* USER CODE BEGIN Header_Charger_Task */
/**
 * @brief Function implementing the Charger thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_Charger_Task */
void Charger_Task(void const *argument)
{
  /* USER CODE BEGIN Charger_Task */
  /* Infinite loop */
  for (;;)
  {
#if (DEBUGGING == 1)
    User_Debug();
    osDelay(4000);
#else
    Charger_Handle();
    osDelay(CHARGING_CYCLE);
#endif
  }
  /* USER CODE END Charger_Task */
}

/* USER CODE BEGIN Header_Report_Task */
/**
 * @brief Function implementing the Report thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_Report_Task */
void Report_Task(void const *argument)
{
  /* USER CODE BEGIN Report_Task */
  /* Infinite loop */
  for (;;)
  {
    Dwin_ReportHadle();
    osDelay(1000);
  }
  /* USER CODE END Report_Task */
}

/* USER CODE BEGIN Header_Update_Task */
/**
 * @brief Function implementing the Update thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_Update_Task */
void Update_Task(void const *argument)
{
  /* USER CODE BEGIN Update_Task */
  /* Infinite loop */
  for (;;)
  {
    /*https://www.cnblogs.com/w-smile/p/11333950.html*/
    if ((osOK == osSemaphoreWait(LTE_ReciveHandle, osWaitForever)) && Slave1_Object)
    {
      if (Check_Mode(Slave1_Object))
      {
#if defined(USING_DEBUG)
        shellPrint(Shell_Object, "About to enter upgrade mode .......\r\n");
#endif
        /*Clear mcp4822 output*/
        /*Clear DAC output*/
        HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 0);
        /*Switch to the upgrade page*/

        uint32_t update_flag = (*(__IO uint32_t *)UPDATE_SAVE_ADDRESS);

        if (((update_flag & 0xFFFF0000) >> 16U) == UPDATE_APP1)
        {
          update_flag = (((uint32_t)UPDATE_APP2 << 16U) | UPDATE_CMD);
        }
        else
        {
          update_flag = (((uint32_t)UPDATE_APP1 << 16U) | UPDATE_CMD);
        }
        taskENTER_CRITICAL();
        FLASH_Write(UPDATE_SAVE_ADDRESS, (uint16_t *)&update_flag, sizeof(update_flag));
        taskEXIT_CRITICAL();
        NVIC_SystemReset();
      }
      else
      {
        // mdRTU_Handler();
        mdRTU_Handler(Slave1_Object);
      }
    }
  }
  /* USER CODE END Update_Task */
}

/* USER CODE BEGIN Header_Iwdg_Task */
/**
 * @brief Function implementing the IWDG thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_Iwdg_Task */
void Iwdg_Task(void const *argument)
{
  /* USER CODE BEGIN Iwdg_Task */
  taskENTER_CRITICAL();
  FlashReadInit();
  taskEXIT_CRITICAL();
  osDelay(3000);
  Report_BackChargingData();
  MX_IWDG_Init();
  /* Infinite loop */
  for (;;)
  {
    HAL_IWDG_Refresh(&hiwdg);
    Flash_Operation();
    Clear_UserInfo();
    osDelay(100);
  }
  /* USER CODE END Iwdg_Task */
}

/* USER CODE BEGIN Header_Dwin_Task */
/**
 * @brief Function implementing the Dwin thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_Dwin_Task */
void Dwin_Task(void const *argument)
{
  /* USER CODE BEGIN Dwin_Task */
  /* Infinite loop */
  for (;;)
  {
    /*https://www.cnblogs.com/w-smile/p/11333950.html*/
    if (osOK == osSemaphoreWait(Dwin_ReciveHandle, osWaitForever))
    {
      /*Screen data distribution detected, close reporting*/
      DWIN_Poll();
#if defined(USING_DEBUG)
      // shellPrint(Shell_Object, "Dwin data!\r\n");
      // shellPrint(Shell_Object, "rx_count = %d\r\n", Dwin_Object->Slave.RxCount);
#endif
    }
  }
  /* USER CODE END Dwin_Task */
}

/* User_Timer_Callback function */
void User_Timer_Callback(void const *argument)
{
  /* USER CODE BEGIN User_Timer_Callback */
#define SET_TIMER_FLAG(Timer)   \
  do                            \
  {                             \
    if (!(--Timer.Timer8Count)) \
      Timer.Timer8Flag = true;  \
  } while (0)
  SET_TIMER_FLAG(g_Timer1);
  /* USER CODE END User_Timer_Callback */
}

/* ChargerTime_Callback function */
void ChargerTime_Callback(void const *argument)
{
  /* USER CODE BEGIN ChargerTime_Callback */
  ChargeTimer();
  SET_TIMER_FLAG(g_Timer);
  /* USER CODE END ChargerTime_Callback */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
