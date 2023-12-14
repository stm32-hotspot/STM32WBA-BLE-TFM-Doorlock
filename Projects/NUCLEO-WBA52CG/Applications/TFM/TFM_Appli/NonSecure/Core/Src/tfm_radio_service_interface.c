/**
  ******************************************************************************
  * @file    tfm_radio_service_interface.c
  * @author  MCD Application Team
  * @brief   Source of radio service interface
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
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "tfm_radio_service_interface.h"

extern void TFM_RADIOSERVICE_EnableRadioIt();
extern void TFM_RADIOSERVICE_DisableRadioIt();
extern void TFM_RADIOSERVICE_SetRadioPriority(uint32_t prio);
extern void TFM_RADIOSERVICE_EnableIrq();
extern void TFM_RADIOSERVICE_DisableIrq();
extern uint32_t TFM_RADIOSERVICE_DisableIrqWithdirectRestore();
extern void TFM_RADIOSERVICE_SetPrimask(uint32_t primask);
extern void TFM_RADIOSERVICE_SetBasepri(uint32_t basepri);
extern void TFM_RADIOSERVICE_SetBaseprimax(uint32_t baseprimax);
extern void TFM_RADIOSERVICE_EnterWfi();

void INTERFACE_TFM_RADIOSERVERICE_EnableRadioIt(){
  TFM_RADIOSERVICE_EnableRadioIt();
}

void INTERFACE_TFM_RADIOSERVERICE_DisableRadioIt(){
  TFM_RADIOSERVICE_DisableRadioIt();
}

void INTERFACE_TFM_RADIOSERVERICE_SetRadioPriority(uint32_t prio){
  TFM_RADIOSERVICE_SetRadioPriority(prio);
}

void INTERFACE_TFM_RADIOSERVERICE_DisableIrq(){
  TFM_RADIOSERVICE_DisableIrq();
}

void INTERFACE_TFM_RADIOSERVERICE_EnableIrq(){
  TFM_RADIOSERVICE_EnableIrq();
}

uint32_t INTERFACE_TFM_RADIOSERVERICE_DisableIrqWithdirectRestore(){
  return TFM_RADIOSERVICE_DisableIrqWithdirectRestore();
}

void INTERFACE_TFM_RADIOSERVERICE_SetPrimask(uint32_t primask){
  TFM_RADIOSERVICE_SetPrimask(primask);
}

void INTERFACE_TFM_RADIOSERVERICE_SetBasepri(uint32_t basepri){
  TFM_RADIOSERVICE_SetBasepri(basepri);
}

void INTERFACE_TFM_RADIOSERVERICE_SetBaseprimax(uint32_t baseprimax){
  TFM_RADIOSERVICE_SetBaseprimax(baseprimax);
}

void INTERFACE_TFM_RADIOSERVERICE_EnterWfi(){
  TFM_RADIOSERVICE_EnterWfi();
}

