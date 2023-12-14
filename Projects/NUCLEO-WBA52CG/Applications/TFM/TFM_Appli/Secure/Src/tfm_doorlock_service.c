/**
  ******************************************************************************
  * @file    tfm_doorlock_service.c
  * @author  MCD Application Team
  * @brief   Secure service for the doorlock application
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
#include <error.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stm32wbaxx.h>

#include "tfm_doorlock_service.h"
#include "tfm_doorlock_service_api_common.h"

#include "tfm_api.h"
#include "tfm_veneers.h"
#include "tfm_secure_api.h"
#include "tfm/tfm_spm_services.h"
#include "tfm_plat_test.h"

#include "log/tfm_log.h"
#include "low_level_rng.h"

#include "psa/service.h"
#include "psa_manifest/pid.h"
#include "mbedtls/gcm.h"

/* LCD include */
#include "hal_lcd.h"

/* Image manipulation include */
#include "bootutil_priv.h"
#include "flash_layout.h"
#include "Driver_Flash.h"
#include "region_defs.h"

#ifdef TFM_PARTITION_DOORLOCK_SERVICE

#if (CIPHER_SZ%16)
#error "Cipher should be a multiple of 16bytes"
#endif

#define LCD_ENABLE (1)
#define ELECTROMAGNET_ENABLE (1)
#define TFM_PARTITION_LOG (0)

/* GPIO for the DL electromagnet */
#define GPIO_PORT_LOCK    GPIOB
#define GPIO_PIN_LOCK     GPIO_PIN_13
#define GPIO_LOCK_CLOCK_ENABLE __HAL_RCC_GPIOB_CLK_ENABLE

/* DOORLOCK Close Timeout : 5s*(32kHz/LPTIM_PRESCALER_DIV4)*/
#define DL_CLOSE_TIMEOUT              (uint32_t) (0x9C40)

/* Allow to complete X/Y symetry of the screen */
#define DL_SCREEN_UPSIDE_DOWN (0)

typedef psa_status_t (*attest_func_t)(const psa_msg_t *msg);
typedef void (*vtor_func)(void);
typedef void (*ns_funcptr)(void) __attribute__((cmse_nonsecure_call));

typedef enum{
  DL_StateWaitChallengeRequest = 0x01,
  DL_StateWaitChallengeSolve   = 0x02,
}DL_State_t;

extern logo_t lock_open_logo;
extern logo_t lock_close_logo;
extern logo_t denied_logo;

static void DL_InitTimer();
static void DL_LaunchDoorCloseTimer();
#if ELECTROMAGNET_ENABLE
static void DL_ElectromagnetGpioInit();
static void DL_ElectromagnetGpioSetOpen();
static void DL_ElectromagnetGpioSetClose();
#endif /*ELECTROMAGNET_ENABLE*/
#if LCD_ENABLE
static void LCD_PrintOpen(void);
static void LCD_PrintClose(void);
static void LCD_PrintDenied(void);
static void LCD_BindSecure(void);
#endif /*LCD_ENABLE*/

static void tfm_doorlock_abort(void);

static DL_State_t DL_CurrentState;
/* Allow to let the compiler check the compliance between preamble size and the string size */
static char DL_ChallengePreamble[PREAMBLE_SZ+1]        = "[CHALLENGE]";
static char DL_ChallengeAnswerPreamble[PREAMBLE_SZ+1]  = "[ANSWER_CH]";
static uint8_t DL_ChallengeRandom[CHALLENGE_RANDOM_SZ] = {0};
static LPTIM_HandleTypeDef LptimHandle;

