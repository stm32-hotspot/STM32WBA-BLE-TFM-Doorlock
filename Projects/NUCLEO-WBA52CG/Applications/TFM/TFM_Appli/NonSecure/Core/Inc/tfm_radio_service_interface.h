/**
  ******************************************************************************
  * @file    tfm_radio_service_interface.h
  * @author  MCD Application Team
  * @brief   Interface for radio service
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
#ifndef _TFM_RADIO_SERVICE_INTERFACE_H_
#define _TFM_RADIO_SERVICE_INTERFACE_H_

/* Enable the Radio IRQ in Secure Side via the TF-M Secure Service */
void INTERFACE_TFM_RADIOSERVERICE_EnableRadioIt();

/* Disable the Radio IRQ in Secure Side via the TF-M Secure Service */
void INTERFACE_TFM_RADIOSERVERICE_DisableRadioIt();

/* Change Radio IRQ priority in Secure Side via the TF-M Secure Service (re-map of NS/S priority will happen) */
void INTERFACE_TFM_RADIOSERVERICE_SetRadioPriority(uint32_t prio);

/* Allow to add the Secure Radio IRQ in the NS Primask activation in order for the Radio Stack to be coherent */
void INTERFACE_TFM_RADIOSERVERICE_DisableIrq();

/* Allow to add the Secure Radio IRQ in the NS Primask deactivation in order for the Radio Stack to be coherent */
void INTERFACE_TFM_RADIOSERVERICE_EnableIrq();

/* Equivalent of INTERFACE_TFM_RADIOSERVERICE_DisableIrq but the previous primask is returned so a call to
   INTERFACE_TFM_RADIOSERVERICE_SetPrimask can be done to restore it */
uint32_t INTERFACE_TFM_RADIOSERVERICE_DisableIrqWithdirectRestore();

/* Restore previous primask and add coherence with the Secure Radio IRQ enabling/disabling */
void INTERFACE_TFM_RADIOSERVERICE_SetPrimask(uint32_t primask);

/* Allow to add the Secure Radio IRQ in the basepri activation in order for the Radio Stack to be coherent */
void INTERFACE_TFM_RADIOSERVERICE_SetBasepri(uint32_t basepri);

/* Allow to add the Secure Radio IRQ in the baseprimax activation in order for the Radio Stack to be coherent */
void INTERFACE_TFM_RADIOSERVERICE_SetBaseprimax(uint32_t baseprimax);

/* Will enter the WFI, if needed will temporarly enable RadioIRQ to be wake-up by it */
void INTERFACE_TFM_RADIOSERVERICE_EnterWfi();

#endif /*_TFM_RADIO_SERVICE_INTERFACE_H_*/
