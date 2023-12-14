/*
 * Copyright (c) 2018, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <error.h>
#include <stdint.h>
#include <stm32wbaxx.h>

#include "tfm_api.h"
#include "tfm_veneers.h"
#include "tfm_secure_api.h"
#include "tfm/tfm_spm_services.h"
#include "psa/service.h"
#include "psa_manifest/pid.h"
#include "tfm_plat_test.h"

#define DEBUG_RADIO_SERVICE (1)
#if DEBUG_RADIO_SERVICE
#include "log/tfm_log.h"
#endif /*DEBUG_RADIO_SERVICE*/

#ifdef TFM_PARTITION_RADIO_SERVICE

#define RADIO_IRQ_NUM (RADIO_IRQn)

#define RADIO_IRQ_PRIO_HIGH_NS (0)
#define RADIO_IRQ_PRIO_LOW_NS  (5)

/* We don't want the Radio IRQ to be higher than SVC */
#define RADIO_IRQ_PRIO_HIGH_S_MAP (1)
#define RADIO_IRQ_PRIO_LOW_S_MAP  (3)

/* Fast mapping of NS RadioIRQ priority to S RadioIRQ priority */
#define RADIO_IRQ_S_TO_NS_PRIO(PRIO) (PRIO == RADIO_IRQ_PRIO_HIGH_S_MAP ? RADIO_IRQ_PRIO_HIGH_NS : RADIO_IRQ_PRIO_LOW_NS)
#define RADIO_IRQ_NS_TO_S_PRIO(PRIO) (PRIO == RADIO_IRQ_PRIO_HIGH_NS ? RADIO_IRQ_PRIO_HIGH_NS : RADIO_IRQ_PRIO_LOW_NS)

#define GET_BASEPRI_PRIO(X) (X >> __NVIC_PRIO_BITS)

typedef void (*vtor_func_t)(void);
typedef void (*ns_funcptr_t)(void) __attribute__((cmse_nonsecure_call));

typedef struct mutex{
  uint32_t primask_s;
}mutex_t;

#if DEBUG_RADIO_SERVICE
static void TFM_RADIOSERVICE_CheckSNSConsistency();
#endif /*DEBUG_RADIO_SERVICE*/
static void radio_mutex_take(mutex_t *mtx);
static void radio_mutex_release(mutex_t *mtx);
static void tfm_radio_service_abort(void);

/* IT radio is enabled from a NS point of view */
static volatile uint32_t it_radio_enabled_ns = 0;
/* IT radio piority from a NS point of view */
static volatile uint32_t it_radio_prio_ns = 0;
/* Allow to count the nested prmask enabling */
static uint32_t primask_enable_cnt = 0;


#if DEBUG_RADIO_SERVICE

/* This function allows to check that the secure and nonsecure configuration match */
static void TFM_RADIOSERVICE_CheckSNSConsistency(){

  /* The primask is set on the NS, but we have Radio IRQ enabled on S */
  if(__TZ_get_PRIMASK_NS() && NVIC_GetEnableIRQ(RADIO_IRQ_NUM)){
    LOG_MSG("check_s_ns_it_consistency: primask asked from NS but radio enable in S\n");
    tfm_radio_service_abort();
  }
  
  /* The basepri on NS should not enable the Radio IRQ bu it is enabled on the S side */
  if((GET_BASEPRI_PRIO(__TZ_get_BASEPRI_NS()) <= it_radio_prio_ns) && NVIC_GetEnableIRQ(RADIO_IRQ_NUM) && __TZ_get_BASEPRI_NS() != 0){
    LOG_MSG("check_s_ns_it_consistency: baspris asked from NS doesn't match radio enabling in S\n");
    tfm_radio_service_abort();
  }
}


__attribute__((cmse_nonsecure_entry)) uint32_t TFM_RADIOSERVICE_GetRadioPrioNS(){
  return it_radio_prio_ns;
}

__attribute__((cmse_nonsecure_entry)) uint32_t TFM_RADIOSERVICE_GetRadioEnable(){
  return NVIC_GetEnableIRQ(RADIO_IRQ_NUM);
}

#endif /*DEBUG_RADIO_SERVICE*/

