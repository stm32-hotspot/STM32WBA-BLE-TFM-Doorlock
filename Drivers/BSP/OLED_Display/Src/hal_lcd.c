/**
  ******************************************************************************
  * @file    hal_lcd.c
  * @author  MCD Application Team
  * @brief   LCD abstraction source
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
#include "hal_lcd.h"
#include "ssd1306_fonts.h"
#include "stm32wba52_Nucleo_OLED_BSP.h"
#include "logo.h"
#include <stdio.h>
#include <string.h>

extern I2C_HandleTypeDef hi2c3;

static char tempLcdBuffer[32];

void LCD_Init(void)
{
    MX_I2C3_Init();
    HAL_Delay(50);
    HAL_GPIO_WritePin(DISP_VDD_GPIO_Port, DISP_VDD_Pin, GPIO_PIN_SET);
    /* A little delay for the SSD1306 power-up */
    HAL_Delay(50);

    SSD1306_Init(&hi2c3);     
}

void LCD_PrintTest(uint16_t x,uint16_t y, char * msg, SSD1306_COLOR_t color, LCD_CharSize_t charSize)
{
    SSD1306_Font_t Font;

    /* Clear screen first */
    SSD1306_Fill(SSD1306_COLOR_BLACK);

    SSD1306_GotoXY(x,y);

    if(charSize == LCD_CHAR_SMALL){
        Font = SSD1306_Font_7x10;
    }else if(charSize == LCD_CHAR_MEDIUM)
    {
        Font = SSD1306_Font_11x18;
    }else if(charSize == LCD_CHAR_BIG)
    {
        Font = SSD1306_Font_16x26;
    }else
    {
        /* put Medium size by default */
        Font = SSD1306_Font_11x18;
    }

    SSD1306_Puts("1", &Font, color);
    SSD1306_UpdateScreen();

    SSD1306_GotoXY(x,y+14);
    SSD1306_Puts("2", &Font, color);
    SSD1306_UpdateScreen();
}

/* Only 11 characters per line with font 11x18 can be displayed */
void LCD_Print1stLine(char * msg, uint8_t update_screen)
{
    SSD1306_Font_t Font;

    /* Clear screen first */
    SSD1306_Fill(SSD1306_COLOR_BLACK);

    SSD1306_GotoXY(0,0);
    /* put Medium size by default */
    Font = SSD1306_Font_11x18;

    SSD1306_Puts(msg, &Font, SSD1306_COLOR_WHITE);
    
    if(update_screen){
      SSD1306_UpdateScreen();
    }
    
}

void LCD_Print2ndLine(char * msg)
{
    SSD1306_Font_t Font;

    /* Clear screen first */
    //SSD1306_Fill(SSD1306_COLOR_BLACK);

    SSD1306_GotoXY(0,14);
    /* put Medium size by default */
    Font = SSD1306_Font_11x18;

    SSD1306_Puts(msg, &Font, SSD1306_COLOR_WHITE);
    SSD1306_UpdateScreen();
}

/* Only 11 characters per line with Font_11x18 can be displayed */
/* Only X characters per line with Font_7x10 can be displayed */
void LCD_Print(char* line1, char* line2){
    SSD1306_Font_t Font;

    /* Clear screen first */
    SSD1306_Fill(SSD1306_COLOR_BLACK);

    SSD1306_GotoXY(0,0);
    /* put Medium size by default */
    Font = SSD1306_Font_11x18;
    SSD1306_Puts(line1, &Font, SSD1306_COLOR_WHITE);

    if(line2 != NULL)
    {
        Font = SSD1306_Font_7x10;
        SSD1306_GotoXY(0,22);
        SSD1306_Puts("-> ", &Font, SSD1306_COLOR_WHITE);
        SSD1306_GotoXY(20,22);
        SSD1306_Puts(line2, &Font, SSD1306_COLOR_WHITE);
    }

    SSD1306_UpdateScreen();
}

