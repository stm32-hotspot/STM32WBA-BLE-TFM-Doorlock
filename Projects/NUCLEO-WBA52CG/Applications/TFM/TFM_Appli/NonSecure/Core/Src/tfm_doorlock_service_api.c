/**
  ******************************************************************************
  * @file    tfm_doorlock_service_api.c
  * @author  MCD Application Team
  * @brief   Source of Doorlock secure service API
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

#include "tfm_doorlock_service_api.h"
#include "psa/client.h"
#include "tfm_veneers.h"

#ifdef TFM_PSA_API
#include "psa_manifest/sid.h"
#endif
#include <string.h>

#define IOVEC_LEN(x) (sizeof(x)/sizeof(x[0]))

psa_status_t
psa_doorlock_request_open(uint8_t       *challenge_buf,
                             size_t         challenge_buf_size,
                             size_t        *challenge_size)
{
    psa_status_t status;

    psa_outvec out_vec[] = {
        {challenge_buf, challenge_buf_size}
    };

#ifdef TFM_PSA_API
    psa_handle_t handle = PSA_NULL_HANDLE;
    handle = psa_connect(TFM_DOORLOCK_REQUEST_OPEN_SID,
                         TFM_DOORLOCK_REQUEST_OPEN_VERSION);
    if (!PSA_HANDLE_IS_VALID(handle)) {
        return PSA_HANDLE_TO_ERROR(handle);
    }

    status = psa_call(handle, PSA_IPC_CALL,
                      NULL, 0,
                      out_vec, IOVEC_LEN(out_vec));
    psa_close(handle);
#else
#error "psa_doorlock_request_open not available without PSA_API"
#endif
    if (status == PSA_SUCCESS) {
        *challenge_size = out_vec[0].len;
    }

    return status;
}

psa_status_t
psa_doorlock_answer_challenge(uint8_t       *answer_challenge_buf,
                             size_t         answer_challenge_buf_size,
                             uint8_t        *open_answer)
{
    psa_status_t status;
    psa_invec in_vec[] = {
        {answer_challenge_buf, answer_challenge_buf_size}
    };
    psa_outvec out_vec[] = {
        {open_answer, sizeof(*open_answer)}
    };

#ifdef TFM_PSA_API
    psa_handle_t handle = PSA_NULL_HANDLE;
    handle = psa_connect(TFM_DOORLOCK_ANSWER_CHALLENGE_SID,
                         TFM_DOORLOCK_ANSWER_CHALLENGE_VERSION);
    if (!PSA_HANDLE_IS_VALID(handle)) {
        return PSA_HANDLE_TO_ERROR(handle);
    }

    status = psa_call(handle, PSA_IPC_CALL,
                      in_vec, IOVEC_LEN(in_vec),
                      out_vec, IOVEC_LEN(out_vec));
    psa_close(handle);
#else
#error "psa_doorlock_answer_challenge not available without PSA_API"
#endif

    return status;
}
