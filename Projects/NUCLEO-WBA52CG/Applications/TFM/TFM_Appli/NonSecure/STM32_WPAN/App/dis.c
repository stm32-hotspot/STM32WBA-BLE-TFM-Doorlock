/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    service2.c
  * @author  MCD Application Team
  * @brief   service2 definition.
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
#include "common_blesvc.h"
#include "dis.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/

/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

typedef struct{
  uint16_t  DisSvcHdle;                  /**< Dis Service Handle */
  uint16_t  MansCharHdle;                  /**< MANS Characteristic Handle */
  uint16_t  MnbsCharHdle;                  /**< MNBS Characteristic Handle */
  uint16_t  SnsCharHdle;                  /**< SNS Characteristic Handle */
  uint16_t  HrsCharHdle;                  /**< HRS Characteristic Handle */
  uint16_t  FrsCharHdle;                  /**< FRS Characteristic Handle */
}DIS_Context_t;

/* Private defines -----------------------------------------------------------*/
#define UUID_128_SUPPORTED  1

#if (UUID_128_SUPPORTED == 1)
#define BM_UUID_LENGTH  UUID_TYPE_128
#else
#define BM_UUID_LENGTH  UUID_TYPE_16
#endif

#define BM_REQ_CHAR_SIZE    (3)

/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* External variables --------------------------------------------------------*/
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/* Private macros ------------------------------------------------------------*/
#define CHARACTERISTIC_DESCRIPTOR_ATTRIBUTE_OFFSET         2
#define CHARACTERISTIC_VALUE_ATTRIBUTE_OFFSET              1
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
static const uint8_t SizeMans = 32;
static const uint8_t SizeMnbs = 32;
static const uint8_t SizeSns = 32;
static const uint8_t SizeHrs = 32;
static const uint8_t SizeFrs = 32;

/**
 * START of Section BLE_DRIVER_CONTEXT
 */
PLACE_IN_SECTION("BLE_DRIVER_CONTEXT") static DIS_Context_t DIS_Context;

/**
 * END of Section BLE_DRIVER_CONTEXT
 */

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static SVCCTL_EvtAckStatus_t DIS_EventHandler(void *p_pckt);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
/* USER CODE BEGIN PFD */

/* USER CODE END PFD */

/* Private functions ----------------------------------------------------------*/

#define COPY_UUID_128(uuid_struct, uuid_15, uuid_14, uuid_13, uuid_12, uuid_11, uuid_10, uuid_9, uuid_8, uuid_7, uuid_6, uuid_5, uuid_4, uuid_3, uuid_2, uuid_1, uuid_0) \
do {\
    uuid_struct[0] = uuid_0; uuid_struct[1] = uuid_1; uuid_struct[2] = uuid_2; uuid_struct[3] = uuid_3; \
    uuid_struct[4] = uuid_4; uuid_struct[5] = uuid_5; uuid_struct[6] = uuid_6; uuid_struct[7] = uuid_7; \
    uuid_struct[8] = uuid_8; uuid_struct[9] = uuid_9; uuid_struct[10] = uuid_10; uuid_struct[11] = uuid_11; \
    uuid_struct[12] = uuid_12; uuid_struct[13] = uuid_13; uuid_struct[14] = uuid_14; uuid_struct[15] = uuid_15; \
}while(0)

/* Hardware Characteristics Service */
/*
 The following 128bits UUIDs have been generated from the random UUID
 generator:
 D973F2E0-B19E-11E2-9E96-0800200C9A66: Service 128bits UUID
 D973F2E1-B19E-11E2-9E96-0800200C9A66: Characteristic_1 128bits UUID
 D973F2E2-B19E-11E2-9E96-0800200C9A66: Characteristic_2 128bits UUID
 */

/* USER CODE BEGIN PF */

/* USER CODE END PF */

/**
 * @brief  Event handler
 * @param  p_Event: Address of the buffer holding the p_Event
 * @retval Ack: Return whether the p_Event has been managed or not
 */
