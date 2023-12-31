/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    linklayer_plat.c
  * @author  MCD Application Team
  * @brief   Source file for the linklayer plateform adaptation layer
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

#include "app_common.h"
#include "stm32wbaxx_hal.h"
#include "linklayer_plat.h"
#include "stm32wbaxx_hal_conf.h"
#include "stm32wbaxx_ll_rcc.h"
#include "app_conf.h"
#include "scm.h"
#include "adc_ctrl.h"
#include "bleplat.h"
#include "tfm_radio_service_interface.h"

#define max(a,b) ((a) > (b) ? a : b)

/* 2.4GHz RADIO ISR callbacks */
void (*radio_callback)(void) = NULL;
void (*low_isr_callback)(void) = NULL;

/* RNG handle */
extern RNG_HandleTypeDef hrng;

/* Link Layer temperature request from background */
extern void ll_sys_bg_temperature_measurement(void);

/* Radio critical sections */
volatile int32_t prio_high_isr_counter = 0;
volatile int32_t prio_low_isr_counter = 0;
volatile int32_t prio_sys_isr_counter = 0;
volatile int32_t irq_counter = 0;
volatile uint32_t local_basepri_value = 0;

/* Radio SW low ISR global variable */
volatile uint8_t radio_sw_low_isr_is_running_high_prio = 0;

/**
  * @brief  Configure the necessary clock sources for the radio.
  * @param  None
  * @retval None
  */
void LINKLAYER_PLAT_ClockInit()
{
  /* Select LSE as Sleep CLK */
  __HAL_RCC_RADIOSLPTIM_CONFIG(RCC_RADIOSTCLKSOURCE_LSE);

  /* Enable AHB5ENR peripheral clock (bus CLK) */
  __HAL_RCC_RADIO_CLK_ENABLE();
}

/**
  * @brief  Link Layer active waiting loop.
  * @param  delay: delay in us
  * @retval None
  */
void LINKLAYER_PLAT_DelayUs(uint32_t delay)
{
__IO register uint32_t Delay = delay * (SystemCoreClock / 1000000U);
	do
	{
		__NOP();
	}
	while (Delay --);
}

/**
  * @brief  Link Layer assertion API
  * @param  condition: conditional statement to be checked.
  * @retval None
  */
void LINKLAYER_PLAT_Assert(uint8_t condition)
{
  assert_param(condition);
}

/**
  * @brief  Enable/disable the Link Layer active clock (baseband clock).
  * @param  enable: boolean value to enable (1) or disable (0) the clock.
  * @retval None
  */
void LINKLAYER_PLAT_WaitHclkRdy(void)
{
  /* Wait on radio bus clock readiness */
  while(HAL_RCCEx_GetRadioBusClockReadiness() != RCC_RADIO_BUS_CLOCK_READY);
}

/**
  * @brief  Active wait on bus clock readiness.
  * @param  None
  * @retval None
  */
void LINKLAYER_PLAT_AclkCtrl(uint8_t enable)
{
  if(enable){
    /* Enable RADIO baseband clock (active CLK) */
    HAL_RCCEx_EnableRadioBBClock();

    /* Polling on HSE32 activation */
    while ( LL_RCC_HSE_IsReady() == 0);
  }
  else
  {
    /* Disable RADIO baseband clock (active CLK) */
    HAL_RCCEx_DisableRadioBBClock();
  }
}

/**
  * @brief  Link Layer RNG request.
  * @param  ptr_rnd: pointer to the variable that hosts the number.
  * @param  len: number of byte of anthropy to get.
  * @retval None
  */
void LINKLAYER_PLAT_GetRNG(uint8_t *ptr_rnd, uint32_t len)
{
  uint32_t nb_remaining_rng = len;
  uint32_t generated_rng;

  /* Get the requested RNGs (4 bytes by 4bytes) */
  while(nb_remaining_rng >= 4)
  {
    generated_rng = 0;
    BLEPLAT_RngGet(1, &generated_rng);
    memcpy((ptr_rnd+(len-nb_remaining_rng)), &generated_rng, 4);
    nb_remaining_rng -=4;
  }

  /* Get the remaining number of RNGs */
  if(nb_remaining_rng>0){
    generated_rng = 0;
    BLEPLAT_RngGet(1, &generated_rng);
    memcpy((ptr_rnd+(len-nb_remaining_rng)), &generated_rng, nb_remaining_rng);
  }
}

