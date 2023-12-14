/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    service1_app.c
  * @author  MCD Application Team
  * @brief   service1_app application definition.
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

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "app_common.h"
#include "app_ble.h"
//#include "ll_sys_if.h"
#include "dbg_trace.h"
#include "ble.h"
#include "dls_app.h"
#include "dls.h"
#include "stm32_seq.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32wbaxx_nucleo.h"
#include "tfm_doorlock_service_api.h"
#include "tfm_doorlock_service_api_common.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/

/* USER CODE BEGIN PTD */
static void DLS_Challenge_info_response_RecvData();
static void DLS_Doorlock_timer_callback();
/* USER CODE END PTD */

typedef enum
{
  Challenge_info_response_NOTIFICATION_OFF,
  Challenge_info_response_NOTIFICATION_ON,
  /* USER CODE BEGIN Service1_APP_SendInformation_t */

  /* USER CODE END Service1_APP_SendInformation_t */
  DLS_APP_SENDINFORMATION_LAST
} DLS_APP_SendInformation_t;

typedef struct
{
  DLS_APP_SendInformation_t     Challenge_info_response_Notification_Status;
  /* USER CODE BEGIN Service1_APP_Context_t */

  /* USER CODE END Service1_APP_Context_t */
  uint16_t              ConnectionHandle;
} DLS_APP_Context_t;

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* External variables --------------------------------------------------------*/
/* USER CODE BEGIN EV */
static uint8_t DLS_app_RxCharData[RX_CHAR_SIZE];
static uint32_t DLS_app_RxCharData_sz;
/* USER CODE END EV */

/* Private macros ------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
static DLS_APP_Context_t DLS_APP_Context;

uint8_t a_DLS_UpdateCharData[247];

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static void DLS_Challenge_info_response_SendNotification(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
void DLS_Notification(DLS_NotificationEvt_t *p_Notification)
{
  /* USER CODE BEGIN Service1_Notification_1 */

  /* USER CODE END Service1_Notification_1 */
  switch(p_Notification->EvtOpcode)
  {
    /* USER CODE BEGIN Service1_Notification_Service1_EvtOpcode */

    /* USER CODE END Service1_Notification_Service1_EvtOpcode */

    case DLS_CHALLENGE_INFO_REQUEST_WRITE_EVT:
      /* USER CODE BEGIN Service1Char1_WRITE_EVT */
      if(p_Notification->DataTransfered.Length <= RX_CHAR_SIZE){
              DLS_app_RxCharData_sz = p_Notification->DataTransfered.Length;
              memcpy(DLS_app_RxCharData, p_Notification->DataTransfered.p_Payload, DLS_app_RxCharData_sz);
              UTIL_SEQ_SetTask(1<<CFG_TASK_DLS_INFO_RESP_RX_ID, CFG_SEQ_PRIO_0);
      }else{
        APP_DBG_MSG("Fail to process received fram too big: %u > %u\n\r", p_Notification->DataTransfered.Length, RX_CHAR_SIZE);
      }
      /* USER CODE END Service1Char1_WRITE_EVT */
      break;

    case DLS_CHALLENGE_INFO_RESPONSE_NOTIFY_ENABLED_EVT:
      /* USER CODE BEGIN Service1Char2_NOTIFY_ENABLED_EVT */

      /* USER CODE END Service1Char2_NOTIFY_ENABLED_EVT */
      break;

    case DLS_CHALLENGE_INFO_RESPONSE_NOTIFY_DISABLED_EVT:
      /* USER CODE BEGIN Service1Char2_NOTIFY_DISABLED_EVT */

      /* USER CODE END Service1Char2_NOTIFY_DISABLED_EVT */
      break;

    default:
      /* USER CODE BEGIN Service1_Notification_default */

      /* USER CODE END Service1_Notification_default */
      break;
  }
  /* USER CODE BEGIN Service1_Notification_2 */

  /* USER CODE END Service1_Notification_2 */
  return;
}