static SVCCTL_EvtAckStatus_t DIS_EventHandler(void *p_Event)
{
  SVCCTL_EvtAckStatus_t return_value;
  hci_event_pckt *p_event_pckt;
  evt_blecore_aci *p_blecore_evt;
  /* USER CODE BEGIN Service2_EventHandler_1 */

  /* USER CODE END Service2_EventHandler_1 */

  return_value = SVCCTL_EvtNotAck;
  p_event_pckt = (hci_event_pckt *)(((hci_uart_pckt*)p_Event)->data);

  switch(p_event_pckt->evt)
  {
    case HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE:
      p_blecore_evt = (evt_blecore_aci*)p_event_pckt->data;
      switch(p_blecore_evt->ecode)
      {
        case ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE:
          /* USER CODE BEGIN EVT_BLUE_GATT_ATTRIBUTE_MODIFIED_BEGIN */

          /* USER CODE END EVT_BLUE_GATT_ATTRIBUTE_MODIFIED_BEGIN */

          /* USER CODE BEGIN EVT_BLUE_GATT_ATTRIBUTE_MODIFIED_END */

          /* USER CODE END EVT_BLUE_GATT_ATTRIBUTE_MODIFIED_END */
          break;

        case ACI_GATT_READ_PERMIT_REQ_VSEVT_CODE :
          /* USER CODE BEGIN EVT_BLUE_GATT_READ_PERMIT_REQ_BEGIN */

          /* USER CODE END EVT_BLUE_GATT_READ_PERMIT_REQ_BEGIN */

          /* USER CODE BEGIN EVT_BLUE_GATT_READ_PERMIT_REQ_END */

          /* USER CODE END EVT_BLUE_GATT_READ_PERMIT_REQ_END */
          break;

        case ACI_GATT_WRITE_PERMIT_REQ_VSEVT_CODE:
          /* USER CODE BEGIN EVT_BLUE_GATT_WRITE_PERMIT_REQ_BEGIN */

          /* USER CODE END EVT_BLUE_GATT_WRITE_PERMIT_REQ_BEGIN */

          /* USER CODE BEGIN EVT_BLUE_GATT_WRITE_PERMIT_REQ_END */

          /* USER CODE END EVT_BLUE_GATT_WRITE_PERMIT_REQ_END */
          break;
        /* USER CODE BEGIN BLECORE_EVT */
        /* Manage ACI_GATT_INDICATION_VSEVT_CODE */
        case ACI_GATT_INDICATION_VSEVT_CODE:
          {
            tBleStatus status = BLE_STATUS_FAILED;
            aci_gatt_indication_event_rp0 *pr = (void*)p_blecore_evt->data;
            status = aci_gatt_confirm_indication(pr->Connection_Handle);
            if (status != BLE_STATUS_SUCCESS)
            {
              APP_DBG_MSG("  Fail   : aci_gatt_confirm_indication command, result: 0x%x \n", status);
            }
            else
            {
              APP_DBG_MSG("  Success: aci_gatt_confirm_indication command\n");
            }   
          }
          break; /* end ACI_GATT_NOTIFICATION_VSEVT_CODE */
        /* USER CODE END BLECORE_EVT */
        default:
          /* USER CODE BEGIN EVT_DEFAULT */

          /* USER CODE END EVT_DEFAULT */
          break;
      }
      /* USER CODE BEGIN EVT_VENDOR*/

      /* USER CODE END EVT_VENDOR*/
      break; /* HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE */

      /* USER CODE BEGIN EVENT_PCKT_CASES*/

      /* USER CODE END EVENT_PCKT_CASES*/

    default:
      /* USER CODE BEGIN EVENT_PCKT*/

      /* USER CODE END EVENT_PCKT*/
      break;
  }

  /* USER CODE BEGIN Service2_EventHandler_2 */

  /* USER CODE END Service2_EventHandler_2 */

  return(return_value);
}/* end DIS_EventHandler */

/* Public functions ----------------------------------------------------------*/

/**
 * @brief  Service initialization
 * @param  None
 * @retval None
 */