#if (MCUBOOT_S_DATA_IMAGE_NUMBER == 1)
/* Access to Secure Data partition if exists, which hold an updatable key */
static uint8_t *DL_KeyGCM = (uint8_t*)(FLASH_BASE_S + S_DATA_IMAGE_PRIMARY_PARTITION_OFFSET + S_DATA_IMAGE_DOORLOCK_KEY);
#else
/* Default key if no Secure Data partition */
static uint8_t DL_KeyGCM[KEY_SZ] = {0xff, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xff, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
#endif


#if DL_SCREEN_UPSIDE_DOWN
uint8_t DL_UpdateScreen = false;
#else
uint8_t DL_UpdateScreen = true;
#endif /*DL_SCREEN_UPSIDE_DOWN*/

#if LCD_ENABLE
static void LCD_PrintOpen(void){

  LCD_Print1stLine("     Open", DL_UpdateScreen);
  LCD_PrintLogo(&lock_open_logo, 0, DL_UpdateScreen);
#if DL_SCREEN_UPSIDE_DOWN
  LCD_Symmetry_X_Y();
#endif /*DL_SCREEN_UPSIDE_DOWN*/
}

static void LCD_PrintClose(void){
  
  LCD_Print1stLine("     Closed", DL_UpdateScreen);
  LCD_PrintLogo(&lock_close_logo, 0, DL_UpdateScreen);
#if DL_SCREEN_UPSIDE_DOWN
  LCD_Symmetry_X_Y();
#endif /*DL_SCREEN_UPSIDE_DOWN*/
}

static void LCD_PrintDenied(void){
  
  LCD_Print1stLine("     Denied", DL_UpdateScreen);
  LCD_PrintLogo(&denied_logo, 0, DL_UpdateScreen);
#if DL_SCREEN_UPSIDE_DOWN
  LCD_Symmetry_X_Y();
#endif /*DL_SCREEN_UPSIDE_DOWN*/
}

static void LCD_BindSecure(void){
  HAL_GTZC_TZSC_ConfigPeriphAttributes(GTZC_PERIPH_I2C3, GTZC_TZSC_PERIPH_SEC | GTZC_TZSC_PERIPH_NPRIV);
  GPIOA_S->SECCFGR |= GPIO_PIN_6 | GPIO_PIN_7;
}
#endif /*LCD_ENABLE*/

void RNG_KERNEL_CLK_ON(void)
{
  LL_RCC_HSI_Enable();
  while(LL_RCC_HSI_IsReady() == 0);
}

static psa_status_t get_random(uint8_t *pBuf, uint32_t BufLen){
  uint32_t random_generate_sz;

  RNG_GetBytes(pBuf, BufLen, &random_generate_sz);

  if(random_generate_sz != BufLen){
    return PSA_ERROR_SERVICE_FAILURE;
  }
  return PSA_SUCCESS;
}

static psa_status_t psa_doorlock_request_open(const psa_msg_t *pMsg)
{
  psa_status_t status = PSA_SUCCESS;
  mbedtls_gcm_context aes;
  
  uint8_t plaintext[PLAINTEXT_SZ] = {0};
  uint8_t cipher[CIPHER_SZ] = {0};
  uint8_t challenge_buf[IN_OUT_BUF_SZ];
  uint8_t iv[IV_SZ] = {0};
  uint8_t tag[TAG_SZ] = {0};
  
  if(sizeof(DL_ChallengePreamble) + sizeof(DL_ChallengeRandom) != sizeof(plaintext) && sizeof(plaintext) % 16){
    tfm_doorlock_abort();
  }
  
  status = get_random(DL_ChallengeRandom, CHALLENGE_RANDOM_SZ);
  if(status != PSA_SUCCESS){
    return status;
  }
  
  status = get_random(iv, IV_SZ);
  if(status != PSA_SUCCESS){
    return status;
  }

  /* not enought room in the current inout buffer for the challenge */
  if(IN_OUT_BUF_SZ < CIPHER_SZ){
    tfm_doorlock_abort();
  }
  
  memcpy(plaintext, DL_ChallengePreamble, strlen(DL_ChallengePreamble));
  memcpy(&plaintext[strlen(DL_ChallengePreamble)], DL_ChallengeRandom, CHALLENGE_RANDOM_SZ);
  
  size_t challenge_size = pMsg->out_size[0];
  
  psa_outvec out_vec[] = {
    {challenge_buf, sizeof(challenge_buf)}
  };
  
  if (challenge_size != IN_OUT_BUF_SZ) {
    return PSA_ERROR_INVALID_ARGUMENT;
  }
  
  if (challenge_size < sizeof(challenge_buf)) {
    out_vec[0].len = challenge_size;
  }
  
  mbedtls_gcm_init(&aes);
  mbedtls_gcm_setkey(&aes, MBEDTLS_CIPHER_ID_AES, DL_KeyGCM, KEY_SZ*8);
  mbedtls_gcm_starts(&aes, MBEDTLS_GCM_ENCRYPT, iv, IV_SZ, NULL, 0);
  mbedtls_gcm_update(&aes, PLAINTEXT_SZ, plaintext, cipher);
  mbedtls_gcm_finish(&aes, tag, TAG_SZ);
  mbedtls_gcm_free(&aes);

  /* size has been checked previously */
  memcpy(&challenge_buf[0], iv, IV_SZ);
  memcpy(&challenge_buf[IV_SZ], cipher, CIPHER_SZ);
  memcpy(&challenge_buf[IV_SZ+CIPHER_SZ], tag, TAG_SZ);
  
  if (status == PSA_SUCCESS) {
    psa_write(pMsg->handle, 0, out_vec[0].base, out_vec[0].len);
  }
  
  DL_CurrentState = DL_StateWaitChallengeSolve;
  return status;
}

static psa_status_t psa_doorlock_answer_challenge_open(const psa_msg_t *pMsg)
{
  psa_status_t status = PSA_SUCCESS;
  mbedtls_gcm_context aes;
  
  uint8_t challenge_buf[IN_OUT_BUF_SZ];
  uint8_t iv[IV_SZ] = {0};
  uint8_t tag[TAG_SZ] = {0};
  uint8_t plaintext[PLAINTEXT_SZ] = {0};
  uint8_t cipher[CIPHER_SZ] = {0};
    
  uint8_t doorlock_service_answer = 0;
  uint32_t bytes_read = 0;
  size_t challenge_size = pMsg->in_size[0];
  size_t answer_size = pMsg->out_size[0];
    
  /* Not in the right state */
  if(DL_CurrentState != DL_StateWaitChallengeSolve){
    return PSA_ERROR_BAD_STATE;
  }
    
  DL_CurrentState = DL_StateWaitChallengeRequest;
    
  psa_outvec out_vec[] = {
    {&doorlock_service_answer, sizeof(doorlock_service_answer)}
  };

  if (challenge_size != IN_OUT_BUF_SZ) {
    tfm_doorlock_abort();
  }
  if (answer_size < sizeof(doorlock_service_answer)) {
      out_vec[0].len = answer_size;
  }
  
  bytes_read = psa_read(pMsg->handle, 0, challenge_buf, challenge_size);
  if (bytes_read != challenge_size) {
    return PSA_ERROR_GENERIC_ERROR;
  }
  
  memcpy(iv, &challenge_buf[0], IV_SZ);
  memcpy(cipher, &challenge_buf[IV_SZ], CIPHER_SZ);
  memcpy(tag, &challenge_buf[IV_SZ+CIPHER_SZ], TAG_SZ);
  
  mbedtls_gcm_init(&aes);
  mbedtls_gcm_setkey(&aes, MBEDTLS_CIPHER_ID_AES , DL_KeyGCM, KEY_SZ*8);
  mbedtls_gcm_starts(&aes, MBEDTLS_GCM_DECRYPT, iv, IV_SZ, NULL, 0);
  mbedtls_gcm_update(&aes, PLAINTEXT_SZ, cipher, plaintext);
  mbedtls_gcm_free(&aes);
    
  /* size has already been checked */
  if(memcmp(plaintext, DL_ChallengeAnswerPreamble, PREAMBLE_SZ) == 0 && memcmp(&plaintext[PREAMBLE_SZ], DL_ChallengeRandom, CHALLENGE_RANDOM_SZ) == 0){
#if TFM_PARTITION_LOG
    LOG_MSG("Challenge success\r\n");
#endif /*TFM_PARTITION_LOG*/
    doorlock_service_answer = 1;
#if ELECTROMAGNET_ENABLE
    DL_ElectromagnetGpioSetOpen();
#endif /*ELECTROMAGNET_ENABLE*/
#if LCD_ENABLE
    LCD_PrintOpen();
#endif /*LCD_ENABLE*/
  }else{
#if TFM_PARTITION_LOG
    LOG_MSG("Challenge error\r\n");
#endif /*TFM_PARTITION_LOG*/
    doorlock_service_answer = 0;
#if ELECTROMAGNET_ENABLE
    DL_ElectromagnetGpioSetClose();
#endif /*ELECTROMAGNET_ENABLE*/
#if LCD_ENABLE
    LCD_PrintDenied();
#endif /*LCD_ENABLE*/
  }
  
  if (status == PSA_SUCCESS) {
    psa_write(pMsg->handle, 0, out_vec[0].base, out_vec[0].len);
  }
  
  if(doorlock_service_answer == 1){
    DL_LaunchDoorCloseTimer();
  }
  
  return status;
}


static void doorlock_signal_handle(psa_signal_t signal, attest_func_t pfn)
{
  psa_msg_t msg;
  psa_status_t status;
  
  status = psa_get(signal, &msg);
  switch (msg.type) {
    case PSA_IPC_CONNECT:
      psa_reply(msg.handle, PSA_SUCCESS);
      break;
    case PSA_IPC_CALL:
      status = (psa_status_t)pfn(&msg);
      psa_reply(msg.handle, status);
      break;
    case PSA_IPC_DISCONNECT:
      psa_reply(msg.handle, PSA_SUCCESS);
      break;
    default:
      tfm_doorlock_abort();
    }
}

/* The new Secure data is accepted from the doorlock service */
static int AcceptDataSImage(){
  const struct flash_area *fap;
  struct boot_swap_state state_primary_slot;
  int rc;
  
  rc = boot_read_swap_state_by_id(FLASH_AREA_IMAGE_PRIMARY(2), &state_primary_slot);
  
  if (rc != 0) {
    return rc;
  }
  
  switch (state_primary_slot.magic) {
    case BOOT_MAGIC_GOOD:
      /* Confirm needed; proceed. */
    break;

    case BOOT_MAGIC_UNSET:
      /* Already confirmed. */
      return 0;
    break;
    
    case BOOT_MAGIC_BAD:
      /* Unexpected state. */
      return BOOT_EBADVECT;
    break;
    }

    rc = flash_area_open(FLASH_AREA_IMAGE_PRIMARY(2), &fap);
    if (rc) {
        rc = BOOT_EFLASH;
        goto done;
    }

    if (state_primary_slot.image_ok != BOOT_FLAG_UNSET) {
        /* Already confirmed. */
        goto done;
    }

    rc = boot_write_image_ok(fap);

done:
    flash_area_close(fap);
    return rc;
}

void doorlock_timer_irq_handler(void){
  HAL_LPTIM_IRQHandler(&LptimHandle);
  
  HAL_LPTIM_TimeOut_Stop_IT(&LptimHandle);
  
  psa_eoi(TFM_DOORLOCK_TIMER_IRQ);
}

psa_status_t doorlock_service_init(void)
{
    DL_CurrentState = DL_StateWaitChallengeRequest;
    
    AcceptDataSImage();


#if ELECTROMAGNET_ENABLE
    DL_ElectromagnetGpioInit();
    DL_ElectromagnetGpioSetClose();
#endif /*ELECTROMAGNET_ENABLE*/
#if LCD_ENABLE
    LCD_Init();
    LCD_PrintClose();
#endif /*LCD_ENABLE*/
    DL_InitTimer();

#ifdef TFM_PSA_API
    psa_signal_t signals = 0;

    while (1) {
        signals = psa_wait(PSA_WAIT_ANY, PSA_BLOCK);
        
	if (signals & TFM_DOORLOCK_REQUEST_OPEN_SIGNAL) {
          doorlock_signal_handle(TFM_DOORLOCK_REQUEST_OPEN_SIGNAL, psa_doorlock_request_open);
        } else if(signals & TFM_DOORLOCK_ANSWER_CHALLENGE_SIGNAL){
          doorlock_signal_handle(TFM_DOORLOCK_ANSWER_CHALLENGE_SIGNAL, psa_doorlock_answer_challenge_open);
        } else if(signals & TFM_DOORLOCK_TIMER_IRQ){
          doorlock_timer_irq_handler();
        }
        else {
            tfm_doorlock_abort();
        }
    }
#endif /*TFM_PSA_API*/

}

#if ELECTROMAGNET_ENABLE
static void DL_ElectromagnetGpioInit(){
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_LOCK_CLOCK_ENABLE();
  
  GPIO_InitStruct.Pin = GPIO_PIN_LOCK;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIO_PORT_LOCK, &GPIO_InitStruct);
  
}

