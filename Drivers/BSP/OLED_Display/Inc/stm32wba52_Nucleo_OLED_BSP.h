/**
  ******************************************************************************
  * @file    stm32wba52_Nucleo_OLED_BSP.h
  * @author  MCD Application Team
  * @brief   Nucleo WBA OLED interface BSP header
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
/* USER CODE BEGIN Header */
/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN Define */
#define DISP_VSS_Pin GPIO_PIN_1
#define DISP_VSS_GPIO_Port GPIOA
#define DISP_VDD_Pin GPIO_PIN_2
#define DISP_VDD_GPIO_Port GPIOA

void MX_I2C3_Init(void);