void DLS_APP_EvtRx(DLS_APP_ConnHandleNotEvt_t *p_Notification)
{
  /* USER CODE BEGIN Service1_APP_EvtRx_1 */

  /* USER CODE END Service1_APP_EvtRx_1 */

  switch(p_Notification->EvtOpcode)
  {
    /* USER CODE BEGIN Service1_APP_EvtRx_Service1_EvtOpcode */

    /* USER CODE END Service1_Notification_Service1_EvtOpcode */
    case DLS_CONN_HANDLE_EVT :
      /* USER CODE BEGIN Service1_APP_CONN_HANDLE_EVT */
      APP_DBG_MSG("DLS_CONN_HANDLE_EVT\n");
      /* USER CODE END Service1_APP_CONN_HANDLE_EVT */
      break;

    case DLS_DISCON_HANDLE_EVT :
      /* USER CODE BEGIN Service1_APP_DISCON_HANDLE_EVT */
      APP_DBG_MSG("DLS_DISCON_HANDLE_EVT\n");
      /* USER CODE END Service1_APP_DISCON_HANDLE_EVT */
      break;

    default:
      /* USER CODE BEGIN Service1_APP_EvtRx_default */

      /* USER CODE END Service1_APP_EvtRx_default */
      break;
  }

  /* USER CODE BEGIN Service1_APP_EvtRx_2 */

  /* USER CODE END Service1_APP_EvtRx_2 */

  return;
}

void DLS_APP_Init(void)
{
  UNUSED(DLS_APP_Context);
  DLS_Init();

  /* USER CODE BEGIN Service1_APP_Init */
  UTIL_SEQ_RegTask(1<<CFG_TASK_DLS_INFO_RESP_RX_ID, UTIL_SEQ_RFU, DLS_Challenge_info_response_RecvData);
  UTIL_SEQ_RegTask(1<<CFG_TASK_DLS_DOORLOCK_TIMER_ID, UTIL_SEQ_RFU, DLS_Doorlock_timer_callback);
  /* USER CODE END Service1_APP_Init */
  return;
}

/* USER CODE BEGIN FD */

/* USER CODE END FD */

/*************************************************************
 *
 * LOCAL FUNCTIONS
 *
 *************************************************************/
__USED void DLS_Challenge_info_response_SendNotification(void) /* Property Notification */
{
  DLS_APP_SendInformation_t notification_on_off = Challenge_info_response_NOTIFICATION_OFF;
  DLS_Data_t dls_notification_data;

  dls_notification_data.p_Payload = (uint8_t*)a_DLS_UpdateCharData;
  dls_notification_data.Length = 0;

  /* USER CODE BEGIN Service1Char2_NS_1*/

  /* USER CODE END Service1Char2_NS_1*/

  if (notification_on_off != Challenge_info_response_NOTIFICATION_OFF)
  {
    DLS_UpdateValue(DLS_CHALLENGE_INFO_RESPONSE, &dls_notification_data);
  }

  /* USER CODE BEGIN Service1Char2_NS_Last*/

  /* USER CODE END Service1Char2_NS_Last*/

  return;
}

/* USER CODE BEGIN FD_LOCAL_FUNCTIONS*/
__USED void DLS_Challenge_info_response_SendNotificationUser(uint8_t *data, uint32_t data_len) /* Property Notification */
{
  DLS_Data_t dls_notification_data;
  
  APP_DBG_MSG("\r\nDLS_TxSendNotification of size: %u\r\n", data_len);
  for(int i=0; i<data_len; i++){
    APP_DBG_MSG("%x", data[i]);
  }
  APP_DBG_MSG("\r\n");


  dls_notification_data.p_Payload = (uint8_t*)data;
  dls_notification_data.Length = data_len;
  

  /* USER CODE BEGIN Service1Char1_NS_1*/
  DLS_UpdateValue(DLS_CHALLENGE_INFO_RESPONSE, &dls_notification_data);
  /* USER CODE END Service1Char1_NS_1*/

  /* USER CODE BEGIN Service1Char1_NS_Last*/

  /* USER CODE END Service1Char1_NS_Last*/

  return;
}


void DLS_Challenge_info_response_SendDoorStatus(uint8_t door_status){
  uint32_t msg_sz = 0;
    
  a_DLS_UpdateCharData[0] = DLS_DOOR_STATUS;
  a_DLS_UpdateCharData[1] = door_status;
  msg_sz = 2;
  
  if(msg_sz != DLS_DOOR_STATUS_SZ){
    APP_DBG_MSG("DLS_Tx_SendDoorStatus, wrong send size\n");
  }
  
  DLS_Challenge_info_response_SendNotificationUser((uint8_t*)a_DLS_UpdateCharData, msg_sz);
}

