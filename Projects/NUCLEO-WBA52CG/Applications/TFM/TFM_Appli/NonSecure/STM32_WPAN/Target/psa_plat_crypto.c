/**
  ******************************************************************************
  * @file    psa_plat_crypto.c
  * @author  MCD Application Team
  * @brief   PSA crypto interface source
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
#if USE_PSA_ENCRYPTION

#include "psa_plat_crypto.h"
#include "app_conf.h"
/* PSA include */
#include "psa/crypto.h"

/* Platform define */
#define PSA_PLAT_CRYPTO_AES_BLOCK_LENGTH         PSA_BLOCK_CIPHER_BLOCK_LENGTH(PSA_KEY_TYPE_AES)
#define PSA_PLAT_CRYPTO_AES_IV_SIZE              PSA_AES_BLOCK_LENGTH  /*The IV is usually one block in size */
#define PSA_PLAT_CRYPTO_AES_KEY_SIZE_BITS        128
#define PSA_PLAT_CRYPTO_AES_KEY_SIZE_BYTES       PSA_BITS_TO_BYTES(PSA_PLAT_CRYPTO_AES_KEY_SIZE_BITS)
#define PSA_PLAT_CRYPTO_AES_CMAC_SIZE_BYTES      PSA_MAC_FINAL_SIZE(PSA_KEY_TYPE_AES, PSA_PLAT_CRYPTO_AES_KEY_SIZE_BITS, PSA_ALG_CMAC)
#define PSA_PLAT_CRYPTO_ECC_PRIV_KEY_SIZE_BITS   256
#define PSA_PLAT_CRYPTO_ECC_PRIV_KEY_SIZE_BYTES  PSA_BITS_TO_BYTES(PSA_PLAT_CRYPTO_ECC_PRIV_KEY_SIZE_BITS)
#define PSA_PLAT_CRYPTO_ECC_PUB_KEY_SIZE_BITS    520
#define PSA_PLAT_CRYPTO_ECC_PUB_KEY_SIZE_BYTES   PSA_BITS_TO_BYTES(PSA_PLAT_CRYPTO_ECC_PUB_KEY_SIZE_BITS)
#define PSA_PLAT_CRYPTO_ECC_DH_KEY_SIZE_BITS     256
#define PSA_PLAT_CRYPTO_ECC_DH_KEY_SIZE_BYTES    PSA_BITS_TO_BYTES(PSA_PLAT_CRYPTO_ECC_DH_KEY_SIZE_BITS)
    
#define BIT32_SWAP( w ) \
        do{ \
        uint8_t t_; \
        t_ = (w)[0]; (w)[0] = (w)[3]; (w)[3] = t_; \
        t_ = (w)[1]; (w)[1] = (w)[2]; (w)[2] = t_; \
        } while(0)
   
#define BIT128_SWAP( w ) \
        do{ \
        uint8_t t_; \
        t_ = (w)[0]; (w)[0] = (w)[15]; (w)[15] = t_; \
        t_ = (w)[1]; (w)[1] = (w)[14]; (w)[14] = t_; \
        t_ = (w)[2]; (w)[2] = (w)[13]; (w)[13] = t_; \
        t_ = (w)[3]; (w)[3] = (w)[12]; (w)[12] = t_; \
        t_ = (w)[4]; (w)[4] = (w)[11]; (w)[11] = t_; \
        t_ = (w)[5]; (w)[5] = (w)[10]; (w)[10] = t_; \
        t_ = (w)[6]; (w)[6] = (w)[9]; (w)[9] = t_; \
        t_ = (w)[7]; (w)[7] = (w)[8]; (w)[8] = t_; \
        } while(0)

/* External declarations */
extern void BPKACB_Complete(void);
extern void BPKACB_Process(void);
extern void HWCB_RNG_Process(void);

