/*
   Copyright (c) 2023 STMicroelectronics
   Copyright (c) 2019 Michal Duda
   Copyright (c) 2016 Tilen Majerle

   Permission is hereby granted, free of charge, to any person
   obtaining a copy of this software and associated documentation
   files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge,
   publish, distribute, sublicense, and/or sell copies of the Software,
   and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
   AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   OTHER DEALINGS IN THE SOFTWARE.
  ----------------------------------------------------------------------
*/

#pragma once

/* C++ detection */
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @defgroup SSD1306_Font Typedefs
 * @brief    Library Typedefs
 * @{
 */

/**
 * @brief  Font structure used on my LCD libraries
 */
typedef struct {
    uint8_t FontWidth;    /*!< Font width in pixels */
    uint8_t FontHeight;   /*!< Font height in pixels */
    const uint16_t *data; /*!< Pointer to data font data array */
} SSD1306_Font_t;

/**
 * @brief  String width and height in unit of pixels
 */
typedef struct {
    uint16_t Width;       /*!< String width in units of pixels */
    uint16_t Height;      /*!< String height in units of pixels */
} SSD1306_Font_Size_t;

/**
 * @}
 */

/**
 * @defgroup SSD1306_Font FontVariables
 * @brief    Library font variables
 * @{
 */

/**
 * @brief  7 x 10 pixels font size structure
 */
extern SSD1306_Font_t SSD1306_Font_7x10;

/**
 * @brief  11 x 18 pixels font size structure
 */
extern SSD1306_Font_t SSD1306_Font_11x18;

/**
 * @brief  16 x 26 pixels font size structure
 */
extern SSD1306_Font_t SSD1306_Font_16x26;

/**
 * @}
 */

/**
 * @defgroup SSD1306_Font Functions
 * @brief    Library functions
 * @{
 */

/**
 * @brief  Calculates string length and height in units of pixels depending on string and font used
 * @param  *str: String to be checked for length and height
 * @param  *SizeStruct: Pointer to empty @ref SSD1306_Font_Size_t structure where informations will be saved
 * @param  *Font: Pointer to @ref TM_FontDef_t font used for calculations
 * @retval None
 */
void SSD1306_Font_GetStringSize(const char* str, SSD1306_Font_Size_t* SizeStruct, const SSD1306_Font_t* Font);

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

/* C++ detection */
#ifdef __cplusplus
}
#endif
