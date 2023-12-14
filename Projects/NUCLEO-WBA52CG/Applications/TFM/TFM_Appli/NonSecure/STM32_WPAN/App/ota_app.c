/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    STM32_WPAN
  * @author  MCD Application Team
  * @brief   STM32_WPAN application definition.
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
#include "main.h"
#include "app_common.h"
#include "app_ble.h"
#include "dbg_trace.h"
#include "ble.h"
#include "ota_app.h"
#include "ota.h"
#include "stm32_seq.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32_timer.h"
#include "flash_driver.h"
#include "flash_manager.h"
#include "secure_data_update.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/

/* USER CODE BEGIN PTD */
typedef enum
{
  User_Conf,
  Fw_App,
} OTA_APP_FileType_t;

/* USER CODE END PTD */

typedef enum
{
  Conf_INDICATION_OFF,
  Conf_INDICATION_ON,
  /* USER CODE BEGIN Service3_APP_SendInformation_t */
  OTA_APP_No_Pending,
  OTA_APP_Pending,
  OTA_APP_Ready_Pending,
  /* USER CODE END Service3_APP_SendInformation_t */
  OTA_APP_SENDINFORMATION_LAST
} OTA_APP_SendInformation_t;

typedef struct
{
  OTA_APP_SendInformation_t     Conf_Indication_Status;
  /* USER CODE BEGIN Service3_APP_Context_t */
  uint32_t base_address;
  uint8_t sectors;
  uint32_t write_value[60];
  uint8_t  write_value_index;
  uint8_t  file_type;
  /* USER CODE END Service3_APP_Context_t */
  uint16_t              ConnectionHandle;
} OTA_APP_Context_t;

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* External variables --------------------------------------------------------*/
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/* Private macros ------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define CFG_DOWNLOAD_ACTIVE_NB_SECTORS (1)
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/**
 * START of Section BLE_APP_CONTEXT
 */

PLACE_IN_SECTION("BLE_APP_CONTEXT") static OTA_APP_Context_t OTA_APP_Context;

/**
 * END of Section BLE_APP_CONTEXT
 */

uint8_t a_OTA_UpdateCharData[247];

/* USER CODE BEGIN PV */
static uint32_t size_left;
static uint32_t address_offset;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static void OTA_Conf_SendIndication(void);

