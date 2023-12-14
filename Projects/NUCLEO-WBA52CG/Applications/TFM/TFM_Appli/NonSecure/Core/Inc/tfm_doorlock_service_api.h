/**
  ******************************************************************************
  * @file    tfm_doorlock_service_api.h
  * @author  MCD Application Team
  * @brief   Interface for doorlock service
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
#ifndef _TFM_DOORLOCK_SERVICE_API_H_
#define _TFM_DOORLOCK_SERVICE_API_H_

#include <stdint.h>
#include <stdlib.h>
#include "psa\crypto.h"

psa_status_t psa_doorlock_request_open(uint8_t       *challenge_buf,
                             size_t         challenge_buf_size,
                             size_t        *challenge_size);

psa_status_t
psa_doorlock_answer_challenge(uint8_t       *answer_challenge_buf,
                             size_t         answer_challenge_buf_size,
                             uint8_t        *open_answer);

#endif /*_TFM_DOORLOCK_SERVICE_API_H_*/