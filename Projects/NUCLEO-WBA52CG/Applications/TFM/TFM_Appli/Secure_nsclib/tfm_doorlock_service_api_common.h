/**
  ******************************************************************************
  * @file    tfm_doorlock_service_api_common.h
  * @author  MCD Application Team
  * @brief   Shared value between S/NS for doorlock service
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
#ifndef _TFM_DOORLOCK_SERVICE_API_COMMON_H_
#define _TFM_DOORLOCK_SERVICE_API_COMMON_H_

#define KEY_SZ (32)
#define IV_SZ  (12)
#define TAG_SZ  (16)
#define PREAMBLE_SZ (11)
#define CIPHER_SZ (PLAINTEXT_SZ)

#define PLAINTEXT_SZ (32)
#define CHALLENGE_RANDOM_SZ (21)

#define IN_OUT_BUF_SZ (CIPHER_SZ+IV_SZ+TAG_SZ)
#define CHALLENGE_SZ (IN_OUT_BUF_SZ)

#define IOVEC_LEN(x) (sizeof(x)/sizeof(x[0]))

#endif /*_TFM_DOORLOCK_SERVICE_API_COMMON_H_*/