/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
void OTA_Notification(OTA_NotificationEvt_t *p_Notification)
{
  /* USER CODE BEGIN Service3_Notification_1 */
  OTA_Data_t msg_conf;
  APP_DBG("p_Notification->EvtOpcode: %x\r\n", p_Notification->EvtOpcode);
  /* USER CODE END Service3_Notification_1 */
  switch(p_Notification->EvtOpcode)
  {
    /* USER CODE BEGIN Service3_Notification_Service3_EvtOpcode */
    case OTA_CONF_EVT:
      {
        APP_DBG("OTA_CONF_EVT\r\n");
        /**
         * The Remote notifies it has send all the data to be written in Flash
         */
        /**
         * Decide now what to do after all the data has been written in Flash
         */
        switch(OTA_APP_Context.file_type)
        {
          case Fw_App:
            {
              int ret;
              
              /**
               * Reboot on FW Application
               */
              ret = secure_data_udate_install();
              APP_DBG("OTA_CONF_EVT: secure_data_udate_install: %u\n", ret);
              
              APP_DBG("OTA_CONF_EVT: Reboot on new application\n");
              NVIC_SystemReset(); /* it waits until reset */
            }
            break;
            
          default:
            break;
        }
      }
      break;

    case OTA_READY_EVT:
      APP_DBG("OTA_READY_EVT\r\n");
      break;
    /* USER CODE END Service3_Notification_Service3_EvtOpcode */

    case OTA_BASE_ADR_WRITE_NO_RESP_EVT:
      /* USER CODE BEGIN Service3Char1_WRITE_NO_RESP_EVT */
      {
        APP_DBG("((OTA_Base_Addr_Event_Format_t*)(p_Notification->DataTransfered.p_Payload))->Command: %x\r\n", ((OTA_Base_Addr_Event_Format_t*)(p_Notification->DataTransfered.p_Payload))->Command);
        switch( ((OTA_Base_Addr_Event_Format_t*)(p_Notification->DataTransfered.p_Payload))->Command )
        {
          case OTA_STOP_ALL_UPLOAD:
            break;

          case OTA_APPLICATION_UPLOAD:
            {
              uint32_t first_valid_address;
              int error = 0;
              
              OTA_APP_Context.file_type = Fw_App;
              ((uint8_t*)&OTA_APP_Context.base_address)[0] = (((uint8_t*)((OTA_Base_Addr_Event_Format_t*)(p_Notification->DataTransfered.p_Payload))->Base_Addr))[2];
              ((uint8_t*)&OTA_APP_Context.base_address)[1] = (((uint8_t*)((OTA_Base_Addr_Event_Format_t*)(p_Notification->DataTransfered.p_Payload))->Base_Addr))[1];
              ((uint8_t*)&OTA_APP_Context.base_address)[2] = (((uint8_t*)((OTA_Base_Addr_Event_Format_t*)(p_Notification->DataTransfered.p_Payload))->Base_Addr))[0];
              OTA_APP_Context.base_address |= FLASH_BASE;
              OTA_APP_Context.sectors = 0;
              if(p_Notification->DataTransfered.Length > 4)
              {
                OTA_APP_Context.sectors = ((OTA_Base_Addr_Event_Format_t*)(p_Notification->DataTransfered.p_Payload))->Sectors;
              }
              else
              {
                OTA_APP_Context.sectors = CFG_DOWNLOAD_ACTIVE_NB_SECTORS;
              }
              /*S_DATA_IMAGE_SECONDARY_PARTITION_OFFSET + FLASH_S_DATA_PARTITION_SIZE*/
              OTA_APP_Context.write_value_index = 0;
              OTA_APP_Context.Conf_Indication_Status = OTA_APP_Ready_Pending;
              first_valid_address = S_DATA_IMAGE_SECONDARY_PARTITION_OFFSET + FLASH_BASE/*((CFG_ACTIVE_SLOT_START_SECTOR_INDEX + CFG_APP_SLOT_PAGE_SIZE) * FLASH_PAGE_SIZE) + FLASH_BASE*/;
              if(((OTA_APP_Context.base_address & 0xF) == 0) &&
                 (OTA_APP_Context.sectors <= CFG_DOWNLOAD_ACTIVE_NB_SECTORS) &&
                 (OTA_APP_Context.base_address >= first_valid_address) &&
                 ((OTA_APP_Context.base_address + ((OTA_APP_Context.sectors) * FLASH_PAGE_SIZE)) <= (first_valid_address + (FLASH_S_DATA_PARTITION_SIZE))))
              { /* Download address is 128 bits aligned */
                /* Size of file to download fit in download slot */
                /* Download address is in the download area */
                /* End download address fit in the download area */
                /* Erase the sectors */
                /* Clear events before start testing */
                  error = secure_data_update_init();

                  /* Check write op. */
                  if (error == 0)
                  {
                    APP_DBG("OTA_APPLICATION_UPLOAD: Erase OK\n");
                  }
                  else
                  {
                    APP_DBG("OTA_APPLICATION_UPLOAD: Erase ERROR\n");
                  }
                
                msg_conf.Length = 1;
                a_OTA_UpdateCharData[0] = OTA_READY_TO_RECEIVE_FILE;
                msg_conf.p_Payload = a_OTA_UpdateCharData;
                OTA_UpdateValue(OTA_CONF, &msg_conf);
              }
              else
              {
                msg_conf.Length = 1;
                APP_DBG("OTA_APPLICATION_UPLOAD: Not ready to receive file, oversized %x %x\n", OTA_APP_Context.base_address, OTA_APP_Context.sectors);
                APP_DBG("OTA_APPLICATION_UPLOAD: First 128 bits aligned address to download should be: 0x%x\n", first_valid_address);
                a_OTA_UpdateCharData[0] = OTA_NOT_READY_TO_RECEIVE_FILE;
                msg_conf.p_Payload = a_OTA_UpdateCharData;
                OTA_UpdateValue(OTA_CONF, &msg_conf);
              }

            }
            break;

          case OTA_UPLOAD_FINISHED:
            {
              if(OTA_APP_Context.file_type == Fw_App)
              { /* Reboot only after new application download */
                OTA_APP_Context.Conf_Indication_Status = OTA_APP_Pending;
                msg_conf.Length = 1;
                a_OTA_UpdateCharData[0] = OTA_REBOOT_CONFIRMED;
                msg_conf.p_Payload = a_OTA_UpdateCharData;
                OTA_UpdateValue(OTA_CONF, &msg_conf);
              }
            }
            break;

          case OTA_CANCEL_UPLOAD:
            {
              APP_DBG("OTA_CANCEL_UPLOAD\n");
            }
            break;

          default:
            break;
        }
      }
      /* USER CODE END Service3Char1_WRITE_NO_RESP_EVT */
      break;

    case OTA_CONF_INDICATE_ENABLED_EVT:
      /* USER CODE BEGIN Service3Char2_INDICATE_ENABLED_EVT */
      APP_DBG("OTA_CONF_INDICATE_ENABLED_EVT\n");
      /* USER CODE END Service3Char2_INDICATE_ENABLED_EVT */
      break;

    case OTA_CONF_INDICATE_DISABLED_EVT:
      /* USER CODE BEGIN Service3Char2_INDICATE_DISABLED_EVT */
      APP_DBG("OTA_CONF_INDICATE_DISABLED_EVT\n");
      /* USER CODE END Service3Char2_INDICATE_DISABLED_EVT */
      break;

    case OTA_RAW_DATA_WRITE_NO_RESP_EVT:
      /* USER CODE BEGIN Service3Char3_WRITE_NO_RESP_EVT */
      {
        int ret;
        APP_DBG("OTA_RAW_DATA_WRITE_NO_RESP_EVT\r\n");
        
        /**
         * Write in Flash the data received in the BLE packet
         */
        size_left = p_Notification->DataTransfered.Length;
        
        /**
         * For the flash manager the address of the data to be stored in FLASH shall be 32bits aligned
         * and the address where the data shall be written shall be 128bits aligned
         */
        memcpy( (uint8_t*)&OTA_APP_Context.write_value,
                ((OTA_Raw_Data_Event_Format_t*)(p_Notification->DataTransfered.p_Payload))->Raw_Data,
                size_left );
        
        for(int i=0; i<size_left; i++){
          APP_DBG("%x ", ((uint8_t*)OTA_APP_Context.write_value)[i]);
        }
        APP_DBG("\r\n");

        
        APP_DBG("offset: %x - size: %u\r\n", address_offset, size_left);
        ret = secure_data_update_write((uint8_t*)(&OTA_APP_Context.write_value[0]), address_offset, size_left);
        APP_DBG("secure_data_update_write %u\r\n", ret);
        
        /* Update write offset address for the next FLASH write */
        address_offset += size_left;

      }
      /* USER CODE END Service3Char3_WRITE_NO_RESP_EVT */
      break;

    default:
      /* USER CODE BEGIN Service3_Notification_default */

      /* USER CODE END Service3_Notification_default */
      break;
  }
  /* USER CODE BEGIN Service3_Notification_2 */

  /* USER CODE END Service3_Notification_2 */
  return;
}

