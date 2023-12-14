/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm_list.c
  * @author  MCD Application Team
  * @brief   TCircular Linked List Implementation.
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

/******************************************************************************
 * Include Files
 ******************************************************************************/
#include "utilities_common.h"

#include "stm_list.h"
#include "tfm_radio_service_interface.h"

/******************************************************************************
 * Function Definitions
 ******************************************************************************/
void LST_init_head (tListNode * listHead)
{
  listHead->next = listHead;
  listHead->prev = listHead;
}

uint8_t LST_is_empty (tListNode * listHead)
{
  uint32_t primask_bit;
  uint8_t return_value;

  /**< backup PRIMASK bit */ /**< Disable all interrupts by setting PRIMASK bit on Cortex*/
  primask_bit = INTERFACE_TFM_RADIOSERVERICE_DisableIrqWithdirectRestore();

  if(listHead->next == listHead)
  {
    return_value = TRUE;
  }
  else
  {
    return_value = FALSE;
  }
  /**< Restore PRIMASK bit*/
  INTERFACE_TFM_RADIOSERVERICE_SetPrimask(primask_bit);

  return return_value;
}

void LST_insert_head (tListNode * listHead, tListNode * node)
{
  uint32_t primask_bit;

  /**< backup PRIMASK bit */ /**< Disable all interrupts by setting PRIMASK bit on Cortex*/
  primask_bit = INTERFACE_TFM_RADIOSERVERICE_DisableIrqWithdirectRestore();

  node->next = listHead->next;
  node->prev = listHead;
  listHead->next = node;
  (node->next)->prev = node;

  /**< Restore PRIMASK bit*/
  INTERFACE_TFM_RADIOSERVERICE_SetPrimask(primask_bit);
}

void LST_insert_tail (tListNode * listHead, tListNode * node)
{
  uint32_t primask_bit;

  /**< backup PRIMASK bit */ /**< Disable all interrupts by setting PRIMASK bit on Cortex*/
  primask_bit = INTERFACE_TFM_RADIOSERVERICE_DisableIrqWithdirectRestore();

  node->next = listHead;
  node->prev = listHead->prev;
  listHead->prev = node;
  (node->prev)->next = node;

  /**< Restore PRIMASK bit*/
  INTERFACE_TFM_RADIOSERVERICE_SetPrimask(primask_bit);
}

void LST_remove_node (tListNode * node)
{
  uint32_t primask_bit;

  /**< backup PRIMASK bit */ /**< Disable all interrupts by setting PRIMASK bit on Cortex*/
  primask_bit = INTERFACE_TFM_RADIOSERVERICE_DisableIrqWithdirectRestore();

  (node->prev)->next = node->next;
  (node->next)->prev = node->prev;

  /**< Restore PRIMASK bit*/
  INTERFACE_TFM_RADIOSERVERICE_SetPrimask(primask_bit);
}

void LST_remove_head (tListNode * listHead, tListNode ** node )
{
  uint32_t primask_bit;

  /**< backup PRIMASK bit */ /**< Disable all interrupts by setting PRIMASK bit on Cortex*/
  primask_bit = INTERFACE_TFM_RADIOSERVERICE_DisableIrqWithdirectRestore();

  *node = listHead->next;
  LST_remove_node (listHead->next);

  /**< Restore PRIMASK bit*/
  INTERFACE_TFM_RADIOSERVERICE_SetPrimask(primask_bit);
}

void LST_remove_tail (tListNode * listHead, tListNode ** node )
{
  uint32_t primask_bit;

  /**< backup PRIMASK bit */ /**< Disable all interrupts by setting PRIMASK bit on Cortex*/
  primask_bit = INTERFACE_TFM_RADIOSERVERICE_DisableIrqWithdirectRestore();

  *node = listHead->prev;
  LST_remove_node (listHead->prev);

  /**< Restore PRIMASK bit*/
  INTERFACE_TFM_RADIOSERVERICE_SetPrimask(primask_bit);
}

void LST_insert_node_after (tListNode * node, tListNode * ref_node)
{
  uint32_t primask_bit;

  /**< backup PRIMASK bit */ /**< Disable all interrupts by setting PRIMASK bit on Cortex*/
  primask_bit = INTERFACE_TFM_RADIOSERVERICE_DisableIrqWithdirectRestore();

  node->next = ref_node->next;
  node->prev = ref_node;
  ref_node->next = node;
  (node->next)->prev = node;

  /**< Restore PRIMASK bit*/
  INTERFACE_TFM_RADIOSERVERICE_SetPrimask(primask_bit);
}

void LST_insert_node_before (tListNode * node, tListNode * ref_node)
{
  uint32_t primask_bit;

  /**< backup PRIMASK bit */ /**< Disable all interrupts by setting PRIMASK bit on Cortex*/
  primask_bit = INTERFACE_TFM_RADIOSERVERICE_DisableIrqWithdirectRestore();

  node->next = ref_node;
  node->prev = ref_node->prev;
  ref_node->prev = node;
  (node->prev)->next = node;

  /**< Restore PRIMASK bit*/
  INTERFACE_TFM_RADIOSERVERICE_SetPrimask(primask_bit);
}

int LST_get_size (tListNode * listHead)
{
  int size = 0;
  tListNode * temp;
  uint32_t primask_bit;

  /**< backup PRIMASK bit */ /**< Disable all interrupts by setting PRIMASK bit on Cortex*/
  primask_bit = INTERFACE_TFM_RADIOSERVERICE_DisableIrqWithdirectRestore();

  temp = listHead->next;
  while (temp != listHead)
  {
    size++;
    temp = temp->next;
  }

  /**< Restore PRIMASK bit*/
  INTERFACE_TFM_RADIOSERVERICE_SetPrimask(primask_bit);

  return (size);
}

void LST_get_next_node (tListNode * ref_node, tListNode ** node)
{
  uint32_t primask_bit;

  /**< backup PRIMASK bit */ /**< Disable all interrupts by setting PRIMASK bit on Cortex*/
  primask_bit = INTERFACE_TFM_RADIOSERVERICE_DisableIrqWithdirectRestore();

  *node = ref_node->next;

  /**< Restore PRIMASK bit*/
  INTERFACE_TFM_RADIOSERVERICE_SetPrimask(primask_bit);
}

void LST_get_prev_node (tListNode * ref_node, tListNode ** node)
{
  uint32_t primask_bit;

  /**< backup PRIMASK bit */ /**< Disable all interrupts by setting PRIMASK bit on Cortex*/
  primask_bit = INTERFACE_TFM_RADIOSERVERICE_DisableIrqWithdirectRestore();

  *node = ref_node->prev;

  /**< Restore PRIMASK bit*/
  INTERFACE_TFM_RADIOSERVERICE_SetPrimask(primask_bit);
}

