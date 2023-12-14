/**
  ******************************************************************************
  * @file    hal_lcd.h
  * @author  MCD Application Team
  * @brief   LCD abstraction header
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
#ifndef __HAL_LCD_H
#define __HAL_LCD_H

#include "ssd1306.h"
#include "logo.h"

extern char LCD_row2[20];

typedef enum
{
    LCD_CHAR_SMALL,
    LCD_CHAR_MEDIUM,
    LCD_CHAR_BIG,
} LCD_CharSize_t;

void LCD_Init(void);
void LCD_PrintTest(uint16_t x,uint16_t y, char * msg, SSD1306_COLOR_t color, LCD_CharSize_t charSize);
void LCD_Print1stLine(char * msg, uint8_t update_screen);
void LCD_Print2ndLine(char * msg);
void LCD_Print(char* line1, char* line2);
void LCD_THREAD_PrintPanId(uint16_t panId);
void LCD_THREAD_PrintRole(char * role);
void LCD_THREAD_PrintRLOC(uint16_t rloc);
void LCD_PrintLabel(char * rloc);
void LCD_PrintError(uint32_t errId);
void LCD_BLE_PrintLocalName(char * name);
void LCD_BLE_PrintStatus(char * status);
void LCD_BLE_PrintLogo(void);
void LCD_BLE_ANCS_PrintData(char* line1, char* line2);
void LCD_BLE_NumberOfLink(char* line1);
void LCD_BLE_ClearStatus(void);
void LCD_BLE_HRS_PrintData(char* line1, char* line2);
void LCD_BLE_HTS_PrintTemperature(uint8_t temperature);
void LCD_BLE_TPS_PrintRSSI(uint8_t RSSI);
void LCD_ClearScreen_except_BLE_logo(void);
void LCD_ClearScreen(void);
void LCD_PrintLogo(logo_t *logo,int pos, uint8_t update_screen);
void LCD_Symmetry_X_Y(void);
#endif /* __HAL_LCD_H */
/* USER CODE END */