/* Internal PSA crypto definition: all the input and ouput are conform to PSA requierement */
static psa_status_t PSA_internal_GenerateRandom(uint8_t* p_Output, size_t Size);
static psa_status_t PSA_internal_AesECBEncrypt(const uint8_t* a_Key, uint8_t* a_Input, uint8_t* a_Output);
static psa_status_t PSA_internal_AesCmacSetKey(const uint8_t* a_Key);
static psa_status_t PSA_internal_AesCmacCompute(const uint8_t* a_Input, uint32_t InputLen, uint8_t* a_Output);
static psa_status_t PSA_internal_PkaStartP256Key(const uint8_t* a_LocalPrivateKey);
static psa_status_t PSA_internal_StartDhKey(const uint8_t* a_LocalPrivateKey, const uint8_t* a_RemotePublicKey);
static psa_status_t PSA_internal_ReadDhKey(uint8_t* a_DhKey);
static void ASN1ToRawECCPublicKeyConvert(uint8_t* a_LocalPublicKey);                         
static void ASN1ToRawECCPrivateKeyConvert(uint8_t* a_PrivateKey);

/* RNG pool */
typedef struct
{
  uint32_t pool[CFG_HW_RNG_POOL_SIZE];
  uint8_t  size;
} PSA_PLAT_CryptoRNGPool_t;

/* Module private global variable */
static psa_key_id_t AesCmacKeyId = NULL;
static uint8_t LocalPublicKey[PSA_PLAT_CRYPTO_ECC_PUB_KEY_SIZE_BYTES] = {0};
static uint8_t DhSharedSecret[PSA_PLAT_CRYPTO_ECC_DH_KEY_SIZE_BYTES] = {0};
static PSA_PLAT_CryptoRNGPool_t RngPool;

int HW_RNG_Process(void)
{
  PSA_PLAT_CryptoRNGPool_t *local_rng_pool = &RngPool;
  psa_status_t status;
  int ret = 0;
  
  if (local_rng_pool->size < CFG_HW_RNG_POOL_SIZE)
  {
    UTILS_ENTER_CRITICAL_SECTION( );
    /* we fill the missing bytes in the pool */
    status = PSA_internal_GenerateRandom((uint8_t*)local_rng_pool->pool, (CFG_HW_RNG_POOL_SIZE-local_rng_pool->size)*sizeof(uint32_t));
    if(status != PSA_SUCCESS){
      assert_param(1);
    }
    
    local_rng_pool->size += CFG_HW_RNG_POOL_SIZE-local_rng_pool->size;
    UTILS_EXIT_CRITICAL_SECTION( );
  }
  
  return ret;
}

/* All the operation have been done, it only need to inform the stack in BG context of the completion */
void BPKA_BG_Process(void)
{
  BPKACB_Complete();
}

void PSA_PLAT_CRYPTO_RngGet(uint8_t n, uint32_t* a_Val)
{
  PSA_PLAT_CryptoRNGPool_t *local_rng_pool = &RngPool;
  uint32_t pool_value;

  while(n--)
  {
    UTILS_ENTER_CRITICAL_SECTION();

    if(local_rng_pool->size == 0)
    {
      pool_value = ~local_rng_pool->pool[n & (CFG_HW_RNG_POOL_SIZE - 1)];
    }
    else
    {
      pool_value = local_rng_pool->pool[--local_rng_pool->size];
    }

    UTILS_EXIT_CRITICAL_SECTION( );
    *a_Val++ = pool_value;
  }

  /* Pool has been reduced, need to fill it in BG */
  HWCB_RNG_Process( );
}

void PSA_PLAT_CRYPTO_AesEcbEncryt(const uint8_t* a_Key, const uint8_t* a_Input, uint8_t* a_Output)
{
  psa_status_t ret;
  uint8_t tmp_key[PSA_PLAT_CRYPTO_AES_KEY_SIZE_BYTES];
  /* input alligned with key size */
  uint8_t tmp_input[PSA_PLAT_CRYPTO_AES_KEY_SIZE_BYTES];
  
  memcpy(tmp_key, a_Key, PSA_PLAT_CRYPTO_AES_KEY_SIZE_BYTES);
  memcpy(tmp_input, a_Input, PSA_PLAT_CRYPTO_AES_KEY_SIZE_BYTES);
  
  /* Convert input to be compliant with the PSA format */
  BIT128_SWAP(tmp_input);
  BIT128_SWAP(tmp_key);
  
  ret = PSA_internal_AesECBEncrypt(tmp_key, tmp_input, a_Output);
  if(ret != PSA_SUCCESS){
    assert_param(1);
  }
  
  /* Convert output to be compliant with the PSA format */
  BIT128_SWAP(a_Output);
}

