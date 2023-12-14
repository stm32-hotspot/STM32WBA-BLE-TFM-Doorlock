/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    bleplat.c
  * @author  MCD Application Team
  * @brief   This file implements the platform functions for BLE stack library.
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
#include "bleplat.h"
#include "nvm.h"
#if USE_PSA_ENCRYPTION
#include "psa_plat_crypto.h"
#else
#include "baes.h"
#include "bpka.h"
#endif /*USE_PSA_ENCRYPTION*/
#include "ble_timer.h"

/*****************************************************************************/

void BLEPLAT_Init( void )
{
#if !USE_PSA_ENCRYPTION
  BAES_Reset( );
  BPKA_Reset( );
#endif /*USE_PSA_ENCRYPTION*/
  BLE_TIMER_Init();
}

/*****************************************************************************/

int BLEPLAT_NvmAdd( uint8_t type,
                    const uint8_t* data,
                    uint16_t size,
                    const uint8_t* extra_data,
                    uint16_t extra_size )
{
  return NVM_Add( type, data, size, extra_data, extra_size );
}

/*****************************************************************************/

int BLEPLAT_NvmGet( uint8_t mode,
                    uint8_t type,
                    uint16_t offset,
                    uint8_t* data,
                    uint16_t size )
{
  return NVM_Get( mode, type, offset, data, size );
}

/*****************************************************************************/

int BLEPLAT_NvmCompare( uint16_t offset,
                        const uint8_t* data,
                        uint16_t size )
{
  return NVM_Compare( offset, data, size );
}

/*****************************************************************************/

void BLEPLAT_NvmDiscard( uint8_t mode )
{
  NVM_Discard( mode );
}

/*****************************************************************************/

void BLEPLAT_RngGet( uint8_t n,
                     uint32_t* val )
{
  /* Read 32-bit random values from HW driver */
#if USE_PSA_ENCRYPTION
  PSA_PLAT_CRYPTO_RngGet(n, val);
#else
  HW_RNG_Get( n, val );
#endif /*USE_PSA_ENCRYPTION*/
}

/*****************************************************************************/

void BLEPLAT_AesEcbEncrypt( const uint8_t* key,
                            const uint8_t* input,
                            uint8_t* output )
{
#if USE_PSA_ENCRYPTION
  PSA_PLAT_CRYPTO_AesEcbEncryt(key, input, output);
#else
  BAES_EcbCrypt( key, input, output, 1 );
#endif /*USE_PSA_ENCRYPTION*/
}

/*****************************************************************************/

void BLEPLAT_AesCmacSetKey( const uint8_t* key )
{
#if USE_PSA_ENCRYPTION
  PSA_PLAT_CRYPTO_AesCmacSetKey(key);
#else
  BAES_CmacSetKey( key );
#endif /*USE_PSA_ENCRYPTION*/

}

/*****************************************************************************/

void BLEPLAT_AesCmacCompute( const uint8_t* input,
                             uint32_t input_length,
                             uint8_t* output_tag )
{
#if USE_PSA_ENCRYPTION
  PSA_PLAT_CRYPTO_AesCmacCompute(input, input_length, output_tag);
#else
  BAES_CmacCompute( input, input_length, output_tag );
#endif /*USE_PSA_ENCRYPTION*/
}

/*****************************************************************************/

int BLEPLAT_PkaStartP256Key( const uint32_t* local_private_key )
{
  int ret;
#if USE_PSA_ENCRYPTION
  ret = PSA_PLAT_CRYPTO_PkaStartP256Key(local_private_key);
#else
  ret = BPKA_StartP256Key( local_private_key );
#endif /*USE_PSA_ENCRYPTION*/
  
  return ret;
}

/*****************************************************************************/

void BLEPLAT_PkaReadP256Key( uint32_t* local_public_key )
{

#if USE_PSA_ENCRYPTION
  PSA_PLAT_CRYPTO_PkaReadP256Key(local_public_key);
#else
  BPKA_ReadP256Key( local_public_key );
#endif /*USE_PSA_ENCRYPTION*/
}

/*****************************************************************************/

int BLEPLAT_PkaStartDhKey( const uint32_t* local_private_key,
                           const uint32_t* remote_public_key )
{
  int ret;
  
#if USE_PSA_ENCRYPTION
  ret = PSA_PLAT_CRYPTO_PkaStartDhKey(local_private_key, remote_public_key);
#else
  ret = BPKA_StartDhKey( local_private_key, remote_public_key );
#endif /*USE_PSA_ENCRYPTION*/

  return ret;
}

/*****************************************************************************/

int BLEPLAT_PkaReadDhKey( uint32_t* dh_key )
{
  int ret;
  
#if USE_PSA_ENCRYPTION
  ret = PSA_PLAT_CRYPTO_PkaReadDhKey(dh_key);
#else
  ret = BPKA_ReadDhKey( dh_key );
#endif /*USE_PSA_ENCRYPTION*/

  return ret;
}

/*****************************************************************************/

void BPKACB_Complete( void )
{
  BLEPLATCB_PkaComplete( );
  HostStack_Process( );
}

/*****************************************************************************/

uint8_t BLEPLAT_TimerStart( uint8_t layer,
                            uint32_t timeout )
{
  return BLE_TIMER_Start( (uint16_t)layer, timeout );
}

/*****************************************************************************/

void BLEPLAT_TimerStop( uint8_t layer )
{
  BLE_TIMER_Stop( (uint16_t)layer );
}

/*****************************************************************************/