/**
  * @brief  Initialize Link Layer radio high priority interrupt.
  * @param  intr_cb: function pointer to assign for the radio high priority ISR routine.
  * @retval None
  */
void LINKLAYER_PLAT_SetupRadioIT(void (*intr_cb)())
{
  radio_callback = intr_cb;
  INTERFACE_TFM_RADIOSERVERICE_SetRadioPriority(RADIO_INTR_PRIO_HIGH);
  INTERFACE_TFM_RADIOSERVERICE_EnableRadioIt();
}

/**
  * @brief  Initialize Link Layer SW low priority interrupt.
  * @param  intr_cb: function pointer to assign for the SW low priority ISR routine.
  * @retval None
  */
void LINKLAYER_PLAT_SetupSwLowIT(void (*intr_cb)())
{
  low_isr_callback = intr_cb;

  HAL_NVIC_SetPriority((IRQn_Type) RADIO_SW_LOW_INTR_NUM, RADIO_SW_LOW_INTR_PRIO, 0);
  HAL_NVIC_EnableIRQ((IRQn_Type) RADIO_SW_LOW_INTR_NUM);
}

/**
  * @brief  Trigger the link layer SW low interrupt.
  * @param  None
  * @retval None
  */
void LINKLAYER_PLAT_TriggerSwLowIT(uint8_t priority)
{
  uint8_t low_isr_priority = RADIO_INTR_PRIO_LOW;

  /* Check if a SW low interrupt as already been raised.
   * Nested call far radio low isr are not supported
   **/

  if(NVIC_GetActive(RADIO_SW_LOW_INTR_NUM) == 0)
  {
    /* No nested SW low ISR, default behavior */

    if(priority == 0)
    {
      low_isr_priority = RADIO_SW_LOW_INTR_PRIO;
    }

    HAL_NVIC_SetPriority((IRQn_Type) RADIO_SW_LOW_INTR_NUM, low_isr_priority, 0);
  }
  else
  {
    /* Nested call detected */
    /* No change for SW radio low interrupt priority for the moment */

    if(priority != 0)
    {
      /* At the end of current SW radio low ISR, this pending SW low interrupt
       * will run with RADIO_INTR_PRIO_LOW priority
       **/
      radio_sw_low_isr_is_running_high_prio = 1;
    }
  }

  HAL_NVIC_SetPendingIRQ((IRQn_Type) RADIO_SW_LOW_INTR_NUM);
}

/**
  * @brief  Enable interrupts.
  * @param  None
  * @retval None
  */
void LINKLAYER_PLAT_EnableIRQ(void)
{
    INTERFACE_TFM_RADIOSERVERICE_EnableIrq();
}

/**
  * @brief  Disable interrupts.
  * @param  None
  * @retval None
  */
void LINKLAYER_PLAT_DisableIRQ(void)
{
  INTERFACE_TFM_RADIOSERVERICE_DisableIrq();
}

/**
  * @brief  Enable specific interrupt group.
  * @param  isr_type: mask for interrupt group to enable.
  *         This parameter can be one of the following:
  *         @arg LL_HIGH_ISR_ONLY: enable link layer high priority ISR.
  *         @arg LL_LOW_ISR_ONLY: enable link layer SW low priority ISR.
  *         @arg SYS_LOW_ISR: mask interrupts for all the other system ISR with
  *              lower priority that link layer SW low interrupt.
  * @retval None
  */