void PSA_PLAT_CRYPTO_AesCmacSetKey(const uint8_t* a_Key)
{
  psa_status_t ret;
  ret = PSA_internal_AesCmacSetKey(a_Key);
  
  if(ret != PSA_SUCCESS)
  {
    assert_param(1);
  }
}

void PSA_PLAT_CRYPTO_AesCmacCompute(const uint8_t* a_Input, uint32_t InputLenght, uint8_t* a_OutputTag)
{
  psa_status_t ret;
  ret = PSA_internal_AesCmacCompute(a_Input, InputLenght, a_OutputTag);
  
  if(ret != PSA_SUCCESS){
    assert_param(1);
  }
}

int PSA_PLAT_CRYPTO_PkaStartP256Key(const uint32_t* a_LocalPrivateKey)
{
  psa_status_t ret;
  uint8_t tmp_local_private_key[PSA_PLAT_CRYPTO_ECC_PRIV_KEY_SIZE_BYTES];
  
  memcpy(tmp_local_private_key, a_LocalPrivateKey, sizeof(tmp_local_private_key));
  
  ASN1ToRawECCPrivateKeyConvert(tmp_local_private_key);
  
  ret = PSA_internal_PkaStartP256Key(tmp_local_private_key);
  
  if(ret != PSA_SUCCESS){
    assert_param(1);
  }
  
  BPKACB_Process();
  return ret;
}

void PSA_PLAT_CRYPTO_PkaReadP256Key(uint32_t* a_LocalPublicKey)
{
  /* The key in ASN.1 format is already present in LocalPublicKey, only need to discard the 0x4 first byte and convert */
  memcpy(a_LocalPublicKey, &LocalPublicKey[1], PSA_PLAT_CRYPTO_ECC_PUB_KEY_SIZE_BYTES-1);
  /* We convert the output from ASN.1 to BPKA output format (raw 64 bytes) */
  ASN1ToRawECCPublicKeyConvert((uint8_t*)a_LocalPublicKey);
}

int PSA_PLAT_CRYPTO_PkaStartDhKey(const uint32_t* a_LocalPrivateKey, const uint32_t* a_RemotePublicKey)
{
  int ret;
  uint8_t psa_remote_public_key[PSA_PLAT_CRYPTO_ECC_PUB_KEY_SIZE_BYTES];
  uint8_t tmp_local_private_key[PSA_PLAT_CRYPTO_ECC_PRIV_KEY_SIZE_BYTES];
  
  /* ASN.1 formating for PSA */
  psa_remote_public_key[0] = 0x04;
  memcpy(&psa_remote_public_key[1], a_RemotePublicKey, PSA_PLAT_CRYPTO_ECC_PUB_KEY_SIZE_BYTES-1);
  ASN1ToRawECCPublicKeyConvert(&psa_remote_public_key[1]);
  
  /* Key conversion */
  memcpy(tmp_local_private_key, a_LocalPrivateKey, PSA_PLAT_CRYPTO_ECC_PRIV_KEY_SIZE_BYTES);
  ASN1ToRawECCPrivateKeyConvert(tmp_local_private_key);
  
  ret = PSA_internal_StartDhKey(tmp_local_private_key, psa_remote_public_key);
  if(ret != PSA_SUCCESS){
    assert_param(1);
  }
  
  BPKACB_Process();
  return ret;
}

int PSA_PLAT_CRYPTO_PkaReadDhKey(uint32_t* a_DhKey)
{
  int ret;
  
  ret = PSA_internal_ReadDhKey((uint8_t*)a_DhKey);
  if(ret != PSA_SUCCESS){
    assert_param(1);
  }
  
  /* Full revert of the key */
  ASN1ToRawECCPrivateKeyConvert((uint8_t*)a_DhKey);
  return ret;
}

static psa_status_t PSA_internal_GenerateRandom(uint8_t* a_Output, size_t Size)
{
  psa_status_t status;
  
  status = psa_generate_random(a_Output, Size);
  return status;
}