void DIS_Init(void)
{
  Char_UUID_t  uuid;
  tBleStatus ret = BLE_STATUS_INVALID_PARAMS;
  /* USER CODE BEGIN SVCCTL_InitService2Svc_1 */

  /* USER CODE END SVCCTL_InitService2Svc_1 */

  /**
   *  Register the event handler to the BLE controller
   */
  SVCCTL_RegisterSvcHandler(DIS_EventHandler);

  /**
   * DIS
   *
   * Max_Attribute_Records = 1 + 2*5 + 1*no_of_char_with_notify_or_indicate_property + 1*no_of_char_with_broadcast_property
   * service_max_attribute_record = 1 for DIS +
   *                                2 for MANS +
   *                                2 for MNBS +
   *                                2 for SNS +
   *                                2 for HRS +
   *                                2 for FRS +
   *                              = 11
   */
  uuid.Char_UUID_16 = 0x180a;
  ret = aci_gatt_add_service(UUID_TYPE_16,
                             (Service_UUID_t *) &uuid,
                             PRIMARY_SERVICE,
                             11,
                             &(DIS_Context.DisSvcHdle));
  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : aci_gatt_add_service command: DIS, error code: 0x%x \n\r", ret);
  }
  else
  {
    APP_DBG_MSG("  Success: aci_gatt_add_service command: DIS \n\r");
  }

  /**
   * MANS
   */
  uuid.Char_UUID_16 = 0x2a29;
  ret = aci_gatt_add_char(DIS_Context.DisSvcHdle,
                          UUID_TYPE_16,
                          (Char_UUID_t *) &uuid,
                          SizeMans,
                          CHAR_PROP_READ,
                          ATTR_PERMISSION_NONE,
                          GATT_DONT_NOTIFY_EVENTS,
                          0x10,
                          CHAR_VALUE_LEN_VARIABLE,
                          &(DIS_Context.MansCharHdle));
  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : aci_gatt_add_char command   : MANS, error code: 0x%2X\n", ret);
  }
  else
  {
    APP_DBG_MSG("  Success: aci_gatt_add_char command   : MANS\n");
  }

  /**
   * MNBS
   */
  uuid.Char_UUID_16 = 0x2a24;
  ret = aci_gatt_add_char(DIS_Context.DisSvcHdle,
                          UUID_TYPE_16,
                          (Char_UUID_t *) &uuid,
                          SizeMnbs,
                          CHAR_PROP_READ,
                          ATTR_PERMISSION_NONE,
                          GATT_DONT_NOTIFY_EVENTS,
                          0x10,
                          CHAR_VALUE_LEN_VARIABLE,
                          &(DIS_Context.MnbsCharHdle));
  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : aci_gatt_add_char command   : MNBS, error code: 0x%2X\n", ret);
  }
  else
  {
    APP_DBG_MSG("  Success: aci_gatt_add_char command   : MNBS\n");
  }

  /**
   * SNS
   */
  uuid.Char_UUID_16 = 0x2a25;
  ret = aci_gatt_add_char(DIS_Context.DisSvcHdle,
                          UUID_TYPE_16,
                          (Char_UUID_t *) &uuid,
                          SizeSns,
                          CHAR_PROP_READ,
                          ATTR_PERMISSION_NONE,
                          GATT_DONT_NOTIFY_EVENTS,
                          0x10,
                          CHAR_VALUE_LEN_VARIABLE,
                          &(DIS_Context.SnsCharHdle));
  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : aci_gatt_add_char command   : SNS, error code: 0x%2X\n", ret);
  }
  else
  {
    APP_DBG_MSG("  Success: aci_gatt_add_char command   : SNS\n");
  }

  /**
   * HRS
   */
  uuid.Char_UUID_16 = 0x2a27;
  ret = aci_gatt_add_char(DIS_Context.DisSvcHdle,
                          UUID_TYPE_16,
                          (Char_UUID_t *) &uuid,
                          SizeHrs,
                          CHAR_PROP_READ,
                          ATTR_PERMISSION_NONE,
                          GATT_DONT_NOTIFY_EVENTS,
                          0x10,
                          CHAR_VALUE_LEN_VARIABLE,
                          &(DIS_Context.HrsCharHdle));
  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : aci_gatt_add_char command   : HRS, error code: 0x%2X\n", ret);
  }
  else
  {
    APP_DBG_MSG("  Success: aci_gatt_add_char command   : HRS\n");
  }

  /**
   * FRS
   */
  uuid.Char_UUID_16 = 0x2a26;
  ret = aci_gatt_add_char(DIS_Context.DisSvcHdle,
                          UUID_TYPE_16,
                          (Char_UUID_t *) &uuid,
                          SizeFrs,
                          CHAR_PROP_READ,
                          ATTR_PERMISSION_NONE,
                          GATT_DONT_NOTIFY_EVENTS,
                          0x10,
                          CHAR_VALUE_LEN_VARIABLE,
                          &(DIS_Context.FrsCharHdle));
  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : aci_gatt_add_char command   : FRS, error code: 0x%2X\n", ret);
  }
  else
  {
    APP_DBG_MSG("  Success: aci_gatt_add_char command   : FRS\n");
  }

  /* USER CODE BEGIN SVCCTL_InitService2Svc_2 */

  /* USER CODE END SVCCTL_InitService2Svc_2 */

  return;
}

