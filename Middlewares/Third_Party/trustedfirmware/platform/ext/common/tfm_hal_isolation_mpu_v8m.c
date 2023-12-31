/*
 * Copyright (c) 2020, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <arm_cmse.h>
#include <stddef.h>
#include <stdint.h>
#include "cmsis.h"
#include "tfm_hal_defs.h"
#include "tfm_hal_isolation.h"

#ifdef TFM_FIH_PROFILE_ON
fih_int tfm_hal_memory_has_access(uintptr_t base,
                                  size_t size,
                                  uint32_t attr)
#else
enum tfm_hal_status_t tfm_hal_memory_has_access(uintptr_t base,
                                                size_t size,
                                                uint32_t attr)
#endif
{
    int flags = 0;

    if (attr & TFM_HAL_ACCESS_NS) {
        CONTROL_Type ctrl;
        ctrl.w = __TZ_get_CONTROL_NS();
        if (ctrl.b.nPRIV == 1) {
            attr |= TFM_HAL_ACCESS_UNPRIVILEGED;
        } else {
            attr &= ~TFM_HAL_ACCESS_UNPRIVILEGED;
        }
        flags |= CMSE_NONSECURE;
    }

    if (attr & TFM_HAL_ACCESS_UNPRIVILEGED) {
        flags |= CMSE_MPU_UNPRIV;
    }

    if ((attr & TFM_HAL_ACCESS_READABLE) && (attr & TFM_HAL_ACCESS_WRITABLE)) {
        flags |= CMSE_MPU_READWRITE;
    } else if (attr & TFM_HAL_ACCESS_READABLE) {
        flags |= CMSE_MPU_READ;
    } else {
#ifdef TFM_FIH_PROFILE_ON
        FIH_RET(fih_int_encode(TFM_HAL_ERROR_INVALID_INPUT));
#else
        return TFM_HAL_ERROR_INVALID_INPUT;
#endif
    }

    if (cmse_check_address_range((void *)base, size, flags) != NULL) {
#ifdef TFM_FIH_PROFILE_ON
        FIH_RET(fih_int_encode(TFM_HAL_SUCCESS));
#else
        return TFM_HAL_SUCCESS;
#endif
    } else {
#ifdef TFM_FIH_PROFILE_ON
	FIH_RET(fih_int_encode(TFM_HAL_ERROR_MEM_FAULT));
#else
	return TFM_HAL_ERROR_MEM_FAULT;
#endif
    }
}
