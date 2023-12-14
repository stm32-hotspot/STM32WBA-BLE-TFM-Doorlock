/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    service1.h
  * @author  MCD Application Team
  * @brief   Header for service1.c
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#ifndef DLS_H
#define DLS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported defines ----------------------------------------------------------*/
/* USER CODE BEGIN ED */
#define TX_CHAR_SIZE (61)
#define RX_CHAR_SIZE (61)
  
#define OPCODE_SZ (1)
#define DLS_CHALLENGE_REQUEST_SZ (OPCODE_SZ)
#define DLS_CHALLENGE_SOLVED_SZ (OPCODE_SZ+CHALLENGE_SZ)
#define DLS_CHALLENGE_SEND_SZ (OPCODE_SZ+CHALLENGE_SZ)
#define DLS_DOOR_STATUS_SZ (OPCODE_SZ+1)
/* USER CODE END ED */

/* Exported types ------------------------------------------------------------*/
typedef enum
{
  DLS_CHALLENGE_INFO_REQUEST,
  DLS_CHALLENGE_INFO_RESPONSE,

  /* USER CODE BEGIN Service1_CharOpcode_t */

  /* USER CODE END Service1_CharOpcode_t */

  DLS_CHAROPCODE_LAST
} DLS_CharOpcode_t;

typedef enum
{
  DLS_CHALLENGE_INFO_REQUEST_WRITE_EVT,
  DLS_CHALLENGE_INFO_RESPONSE_NOTIFY_ENABLED_EVT,
  DLS_CHALLENGE_INFO_RESPONSE_NOTIFY_DISABLED_EVT,

  /* USER CODE BEGIN Service1_OpcodeEvt_t */

  /* USER CODE END Service1_OpcodeEvt_t */

  DLS_BOOT_REQUEST_EVT
} DLS_OpcodeEvt_t;

typedef struct
{
  uint8_t *p_Payload;
  uint8_t Length;

  /* USER CODE BEGIN Service1_Data_t */

  /* USER CODE END Service1_Data_t */

} DLS_Data_t;

typedef struct
{
  DLS_OpcodeEvt_t       EvtOpcode;
  DLS_Data_t             DataTransfered;
  uint16_t                ConnectionHandle;
  uint8_t                 ServiceInstance;

  /* USER CODE BEGIN Service1_NotificationEvt_t */

  /* USER CODE END Service1_NotificationEvt_t */

} DLS_NotificationEvt_t;

/* USER CODE BEGIN ET */
typedef enum
{
  //DLS_TX,
  DLS_CHALLENGE_REQUEST = 0x01,
  DLS_CHALLENGE_SOLVED = 0x02,
  DLS_CHALLENGE_SEND = 0x11,
  DLS_DOOR_STATUS = 0x12
    
} DLS_Opcode_t;
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* External variables --------------------------------------------------------*/
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/* Exported macros -----------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions ------------------------------------------------------- */
void DLS_Init(void);
void DLS_Notification(DLS_NotificationEvt_t *p_Notification);
tBleStatus DLS_UpdateValue(DLS_CharOpcode_t CharOpcode, DLS_Data_t *pData);
/* USER CODE BEGIN EF */

/* USER CODE END EF */

#ifdef __cplusplus
}
#endif

#endif /*DLS_H */