void RADIO_IRQHandler(void)
{
  volatile vtor_func_t *vector_table_ns = (vtor_func_t*) (SCB_NS->VTOR);
  volatile vtor_func_t radio_irq_ns_handler = vector_table_ns[RADIO_IRQ_NUM+16];
  ns_funcptr_t nspe_callback = (ns_funcptr_t)cmse_nsfptr_create(radio_irq_ns_handler);

#if DEBUG_RADIO_SERVICE
  TFM_RADIOSERVICE_CheckSNSConsistency();
#endif /*DEBUG_RADIO_SERVICE*/
  
  /* we check the callback before using it */
  if(nspe_callback != NULL && cmse_is_nsfptr(nspe_callback)){
    nspe_callback();
  }
  
#if DEBUG_RADIO_SERVICE
  TFM_RADIOSERVICE_CheckSNSConsistency();
#endif /*DEBUG_RADIO_SERVICE*/
}

static void radio_mutex_take(mutex_t *mtx){
  mtx->primask_s = __get_PRIMASK();
  __disable_irq();
}

static void radio_mutex_release(mutex_t *mtx){
  __set_PRIMASK(mtx->primask_s);
}

__attribute__((cmse_nonsecure_entry)) void TFM_RADIOSERVICE_EnableRadioIt(){
#if DEBUG_RADIO_SERVICE
  TFM_RADIOSERVICE_CheckSNSConsistency();
#endif /*DEBUG_RADIO_SERVICE*/
  
  mutex_t mtx;
  radio_mutex_take(&mtx);
    
  /* from the NS point of view, the RadioIRQ is enabled */
  it_radio_enabled_ns = 1;
  
  /* from the S point of view, we enable the RadioIRQ only if the basepri & primask allows it */
  if((__TZ_get_PRIMASK_NS() == 0) && (GET_BASEPRI_PRIO(__TZ_get_BASEPRI_NS()) > it_radio_prio_ns || __TZ_get_BASEPRI_NS() == 0)){
    HAL_NVIC_EnableIRQ(RADIO_IRQ_NUM);
  }
  
  radio_mutex_release(&mtx);
  
#if DEBUG_RADIO_SERVICE
  TFM_RADIOSERVICE_CheckSNSConsistency();
#endif /*DEBUG_RADIO_SERVICE*/
  
  
}

__attribute__((cmse_nonsecure_entry)) void TFM_RADIOSERVICE_DisableRadioIt(){
#if DEBUG_RADIO_SERVICE
  TFM_RADIOSERVICE_CheckSNSConsistency();
#endif /*DEBUG_RADIO_SERVICE*/
  
  mutex_t mtx;
  radio_mutex_take(&mtx);
    
  /* from the NS point of view, the RadioIRQ is disabled */
  it_radio_enabled_ns = 0;

  /* No need to check the primask & basepri, RadioIRQ is disabled */
  HAL_NVIC_DisableIRQ(RADIO_IRQ_NUM);
  
  radio_mutex_release(&mtx);
  
#if DEBUG_RADIO_SERVICE
  TFM_RADIOSERVICE_CheckSNSConsistency();
#endif /*DEBUG_RADIO_SERVICE*/
}

__attribute__((cmse_nonsecure_entry)) void TFM_RADIOSERVICE_SetRadioPriority(uint32_t NSPrio){
  uint32_t s_prio;
  
#if DEBUG_RADIO_SERVICE
  TFM_RADIOSERVICE_CheckSNSConsistency();
#endif /*DEBUG_RADIO_SERVICE*/
  
  mutex_t mtx;
  radio_mutex_take(&mtx);
    
  /* We save the requested nonSecure priority requested */
  it_radio_prio_ns = NSPrio;
  
  /* Mapping of NS/S Radio priority */
  if(NSPrio == RADIO_IRQ_PRIO_HIGH_NS){
    s_prio = RADIO_IRQ_PRIO_HIGH_S_MAP;
  }else if(NSPrio == RADIO_IRQ_PRIO_LOW_NS){
    s_prio = RADIO_IRQ_PRIO_LOW_S_MAP;
  }else{
#if DEBUG_RADIO_SERVICE
    LOG_MSG("TFM_RADIOSERVICE_SetRadioPriority %u not in map\n", NSPrio);
#endif /*DEBUG_RADIO_SERVICE*/
    tfm_radio_service_abort();
  }
  
  HAL_NVIC_SetPriority(RADIO_IRQ_NUM, s_prio, 0);
  
  /* Enable it if we are higher than basepri register (if primask not set). 
   * If primask and basepri not set, RadioIRQ should already be enabled accorded to NS
   */
  if(it_radio_enabled_ns == 1 && __TZ_get_PRIMASK_NS() == 0 && (GET_BASEPRI_PRIO(__TZ_get_BASEPRI_NS()) > it_radio_prio_ns)){
    HAL_NVIC_EnableIRQ(RADIO_IRQ_NUM);
  }
  
  radio_mutex_release(&mtx);
  
#if DEBUG_RADIO_SERVICE
  TFM_RADIOSERVICE_CheckSNSConsistency();
#endif /*DEBUG_RADIO_SERVICE*/
}