static void DLS_Challenge_info_response_SendChallenge(uint8_t *challenge, uint32_t challenge_sz){
  uint32_t msg_sz = 0;
  /* The challenge and the opcode */
  msg_sz = challenge_sz+1;
 
  if(msg_sz != DLS_CHALLENGE_SEND_SZ){
    APP_DBG_MSG("DLS_Tx_SendChallenge, wrong send size\n");
    return;
  }
  
  a_DLS_UpdateCharData[0] = DLS_CHALLENGE_SEND;
  memcpy(&a_DLS_UpdateCharData[1], challenge, challenge_sz);
  
  DLS_Challenge_info_response_SendNotificationUser((uint8_t*)a_DLS_UpdateCharData, msg_sz);
}

static void DLS_Challenge_info_response_RecvChallengeRequest(){
  psa_status_t psa_status;
  uint32_t challenge_out_buf_sz;
  uint8_t challenge_out[CHALLENGE_SZ];
  
  psa_status = psa_doorlock_request_open(challenge_out, IN_OUT_BUF_SZ, &challenge_out_buf_sz);
  
  if(psa_status != PSA_SUCCESS){
    APP_DBG_MSG("psa_doorlock_answer_challenge returned %x\n", psa_status);
    return;
  }
  if(challenge_out_buf_sz != CHALLENGE_SZ){
    APP_DBG_MSG("psa_doorlock_request_open returned a size of %u != %u\n", challenge_out_buf_sz, CHALLENGE_SZ);
    return;
  }
    
  DLS_Challenge_info_response_SendChallenge(challenge_out, challenge_out_buf_sz);
  
}

static void DLS_Challenge_info_response_RecvChallengeSolved(uint8_t *payload, uint32_t payload_len){
  psa_status_t psa_status;
  uint8_t door_status;
  
  if(payload_len != (CHALLENGE_SZ)){
    APP_DBG_MSG("DLS_Rx_ChallengeSolved: payload has the wrong size: %u expected %u\r\n", payload_len, CHALLENGE_SZ);
  }
  
  psa_status = psa_doorlock_answer_challenge(payload, payload_len, &door_status);
  
  if(psa_status != PSA_SUCCESS){
    APP_DBG_MSG("psa_doorlock_answer_challenge returned %x\n", psa_status);
    return;
  }
  
  DLS_Challenge_info_response_SendDoorStatus(door_status);
  
}

/* USER CODE BEGIN FD_LOCAL_FUNCTIONS*/
static void DLS_Challenge_info_response_RecvData(void *arg){
  
  uint8_t opcode;

#if DEBUG_DLS
  APP_DBG_MSG("DLS_Rx_Data running : %u\n", DLS_app_RxCharData_sz);
  for(int i=0; i<DLS_app_RxCharData_sz;  i++){
    APP_DBG_MSG("%x", DLS_app_RxCharData[i]);
  }
  APP_DBG_MSG("\n");
#endif /*DEBUG_DLS*/
  
  if(DLS_app_RxCharData_sz == 0){
    APP_DBG_MSG("DLS_Rx_Data error, received size null\n");
    return;
  }
  
  opcode = DLS_app_RxCharData[0];
  switch(opcode){
    
  case DLS_CHALLENGE_REQUEST:
    {
      if(DLS_app_RxCharData_sz != DLS_CHALLENGE_REQUEST_SZ){
        APP_DBG_MSG("The payload size doesn't match with the DLS_CHALLENGE_REQUEST opcode: %u != %u\n", DLS_app_RxCharData_sz, DLS_CHALLENGE_REQUEST_SZ);
        return;
      }
      DLS_Challenge_info_response_RecvChallengeRequest();
    }break;
    
  case DLS_CHALLENGE_SOLVED:
    {
      if(DLS_app_RxCharData_sz != DLS_CHALLENGE_SOLVED_SZ){
        APP_DBG_MSG("The payload size doesn't match with the DLS_CHALLENGE_SOLVED opcode: %u != %u\n", DLS_app_RxCharData_sz, DLS_CHALLENGE_SOLVED_SZ);
        return;
      }
      DLS_Challenge_info_response_RecvChallengeSolved(&DLS_app_RxCharData[1], DLS_app_RxCharData_sz-1);
    }break;
    
  default:
    APP_DBG_MSG("Opcode not recognized\n");
    break;
  }
}

static void DLS_Doorlock_timer_callback(void *arg){
  DLS_Challenge_info_response_SendDoorStatus(0);
}
/* USER CODE END FD_LOCAL_FUNCTIONS*/