static void DL_ElectromagnetGpioSetOpen(){
  HAL_GPIO_WritePin(GPIO_PORT_LOCK, GPIO_PIN_LOCK, GPIO_PIN_RESET);
}

static void DL_ElectromagnetGpioSetClose(){
  HAL_GPIO_WritePin(GPIO_PORT_LOCK, GPIO_PIN_LOCK, GPIO_PIN_SET);
}
#endif /*ELECTROMAGNET_ENABLE*/

void DL_InitTimer(){

  LptimHandle.Instance = LPTIM1;
  LptimHandle.Init.Clock.Source = LPTIM_CLOCKSOURCE_APBCLOCK_LPOSC;
  LptimHandle.Init.Clock.Prescaler = LPTIM_PRESCALER_DIV4;
  LptimHandle.Init.Trigger.Source = LPTIM_TRIGSOURCE_SOFTWARE;
  LptimHandle.Init.Period = 0xFFFF;
  LptimHandle.Init.UpdateMode = LPTIM_UPDATE_IMMEDIATE;
  LptimHandle.Init.CounterSource = LPTIM_COUNTERSOURCE_INTERNAL;
  LptimHandle.Init.Input1Source = LPTIM_INPUT1SOURCE_GPIO;
  LptimHandle.Init.Input2Source = LPTIM_INPUT2SOURCE_GPIO;
  LptimHandle.Init.RepetitionCounter = 0;
  
  NVIC_ClearTargetState(LPTIM1_IRQn);
  
  if (HAL_LPTIM_Init(&LptimHandle) != HAL_OK)
  {
    Error_Handler();
  }

}