__attribute__((cmse_nonsecure_entry)) void TFM_RADIOSERVICE_EnableIrq(){
#if DEBUG_RADIO_SERVICE
  TFM_RADIOSERVICE_CheckSNSConsistency();
#endif /*DEBUG_RADIO_SERVICE*/

  mutex_t mtx;
  radio_mutex_take(&mtx);
    
  primask_enable_cnt = primask_enable_cnt > 0 ? primask_enable_cnt-1 : 0;
  /* Only enable if the total count of primask enabling has been disabled */
  if(primask_enable_cnt == 0){
    
    __TZ_set_PRIMASK_NS(0);
    /* If the RadioIRQ was requested from NS, we have to reenabled-it */
    if(it_radio_enabled_ns == 1 && ((GET_BASEPRI_PRIO(__TZ_get_BASEPRI_NS()) > it_radio_prio_ns) || __TZ_get_BASEPRI_NS() == 0)){
      HAL_NVIC_EnableIRQ(RADIO_IRQ_NUM);
    }
  
  }
  
  radio_mutex_release(&mtx);
#if DEBUG_RADIO_SERVICE
  TFM_RADIOSERVICE_CheckSNSConsistency();
#endif /*DEBUG_RADIO_SERVICE*/
}

__attribute__((cmse_nonsecure_entry)) void TFM_RADIOSERVICE_DisableIrq(){
#if DEBUG_RADIO_SERVICE
  TFM_RADIOSERVICE_CheckSNSConsistency();
#endif /*DEBUG_RADIO_SERVICE*/
    
  mutex_t mtx;
  radio_mutex_take(&mtx);
    
  HAL_NVIC_DisableIRQ(RADIO_IRQ_NUM);
  __TZ_set_PRIMASK_NS(1);
  primask_enable_cnt++;
  
  radio_mutex_release(&mtx);
  
#if DEBUG_RADIO_SERVICE
  TFM_RADIOSERVICE_CheckSNSConsistency();
#endif /*DEBUG_RADIO_SERVICE*/
}

/* This function disable the primask but saves its value to restore-it directly from the saved value */
__attribute__((cmse_nonsecure_entry)) uint32_t TFM_RADIOSERVICE_DisableIrqWithdirectRestore(){
  uint32_t primask_to_restore;
#if DEBUG_RADIO_SERVICE
  TFM_RADIOSERVICE_CheckSNSConsistency();
#endif /*DEBUG_RADIO_SERVICE*/
    
  mutex_t mtx;
  radio_mutex_take(&mtx);
    
  primask_to_restore = __TZ_get_PRIMASK_NS();
  HAL_NVIC_DisableIRQ(RADIO_IRQ_NUM);
  __TZ_set_PRIMASK_NS(1);
  
  radio_mutex_release(&mtx);
  
#if DEBUG_RADIO_SERVICE
  TFM_RADIOSERVICE_CheckSNSConsistency();
#endif /*DEBUG_RADIO_SERVICE*/
  
  return primask_to_restore;
}

__attribute__((cmse_nonsecure_entry)) void TFM_RADIOSERVICE_SetPrimask(uint32_t NSPrimask){
#if DEBUG_RADIO_SERVICE
  TFM_RADIOSERVICE_CheckSNSConsistency();
#endif /*DEBUG_RADIO_SERVICE*/

  mutex_t mtx;
  radio_mutex_take(&mtx);
    
  if(NSPrimask){
    /* We are in a restoring situation, the radio and primask should be disabled here and we let it as it */
    /* With the current stack, a previous call to DisableIrqWithdirectRestore whould have been done */
    if(__TZ_get_PRIMASK_NS() == 0  || NVIC_GetEnableIRQ(RADIO_IRQ_NUM) == 1){
#if DEBUG_RADIO_SERVICE
      LOG_MSG("TFM_RADIOSERVICE_SetPrimask consistency error\n");
#endif /*DEBUG_RADIO_SERVICE*/
      tfm_radio_service_abort();
    }
  }else{
    
    /* We set the primask as it was previously and we restore the radio if needed */
    __TZ_set_PRIMASK_NS(0);
    if(it_radio_enabled_ns == 1 && ((GET_BASEPRI_PRIO(__TZ_get_BASEPRI_NS()) > it_radio_prio_ns) || __TZ_get_BASEPRI_NS() == 0)){
      HAL_NVIC_EnableIRQ(RADIO_IRQ_NUM);
    }
  }
  
  radio_mutex_release(&mtx);
#if DEBUG_RADIO_SERVICE
  TFM_RADIOSERVICE_CheckSNSConsistency();
#endif /*DEBUG_RADIO_SERVICE*/

}