void OTA_APP_EvtRx(OTA_APP_ConnHandleNotEvt_t *p_Notification)
{
  /* USER CODE BEGIN Service3_APP_EvtRx_1 */

  /* USER CODE END Service3_APP_EvtRx_1 */

  switch(p_Notification->EvtOpcode)
  {
    /* USER CODE BEGIN Service3_APP_EvtRx_Service3_EvtOpcode */

    /* USER CODE END Service3_Notification_Service3_EvtOpcode */
    case OTA_CONN_HANDLE_EVT :
      /* USER CODE BEGIN Service3_APP_CONN_HANDLE_EVT */

      /* USER CODE END Service3_APP_CONN_HANDLE_EVT */
      break;

    case OTA_DISCON_HANDLE_EVT :
      /* USER CODE BEGIN Service3_APP_DISCON_HANDLE_EVT */

      /* USER CODE END Service3_APP_DISCON_HANDLE_EVT */
      break;

    default:
      /* USER CODE BEGIN Service3_APP_EvtRx_default */

      /* USER CODE END Service3_APP_EvtRx_default */
      break;
  }

  /* USER CODE BEGIN Service3_APP_EvtRx_2 */

  /* USER CODE END Service3_APP_EvtRx_2 */

  return;
}

void OTA_APP_Init(void)
{
  OTA_Init();

  size_left = 0;
  address_offset = 0;
  /* USER CODE END Service3_APP_Init */
  return;
}

/* USER CODE BEGIN FD */
/**
 * Get Confiramation status
 */
uint8_t OTA_APP_GetConfStatus(void)
{
  return(OTA_APP_Context.Conf_Indication_Status);
}

/* USER CODE END FD */

/*************************************************************
 *
 * LOCAL FUNCTIONS
 *
 *************************************************************/
__USED void OTA_Conf_SendIndication(void) /* Property Indication */
{
  OTA_APP_SendInformation_t indication_on_off = Conf_INDICATION_OFF;
  OTA_Data_t ota_indication_data;

  ota_indication_data.p_Payload = (uint8_t*)a_OTA_UpdateCharData;
  ota_indication_data.Length = 0;

  /* USER CODE BEGIN Service3Char2_IS_1*/

  /* USER CODE END Service3Char2_IS_1*/

  if (indication_on_off != Conf_INDICATION_OFF)
  {
    OTA_UpdateValue(OTA_CONF, &ota_indication_data);
  }

  /* USER CODE BEGIN Service3Char2_IS_Last*/

  /* USER CODE END Service3Char2_IS_Last*/

  return;
}

/* USER CODE BEGIN FD_LOCAL_FUNCTIONS*/

/* USER CODE END FD_LOCAL_FUNCTIONS*/