void DL_LaunchDoorCloseTimer(){
  HAL_LPTIM_TimeOut_Start_IT(&LptimHandle, DL_CLOSE_TIMEOUT);
}

void HAL_LPTIM_CompareMatchCallback(LPTIM_HandleTypeDef *htim)
{
  volatile vtor_func *vector_table_ns = (vtor_func*) (SCB_NS->VTOR);
  volatile vtor_func timer_irq_ns_handler = vector_table_ns[LPTIM1_IRQn+16];
  ns_funcptr nspe_callback = (ns_funcptr)cmse_nsfptr_create(timer_irq_ns_handler);
   
#if ELECTROMAGNET_ENABLE
  DL_ElectromagnetGpioSetClose();
#endif /*ELECTROMAGNET_ENABLE*/
#if LCD_ENABLE
  LCD_PrintClose();
#endif /*LCD_ENABLE*/
  
  /* we check the callback before using it */
  if(nspe_callback != NULL && cmse_is_nsfptr(nspe_callback)){
    nspe_callback();
  }
  
}


void HAL_LPTIM_MspInit(LPTIM_HandleTypeDef *hlptim)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  
  if(hlptim->Instance==LPTIM1)
  {
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_LPTIM1;
    PeriphClkInit.Lptim1ClockSelection = RCC_LPTIM1CLKSOURCE_LSI;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    /* Peripheral clock enable */
    __HAL_RCC_LPTIM1_CLK_ENABLE();
    /* LPTIM1 interrupt Init */
    HAL_NVIC_SetPriority(LPTIM1_IRQn, 64, 0);
    HAL_NVIC_EnableIRQ(LPTIM1_IRQn);
  }
  
}