static psa_status_t PSA_internal_AesECBEncrypt(const uint8_t* a_Key, uint8_t* a_Input, uint8_t* a_Output)
{
  psa_status_t status;
  psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
  psa_cipher_operation_t operation = PSA_CIPHER_OPERATION_INIT;
  psa_key_id_t key_id = 0;
  size_t output_len = 0;

  /* Setup the key policy */
  psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
  psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_ENCRYPT);
  psa_set_key_algorithm(&attributes, PSA_ALG_ECB_NO_PADDING);
  psa_set_key_bits(&attributes, PSA_PLAT_CRYPTO_AES_KEY_SIZE_BITS);

  /* Import a key */
  status = psa_import_key(&attributes, a_Key, PSA_PLAT_CRYPTO_AES_KEY_SIZE_BYTES, &key_id);
  if (status != PSA_SUCCESS)
  {
    psa_destroy_key(key_id);
    return status;
  }

  /* Set the key for a multi-part symmetric encryption operation */
  status = psa_cipher_encrypt_setup(&operation, key_id, PSA_ALG_ECB_NO_PADDING);
  if (status != PSA_SUCCESS)
  {
    psa_cipher_abort(&operation);
    psa_destroy_key(key_id);
    return status;
  }

  /* Input equal with key size */
  status = psa_cipher_update(&operation, a_Input, PSA_PLAT_CRYPTO_AES_KEY_SIZE_BYTES, a_Output, PSA_PLAT_CRYPTO_AES_KEY_SIZE_BYTES, &output_len);
  if (status != PSA_SUCCESS)
  {
    psa_cipher_abort(&operation);
    psa_destroy_key(key_id);
    return status;
  }

  /* Finish encrypting a message in a cipher operation */
  status = psa_cipher_finish(&operation, a_Output, PSA_PLAT_CRYPTO_AES_KEY_SIZE_BYTES, &output_len);
  if (status != PSA_SUCCESS)
  {
    psa_cipher_abort(&operation);
    psa_destroy_key(key_id);
    return status;
  }

  /* Destroy the key */
  status = psa_destroy_key(key_id);
  if (status != PSA_SUCCESS)
  {
    return status;
  }

  return PSA_SUCCESS;
}

static psa_status_t PSA_internal_AesCmacSetKey(const uint8_t* a_Key)
{
  psa_status_t status;
  psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

  /* Setup the key policy */
  psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
  psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_HASH);
  psa_set_key_algorithm(&attributes, PSA_ALG_CMAC);
  psa_set_key_bits(&attributes, PSA_PLAT_CRYPTO_AES_KEY_SIZE_BITS);

  /* Import a key */
  status = psa_import_key(&attributes, a_Key, PSA_PLAT_CRYPTO_AES_KEY_SIZE_BYTES, &AesCmacKeyId);
  if (status != PSA_SUCCESS)
  {
    psa_destroy_key(AesCmacKeyId);
    return status;
  }

  return PSA_SUCCESS;
}

static psa_status_t PSA_internal_AesCmacCompute(const uint8_t* a_Input, uint32_t InputLen, uint8_t* a_Output)
{
  psa_status_t status;
  psa_mac_operation_t operation = PSA_MAC_OPERATION_INIT;
  const size_t outputSize = PSA_PLAT_CRYPTO_AES_CMAC_SIZE_BYTES;
  size_t outputLen = 0;

  /* Specify algorithm and key */
  status = psa_mac_sign_setup(&operation, AesCmacKeyId, PSA_ALG_CMAC);
  if (status != PSA_SUCCESS)
  {
    psa_mac_abort(&operation);
    psa_destroy_key(AesCmacKeyId);
    return status;
  }

  status = psa_mac_update(&operation, a_Input, InputLen);
  if (status != PSA_SUCCESS)
  {
    psa_mac_abort(&operation);
    psa_destroy_key(AesCmacKeyId);
    return status;
  }

  /* Calculate the MAC of the message */
  status = psa_mac_sign_finish(&operation, a_Output, outputSize, &outputLen);
  if (status != PSA_SUCCESS)
  {
    psa_mac_abort(&operation);
    psa_destroy_key(AesCmacKeyId);
    return status;
  }

  return PSA_SUCCESS;
}
  
