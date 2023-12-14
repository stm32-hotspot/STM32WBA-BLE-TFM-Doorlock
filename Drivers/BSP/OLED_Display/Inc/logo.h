/**
  ******************************************************************************
  * @file    logo.h
  * @author  MCD Application Team
  * @brief   Different LCD logo header
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
/* USER CODE BEGIN */
#ifndef LOGO_H
#define LOGO_H

/* C++ detection */
#ifdef __cplusplus
extern C {
#endif

#define BLUETOOTH_LOGO_WIDTH  22
#define BLUETOOTH_LOGO_HEIGHT 32

typedef struct {
  uint8_t height;
  uint8_t length;
  uint8_t * ptr;
}logo_t;  

extern const long int bluetooth_logo[];
extern logo_t camera_logo;
extern logo_t microphone_logo;
extern logo_t small_heart_logo;
extern logo_t big_heart_logo;
/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif
/* USER CODE END */