/**
 * @brief  Characteristic update
 * @param  CharOpcode: Characteristic identifier
 * @param  Service_Instance: Instance of the service to which the characteristic belongs
 *
 */
tBleStatus DIS_UpdateValue(DIS_CharOpcode_t CharOpcode, DIS_Data_t *pData)
{
  tBleStatus ret = BLE_STATUS_INVALID_PARAMS;
  /* USER CODE BEGIN Service2_App_Update_Char_1 */

  /* USER CODE END Service2_App_Update_Char_1 */

  switch(CharOpcode)
  {
    case DIS_MANS:
      ret = aci_gatt_update_char_value(DIS_Context.DisSvcHdle,
                                       DIS_Context.MansCharHdle,
                                       0, /* charValOffset */
                                       pData->Length, /* charValueLen */
                                       (uint8_t *)pData->p_Payload);
      if (ret != BLE_STATUS_SUCCESS)
      {
        APP_DBG_MSG("  Fail   : aci_gatt_update_char_value MANS command, error code: 0x%2X\n", ret);
      }
      else
      {
        APP_DBG_MSG("  Success: aci_gatt_update_char_value MANS command\n");
      }
      /* USER CODE BEGIN Service2_Char_Value_1*/

      /* USER CODE END Service2_Char_Value_1*/
      break;

    case DIS_MNBS:
      ret = aci_gatt_update_char_value(DIS_Context.DisSvcHdle,
                                       DIS_Context.MnbsCharHdle,
                                       0, /* charValOffset */
                                       pData->Length, /* charValueLen */
                                       (uint8_t *)pData->p_Payload);
      if (ret != BLE_STATUS_SUCCESS)
      {
        APP_DBG_MSG("  Fail   : aci_gatt_update_char_value MNBS command, error code: 0x%2X\n", ret);
      }
      else
      {
        APP_DBG_MSG("  Success: aci_gatt_update_char_value MNBS command\n");
      }
      /* USER CODE BEGIN Service2_Char_Value_2*/

      /* USER CODE END Service2_Char_Value_2*/
      break;

    case DIS_SNS:
      ret = aci_gatt_update_char_value(DIS_Context.DisSvcHdle,
                                       DIS_Context.SnsCharHdle,
                                       0, /* charValOffset */
                                       pData->Length, /* charValueLen */
                                       (uint8_t *)pData->p_Payload);
      if (ret != BLE_STATUS_SUCCESS)
      {
        APP_DBG_MSG("  Fail   : aci_gatt_update_char_value SNS command, error code: 0x%2X\n", ret);
      }
      else
      {
        APP_DBG_MSG("  Success: aci_gatt_update_char_value SNS command\n");
      }
      /* USER CODE BEGIN Service2_Char_Value_3*/

      /* USER CODE END Service2_Char_Value_3*/
      break;

    case DIS_HRS:
      ret = aci_gatt_update_char_value(DIS_Context.DisSvcHdle,
                                       DIS_Context.HrsCharHdle,
                                       0, /* charValOffset */
                                       pData->Length, /* charValueLen */
                                       (uint8_t *)pData->p_Payload);
      if (ret != BLE_STATUS_SUCCESS)
      {
        APP_DBG_MSG("  Fail   : aci_gatt_update_char_value HRS command, error code: 0x%2X\n", ret);
      }
      else
      {
        APP_DBG_MSG("  Success: aci_gatt_update_char_value HRS command\n");
      }
      /* USER CODE BEGIN Service2_Char_Value_4*/

      /* USER CODE END Service2_Char_Value_4*/
      break;

    case DIS_FRS:
      ret = aci_gatt_update_char_value(DIS_Context.DisSvcHdle,
                                       DIS_Context.FrsCharHdle,
                                       0, /* charValOffset */
                                       pData->Length, /* charValueLen */
                                       (uint8_t *)pData->p_Payload);
      if (ret != BLE_STATUS_SUCCESS)
      {
        APP_DBG_MSG("  Fail   : aci_gatt_update_char_value FRS command, error code: 0x%2X\n", ret);
      }
      else
      {
        APP_DBG_MSG("  Success: aci_gatt_update_char_value FRS command\n");
      }
      /* USER CODE BEGIN Service2_Char_Value_5*/

      /* USER CODE END Service2_Char_Value_5*/
      break;

    default:
      break;
  }

  /* USER CODE BEGIN Service2_App_Update_Char_2 */

  /* USER CODE END Service2_App_Update_Char_2 */

  return ret;
}