__attribute__((cmse_nonsecure_entry)) void TFM_RADIOSERVICE_SetBasepri(uint32_t NSBasepri){
#if DEBUG_RADIO_SERVICE
  TFM_RADIOSERVICE_CheckSNSConsistency();
#endif /*DEBUG_RADIO_SERVICE*/
    
  mutex_t mtx;
  radio_mutex_take(&mtx);
    
  uint32_t basepri_prio = GET_BASEPRI_PRIO(NSBasepri);
  if(basepri_prio <= it_radio_prio_ns){
    HAL_NVIC_DisableIRQ(RADIO_IRQ_NUM);
  }else{
    if(it_radio_enabled_ns == 1 && __TZ_get_PRIMASK_NS() != 0){
      HAL_NVIC_EnableIRQ(RADIO_IRQ_NUM);
    }
  }

  radio_mutex_release(&mtx);
#if DEBUG_RADIO_SERVICE
  TFM_RADIOSERVICE_CheckSNSConsistency();
#endif /*DEBUG_RADIO_SERVICE*/
}

__attribute__((cmse_nonsecure_entry)) void TFM_RADIOSERVICE_SetBaseprimax(uint32_t NSBaseprimax){
  
  mutex_t mtx;
  radio_mutex_take(&mtx);
    
  /* An update will be done only if new value will be less than previous (or if previous was 0)*/
  if((NSBaseprimax < __TZ_get_BASEPRI_NS()) || __TZ_get_BASEPRI_NS() == 0){
    TFM_RADIOSERVICE_SetBasepri(NSBaseprimax);
  }

  radio_mutex_release(&mtx);
  
#if DEBUG_RADIO_SERVICE
  TFM_RADIOSERVICE_CheckSNSConsistency();
#endif /*DEBUG_RADIO_SERVICE*/
}
     
__attribute__((cmse_nonsecure_entry)) void TFM_RADIOSERVICE_EnterWfi(){
#if DEBUG_RADIO_SERVICE
  TFM_RADIOSERVICE_CheckSNSConsistency();
#endif /*DEBUG_RADIO_SERVICE*/
  
  mutex_t mtx;
  radio_mutex_take(&mtx);
  
  /* The radio should be enabled, it has been disabled because of the NS/S consistency :
   * With current stack implementation, a previous call to CRITICAL_ENTER has been made, disabling the RadioIRQ in S
   *
   * We have to mimic the primask and RADIOIrq enabling on the NS side 
   * - Primask set by NS with RadioIRQ enabled should wake up from the WFI, the IT will fire when primask will be unset
   * - With S use, we have to reactivate the Radio IT for it to wake up the WFI
   */
  if(it_radio_enabled_ns == 1){
    /* Radio has been disabled on S, but NS only wanted a primask. We enable it for RadioIRQ to wake-up the part */
    HAL_NVIC_EnableIRQ(RADIO_IRQ_NUM);
  }
  
  volatile uint32_t rng_cr_save = RNG->CR;
  RNG->CR &= ~RNG_CR_RNGEN;
  
  /* Thanks to radio_mutex_take, the part will only be wakeup on IRQ without IRQ handler to execute */
  
  __WFI();
  
  RNG->CR = rng_cr_save;
  
  /* We let a future call to TFM_RADIOSERVICE_{primask_basepri} modification to re-enable  it */
  if(it_radio_enabled_ns == 1){
    HAL_NVIC_DisableIRQ(RADIO_IRQ_NUM);
  }
  
  radio_mutex_release(&mtx);
#if DEBUG_RADIO_SERVICE
  TFM_RADIOSERVICE_CheckSNSConsistency();
#endif /*DEBUG_RADIO_SERVICE*/
}

psa_status_t radio_service_init(void)
{
    NVIC_ClearTargetState(RADIO_IRQ_NUM);
    
#ifdef TFM_PSA_API
    while (1) {
      psa_wait(PSA_WAIT_ANY, PSA_BLOCK);
    }
#endif /*TFM_PSA_API*/
}

static void tfm_radio_service_abort(void)
{
    LOG_MSG("tfm_abort from tfm_radio_service_abort\n");
    while (1);
}


#endif
