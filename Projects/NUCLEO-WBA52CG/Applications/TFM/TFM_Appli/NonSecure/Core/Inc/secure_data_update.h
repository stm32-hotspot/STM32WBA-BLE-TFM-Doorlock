/**
  ******************************************************************************
  * @file    secure_data_update.h
  * @author  MCD Application Team
  * @brief   Header for secure data update
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
#ifndef _SECURE_DATA_IMAGE_H_
#define _SECURE_DATA_IMAGE_H_

#include <stdint.h>

int secure_data_update_init();

int secure_data_update_write(uint8_t *buf, uint32_t offset, uint32_t buf_sz);

int secure_data_udate_install();

#endif /*_SECURE_DATA_IMAGE_H_*/