void LINKLAYER_PLAT_EnableSpecificIRQ(uint8_t isr_type)
{
  if( (isr_type & LL_HIGH_ISR_ONLY) != 0 )
  {
    prio_high_isr_counter--;
    if(prio_high_isr_counter == 0)
    {
      INTERFACE_TFM_RADIOSERVERICE_EnableRadioIt();
    }
  }

  if( (isr_type & LL_LOW_ISR_ONLY) != 0 )
  {
    prio_low_isr_counter--;
    if(prio_low_isr_counter == 0)
    {
      /* When specific counter for link layer SW low ISR reaches 0, interrupt is enabled */
      HAL_NVIC_EnableIRQ(RADIO_SW_LOW_INTR_NUM);
    }

  }

  if( (isr_type & SYS_LOW_ISR) != 0 )
  {
    prio_sys_isr_counter--;
    if(prio_sys_isr_counter == 0)
    {
      INTERFACE_TFM_RADIOSERVERICE_SetBasepri(local_basepri_value);
      __set_BASEPRI(local_basepri_value);
    }
  }
}

/**
  * @brief  Disable specific interrupt group.
  * @param  isr_type: mask for interrupt group to disable.
  *         This parameter can be one of the following:
  *         @arg LL_HIGH_ISR_ONLY: disable link layer high priority ISR.
  *         @arg LL_LOW_ISR_ONLY: disable link layer SW low priority ISR.
  *         @arg SYS_LOW_ISR: unmask interrupts for all the other system ISR with
  *              lower priority that link layer SW low interrupt.
  * @retval None
  */
void LINKLAYER_PLAT_DisableSpecificIRQ(uint8_t isr_type)
{
  if( (isr_type & LL_HIGH_ISR_ONLY) != 0 )
  {
    prio_high_isr_counter++;
    if(prio_high_isr_counter == 1)
    {
      INTERFACE_TFM_RADIOSERVERICE_DisableRadioIt();
    }
  }

  if( (isr_type & LL_LOW_ISR_ONLY) != 0 )
  {
    prio_low_isr_counter++;
    if(prio_low_isr_counter == 1)
    {
      /* When specific counter for link layer SW low ISR value is 1, interrupt is disabled */
      HAL_NVIC_DisableIRQ(RADIO_SW_LOW_INTR_NUM);
    }
  }

  if( (isr_type & SYS_LOW_ISR) != 0 )
  {
    prio_sys_isr_counter++;
    if(prio_sys_isr_counter == 1)
    {
      /* Save basepri register value */
      local_basepri_value = __get_BASEPRI();
      INTERFACE_TFM_RADIOSERVERICE_SetBaseprimax(RADIO_INTR_PRIO_LOW<<4);
      __set_BASEPRI_MAX(RADIO_INTR_PRIO_LOW<<4);
    }
  }
}

/**
  * @brief  Enable link layer high priority ISR only.
  * @param  None
  * @retval None
  */
void LINKLAYER_PLAT_EnableRadioIT(void)
{
  INTERFACE_TFM_RADIOSERVERICE_EnableRadioIt();
}

/**
  * @brief  Disable link layer high priority ISR only.
  * @param  None
  * @retval None
  */
void LINKLAYER_PLAT_DisableRadioIT(void)
{
  INTERFACE_TFM_RADIOSERVERICE_DisableRadioIt();
}

/**
  * @brief  Link Layer notification for radio activity start.
  * @param  None
  * @retval None
  */
void LINKLAYER_PLAT_StartRadioEvt(void)
{
  __HAL_RCC_RADIO_CLK_SLEEP_ENABLE();
  INTERFACE_TFM_RADIOSERVERICE_SetRadioPriority(RADIO_INTR_PRIO_HIGH);
  scm_notifyradiostate(SCM_RADIO_ACTIVE);
}

/**
  * @brief  Link Layer notification for radio activity end.
  * @param  None
  * @retval None
  */
void LINKLAYER_PLAT_StopRadioEvt(void)
{
  __HAL_RCC_RADIO_CLK_SLEEP_DISABLE();
  INTERFACE_TFM_RADIOSERVERICE_SetRadioPriority(RADIO_INTR_PRIO_LOW);
  scm_notifyradiostate(SCM_RADIO_NOT_ACTIVE);
}

/**
  * @brief  Link Layer requests temperature.
  * @param  None
  * @retval None
  */
void LINKLAYER_PLAT_RequestTemperature(void)
{
#if (USE_TEMPERATURE_BASED_RADIO_CALIBRATION == 1)
  ll_sys_bg_temperature_measurement();
#endif /* USE_TEMPERATURE_BASED_RADIO_CALIBRATION */
}
