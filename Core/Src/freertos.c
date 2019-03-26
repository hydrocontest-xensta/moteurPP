/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under Ultimate Liberty license
 * SLA0044, the "License"; You may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 *                             www.st.com/SLA0044
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "main.h"
#include "task.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "moteurpp.h"
#include "usart.h"
#include <stdio.h>
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
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
osThreadId defaultTaskHandle;
osThreadId getMessageTskHandle;
osSemaphoreId messageReceptionSemHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void
StartDefaultTask(void const* argument);
void
startGetMessageTk(void const* argument);

void
MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
 * @brief  FreeRTOS initialization
 * @param  None
 * @retval None
 */
void
MX_FREERTOS_Init(void)
{
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* definition and creation of messageReceptionSem */
  osSemaphoreDef(messageReceptionSem);
  messageReceptionSemHandle =
    osSemaphoreCreate(osSemaphore(messageReceptionSem), 1);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityAboveNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of getMessageTsk */
  osThreadDef(getMessageTsk, startGetMessageTk, osPriorityNormal, 0, 128);
  getMessageTskHandle = osThreadCreate(osThread(getMessageTsk), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */
}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void
StartDefaultTask(void const* argument)
{

  /* USER CODE BEGIN StartDefaultTask */
  char tick[] = "tick\r\n";
  /* Infinite loop */
  for (;;) {
    HAL_UART_Transmit(&huart2, (uint8_t*)tick, 6, 0xFFFF);
    HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
    osDelay(1000);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_startGetMessageTk */
/**
 * @brief Function implementing the getMessageTsk thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_startGetMessageTk */
void
startGetMessageTk(void const* argument)
{
  /* USER CODE BEGIN startGetMessageTk */
  char message[33];
  const char none_msg[] = "got invalid message";
  /* Infinite loop */

  for (;;) {
    // xSemaphoreTake(messageReceptionSemHandle);
    // int8_t msg_len = receiveMessage(message, 33);
    int8_t msg_len = -1;
    if (msg_len != -1) {
      char out_log[32];
      snprintf(out_log, 48, "got [%s]\r\n", message);
      HAL_UART_Transmit(&huart2, (uint8_t*)out_log, 32, 0xFF);
    } else {
      HAL_UART_Transmit(&huart2, (uint8_t*)none_msg, sizeof(none_msg), 0xFF);

    }

    osDelay(500);
  }
  /* USER CODE END startGetMessageTk */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