#if LCD_ENABLE

void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* USER CODE BEGIN I2C3_MspInit 0 */

  /* USER CODE END I2C3_MspInit 0 */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  
  /**I2C3 GPIO Configuration
    PA6     ------> I2C3_SCL
    PA7     ------> I2C3_SDA
  */
  GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* Peripheral clock enable */
  __HAL_RCC_GPIOA_CLK_DISABLE();
  __HAL_RCC_I2C3_CLK_ENABLE();
  /* USER CODE BEGIN I2C3_MspInit 1 */
  LCD_BindSecure();
  /* USER CODE END I2C3_MspInit 1 */
}

/**
* @brief I2C MSP De-Initialization
* This function freeze the hardware resources used in this example
* @param hi2c: I2C handle pointer
* @retval None
*/
void HAL_I2C_MspDeInit(I2C_HandleTypeDef* hi2c)
{
  /* Peripheral clock disable */
  __HAL_RCC_I2C3_CLK_DISABLE();

  /**I2C3 GPIO Configuration
    PA6     ------> I2C3_SCL
    PA7     ------> I2C3_SDA
  */
  HAL_GPIO_DeInit(GPIOA, GPIO_PIN_6|GPIO_PIN_7);
}
#endif /*LCD_ENABLE*/

static void tfm_doorlock_abort(void)
{
    LOG_MSG("tfm_doorlock_abort from tfm_doorlock_service\n");
    while (1);
}


#endif