void LCD_PrintLabel(char * label)
{
  SSD1306_DrawFilledRectangle(0,0,80,19,SSD1306_COLOR_BLACK);
  SSD1306_GotoXY(0,0);
  SSD1306_Puts(label, &SSD1306_Font_11x18, SSD1306_COLOR_WHITE); 
  SSD1306_UpdateScreen();
}

void LCD_THREAD_PrintRLOC(uint16_t rloc)
{
  sprintf(tempLcdBuffer, "0x%04X", rloc);
  SSD1306_DrawFilledRectangle(80,20,52,12,SSD1306_COLOR_BLACK);
  SSD1306_GotoXY(82,22);
  SSD1306_Puts(tempLcdBuffer, &SSD1306_Font_7x10, SSD1306_COLOR_WHITE); 
  SSD1306_UpdateScreen();
}

void LCD_THREAD_PrintPanId(uint16_t panId)
{
  sprintf(tempLcdBuffer, "0x%04X", panId);
  SSD1306_DrawFilledRectangle(80,0,52,20,SSD1306_COLOR_BLACK);
  SSD1306_GotoXY(82,7);
  SSD1306_Puts(tempLcdBuffer, &SSD1306_Font_7x10, SSD1306_COLOR_WHITE); 
  SSD1306_UpdateScreen();
}

void LCD_THREAD_PrintRole(char * role)
{
  SSD1306_DrawFilledRectangle(0,20,80,12,SSD1306_COLOR_BLACK);
  SSD1306_DrawFilledRectangle(0,20,5 + (strlen(role) * 7),12,SSD1306_COLOR_WHITE);
  SSD1306_GotoXY(3,22);
  SSD1306_Puts(role, &SSD1306_Font_7x10, SSD1306_COLOR_BLACK); 
  SSD1306_UpdateScreen();
}

void LCD_BLE_PrintLocalName(char * name)
{
  SSD1306_DrawFilledRectangle(31,0,80,19,SSD1306_COLOR_BLACK);
  SSD1306_GotoXY(31,0);
  SSD1306_Puts(name + 1, &SSD1306_Font_11x18, SSD1306_COLOR_WHITE);
  SSD1306_UpdateScreen();
}

void LCD_BLE_PrintLogo(void)
{
  uint16_t x, y;
  
  for (y = 0; y < BLUETOOTH_LOGO_HEIGHT; y++)
  {
    for (x = 0; x < (BLUETOOTH_LOGO_WIDTH); x++)
    {
      SSD1306_DrawPixel(22 - x, y, (SSD1306_COLOR_t)((bluetooth_logo[y] >> (x + 2)) & 0x00000001));
    }
  }
  SSD1306_UpdateScreen();
}

void LCD_PrintLogo(logo_t *logo,int pos, uint8_t update_screen){
  uint8_t x,y,i;
  uint8_t data_uint8 = 0; 
  SSD1306_COLOR_t data_bit;
  uint8_t height = logo->height;
  uint8_t length = logo->length;
  for (y = 0; y < height; y++)
  {
    for (x = 0; x < length ; x++)
    {
      data_uint8 = logo->ptr[ x + (y * length)];
           
      for (i = 0; i < 8 ; i++)
      {        
        if((data_uint8 &((uint8_t)0x80 >> i)) == ((uint8_t)0x80 >> i)){
          data_bit = SSD1306_COLOR_WHITE;
        }else{
          data_bit = SSD1306_COLOR_BLACK;
        }
        SSD1306_DrawPixel(pos+x*8+i, y, data_bit);
      }
    }
  }      

  if(update_screen){
    SSD1306_UpdateScreen();
  }
}

/* Clean Screen without removing bluetooth logo */
void LCD_ClearScreen_except_BLE_logo(void)
{
    SSD1306_DrawFilledRectangle(26,0,122,32,SSD1306_COLOR_BLACK);
    SSD1306_UpdateScreen();
}     