static psa_status_t PSA_internal_PkaStartP256Key(const uint8_t* a_LocalPrivateKey)
{
  psa_status_t status;
  psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
  psa_key_id_t keyId;
  size_t publicKeyLength = 0;
  
  /* Setup the key policy */
  psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
  psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_DERIVE);
  psa_set_key_algorithm(&attributes, PSA_ALG_ECDH);

  /* Import the local private key */
  status = psa_import_key(&attributes, a_LocalPrivateKey, PSA_PLAT_CRYPTO_ECC_PRIV_KEY_SIZE_BYTES, &keyId);
  if (status != PSA_SUCCESS)
  {
    return status;
  }

  /* Export the public key */
  status = psa_export_public_key(keyId, LocalPublicKey, PSA_PLAT_CRYPTO_ECC_PUB_KEY_SIZE_BYTES, &publicKeyLength);
  if (status != PSA_SUCCESS)
  {
    psa_destroy_key(keyId);
    return status;
  }

  return PSA_SUCCESS;
}

static psa_status_t PSA_internal_StartDhKey(const uint8_t* a_LocalPrivateKey, const uint8_t* a_RemotePublicKey)
{
  psa_status_t status;
  psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
  psa_key_id_t local_priv_key_id;
  size_t output_len = 0;
  
  /* Setup the key policy */
  psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
  psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_DERIVE);
  psa_set_key_algorithm(&attributes, PSA_ALG_ECDH);

  /* Import the local private key */
  status = psa_import_key(&attributes, a_LocalPrivateKey, PSA_PLAT_CRYPTO_ECC_PRIV_KEY_SIZE_BYTES, &local_priv_key_id);
  if (status != PSA_SUCCESS)
  {
    return status;
  }
  
  /* Perform a key agreement and return the raw shared secret */
  status = psa_raw_key_agreement(PSA_ALG_ECDH, local_priv_key_id, a_RemotePublicKey, PSA_PLAT_CRYPTO_ECC_PUB_KEY_SIZE_BYTES, DhSharedSecret, PSA_PLAT_CRYPTO_ECC_DH_KEY_SIZE_BYTES, &output_len);
  if (status != PSA_SUCCESS)
  {
    psa_destroy_key(local_priv_key_id);
    return status;
  }
  
  return PSA_SUCCESS;
}

static psa_status_t PSA_internal_ReadDhKey(uint8_t* a_DhKey)
{
  if (a_DhKey == NULL)
  {
    return PSA_ERROR_INVALID_ARGUMENT;
  }

  /* Copy from local buffer to output */
  memcpy(a_DhKey, DhSharedSecret, PSA_PLAT_CRYPTO_ECC_DH_KEY_SIZE_BYTES);

  return PSA_SUCCESS;
}

static void ASN1ToRawECCPublicKeyConvert(uint8_t* a_LocalPublicKey)
{
  uint32_t* local_pubic_key_32 = (uint32_t*)a_LocalPublicKey;
  uint32_t tmp;
  
  /* Invert 16 words array by 8 word packet */
  for(int i=0; i<4; i++)
  {
    tmp = local_pubic_key_32[i];
    local_pubic_key_32[i] = local_pubic_key_32[7-i];
    local_pubic_key_32[7-i] = tmp;
    
    tmp = local_pubic_key_32[(i+8)];
    local_pubic_key_32[(i+8)] = local_pubic_key_32[15-i];
    local_pubic_key_32[15-i] = tmp;
    
    /* we swap all the byte of the words */
    BIT32_SWAP((uint8_t*)&local_pubic_key_32[i]);
    BIT32_SWAP((uint8_t*)&local_pubic_key_32[i+8]);
    BIT32_SWAP((uint8_t*)&local_pubic_key_32[7-i]);
    BIT32_SWAP((uint8_t*)&local_pubic_key_32[15-i]);
  }
}
                            
static void ASN1ToRawECCPrivateKeyConvert(uint8_t* a_PrivateKey)
{
  uint8_t tmp;
  
  /* Full revert of 32bytes array */
  for(int i=0; i<16; i++)
  {
    tmp = a_PrivateKey[i];
    a_PrivateKey[i] = a_PrivateKey[31-i];
    a_PrivateKey[31-i] = tmp;
  }
}

#endif /*USE_PSA_ENCRYPTION*/