void LCD_ClearScreen(void)
{
  /* Clear screen */
  SSD1306_Fill(SSD1306_COLOR_BLACK);
  /* Update screen */
  SSD1306_UpdateScreen();
}

void LCD_BLE_PrintStatus(char * status)
{
  SSD1306_DrawFilledRectangle(31,20,100,12,SSD1306_COLOR_BLACK);
  SSD1306_DrawFilledRectangle(31,20,5 + (strlen(status) * 7),12,SSD1306_COLOR_WHITE);
  SSD1306_GotoXY(34,22);
  SSD1306_Puts(status, &SSD1306_Font_7x10, SSD1306_COLOR_BLACK); 
  SSD1306_UpdateScreen();
}
  
void LCD_BLE_ClearStatus(void)
{
  SSD1306_DrawFilledRectangle(31,20,100,12,SSD1306_COLOR_BLACK);
  SSD1306_UpdateScreen();
}
  
void LCD_BLE_HRS_PrintData(char* line1, char* line2)
{
    SSD1306_Font_t Font;
    Font = SSD1306_Font_7x10;

    SSD1306_GotoXY(40,0);
    /* Display HeartRate Measurement Value in BPM on line 1 */
    SSD1306_Puts(line1, &Font, SSD1306_COLOR_WHITE);
    SSD1306_GotoXY(40,14);
    /* Display HeartRate Energy Expended Value in kJ on line 2 */
    SSD1306_Puts(line2, &Font, SSD1306_COLOR_WHITE);
    SSD1306_UpdateScreen();
}

void LCD_BLE_ANCS_PrintData(char* line1, char* line2)
{
    SSD1306_Font_t Font;
    Font = SSD1306_Font_7x10;

    SSD1306_GotoXY(0,0);
    /* Display HeartRate Measurement Value in BPM on line 1 */
    SSD1306_Puts(line1, &Font, SSD1306_COLOR_WHITE);
    
    if(line2 != NULL)
    {
      SSD1306_GotoXY(0,14);
      /* Display HeartRate Energy Expended Value in kJ on line 2 */
      SSD1306_Puts(line2, &Font, SSD1306_COLOR_WHITE);
    }
    SSD1306_UpdateScreen();
}

void LCD_BLE_NumberOfLink(char* line1)
{
  SSD1306_GotoXY(34,0);
  SSD1306_Puts(line1, &SSD1306_Font_7x10, SSD1306_COLOR_WHITE); 
  SSD1306_UpdateScreen(); 
}

void LCD_BLE_HTS_PrintTemperature(uint8_t temperature)
{
  //sprintf(tempLcdBuffer, "#%02d", errId);
  SSD1306_DrawFilledRectangle(0,0,128,32,SSD1306_COLOR_WHITE);
  SSD1306_GotoXY(7,8);
  SSD1306_Puts("ERROR", &SSD1306_Font_11x18, SSD1306_COLOR_BLACK); 
  SSD1306_GotoXY(66,9);
  SSD1306_Puts(tempLcdBuffer, &SSD1306_Font_7x10, SSD1306_COLOR_BLACK); 
  SSD1306_UpdateScreen();
}

void LCD_BLE_TPS_PrintRSSI(uint8_t RSSI)
{
  //sprintf(tempLcdBuffer, "#%02d", errId);
  SSD1306_DrawFilledRectangle(0,0,128,32,SSD1306_COLOR_WHITE);
  SSD1306_GotoXY(7,8);
  SSD1306_Puts("ERROR", &SSD1306_Font_11x18, SSD1306_COLOR_BLACK); 
  SSD1306_GotoXY(66,9);
  SSD1306_Puts(tempLcdBuffer, &SSD1306_Font_7x10, SSD1306_COLOR_BLACK); 
  SSD1306_UpdateScreen();
}

/* USER CODE END */
