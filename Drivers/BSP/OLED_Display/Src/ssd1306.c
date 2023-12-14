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

#include "ssd1306.h"

/* Private functions */
static void I2C_Write(uint8_t address, uint8_t reg, uint8_t data, uint32_t timeout);
static void I2C_WriteMulti(uint8_t address, uint8_t reg, uint8_t* data, uint16_t size, uint32_t timeout);

/* Write command */
#define SSD1306_WRITECOMMAND(command)      I2C_Write(SSD1306_I2C_ADDR_SHIFTED, 0x00, (command), SSD1306_I2C_WRITE_TIMEOUT)
/* Write data */
#define SSD1306_WRITEDATA(data)            I2C_Write(SSD1306_I2C_ADDR_SHIFTED, 0x40, (data), SSD1306_I2C_WRITE_TIMEOUT)
/* Absolute value */
#define ABS(x)                             ((x) > 0 ? (x) : -(x))

/* SSD1306 data buffer */
static uint8_t SSD1306_Buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];

/* Private SSD1306 structure */
typedef struct {
    uint16_t CurrentX;
    uint16_t CurrentY;
    uint8_t Inverted;
    uint8_t Initialized;
} SSD1306_t;

/* Private variable */
static I2C_HandleTypeDef *i2cHandle = NULL;
static SSD1306_t SSD1306 = { 0 };

bool SSD1306_Init(I2C_HandleTypeDef *handle) {
    /* Check if LCD connected to I2C */
    if (HAL_I2C_IsDeviceReady(handle, SSD1306_I2C_ADDR_SHIFTED, 2, 5) != HAL_OK) {
        return false;
    }

    /* Store i2c handle */
    i2cHandle = handle;

    /* A little delay */
    HAL_Delay(100);

    /* Init LCD */
    SSD1306_WRITECOMMAND(0xAE); // Set display off
    SSD1306_WRITECOMMAND(0xA8); // Set multiplex ratio
    SSD1306_WRITECOMMAND(0x1F); // -- from default 63 to 31 (i.e. 32MUX)
    SSD1306_WRITECOMMAND(0xD3); // Set display offset
    SSD1306_WRITECOMMAND(0x00); // -- no offset
    SSD1306_WRITECOMMAND(0x40); // Set display start line
    SSD1306_WRITECOMMAND(0xA1); // Set segment re-map, column address 127 is mapped to SEG0
    SSD1306_WRITECOMMAND(0xC8); // Set COM output scan direction - remapped mode
    SSD1306_WRITECOMMAND(0x81); // Set contrast control for BANK0
    SSD1306_WRITECOMMAND(0x7F); // -- range 0x00 to 0xFF => 50%
    SSD1306_WRITECOMMAND(0xA4); // Enable display outputs according to the GDDRAM contents.
    SSD1306_WRITECOMMAND(0xA6); // Set normal display
    SSD1306_WRITECOMMAND(0xD5); // Set display clock divide ration and oscillator frequency
    SSD1306_WRITECOMMAND(0x80); // -- frequency (1000 - default); display clock divide ratio (0000 - divide ration 1)
    SSD1306_WRITECOMMAND(0x8D); // Charge pump setting
    SSD1306_WRITECOMMAND(0x14); // -- enable charge pump

    SSD1306_WRITECOMMAND(0x2E); // Deactivate scroll
    SSD1306_WRITECOMMAND(0x20); // Set memory addressing mode
    SSD1306_WRITECOMMAND(0x10); // -- Page Addressing Mode (RESET)
    SSD1306_WRITECOMMAND(0xDA); // Set COM pins hardware configuration
    SSD1306_WRITECOMMAND(0x02); // --
    SSD1306_WRITECOMMAND(0xD9); // Set pre-charge period
    SSD1306_WRITECOMMAND(0x22); // --
    SSD1306_WRITECOMMAND(0xDB); // Set Vcomh deselect level
    SSD1306_WRITECOMMAND(0x20); // -- 0.77 x Vcc (RESET)

    SSD1306_WRITECOMMAND(0xB0); // Set page start address for page addressing mode
    SSD1306_WRITECOMMAND(0x00); // Set lower column start address for page addressing mode
    SSD1306_WRITECOMMAND(0x10); // Set higher column start address for page addressing mode

    SSD1306_WRITECOMMAND(0xAF); // Set display on

    /* Clear screen */
    SSD1306_Fill(SSD1306_COLOR_BLACK);

    /* Update screen */
    SSD1306_UpdateScreen();

    /* Set default values */
    SSD1306.CurrentX = 0;
    SSD1306.CurrentY = 0;

    /* Initialized OK */
    SSD1306.Initialized = 1;

    return true;
}

static void I2C_Write(uint8_t address, uint8_t reg, uint8_t data, uint32_t timeout) {
    uint8_t dataToSend[2] = { reg, data };
    HAL_I2C_Master_Transmit(i2cHandle, address, dataToSend, sizeof(dataToSend), timeout);
}

static void I2C_WriteMulti(uint8_t address, uint8_t reg, uint8_t* data, uint16_t size, uint32_t timeout) {
    HAL_I2C_Mem_Write(i2cHandle, address, reg, I2C_MEMADD_SIZE_8BIT, data, size, timeout);
}

void SSD1306_UpdateScreen(void) {
    for (uint8_t m = 0; m < 8; ++m) {
        SSD1306_WRITECOMMAND(0xB0 + m);
        SSD1306_WRITECOMMAND(0x00);
        SSD1306_WRITECOMMAND(0x10);

        /* Write multi data */
        I2C_WriteMulti(SSD1306_I2C_ADDR_SHIFTED, 0x40, &SSD1306_Buffer[SSD1306_WIDTH * m], SSD1306_WIDTH, SSD1306_I2C_WRITE_TIMEOUT);
    }
}

void SSD1306_ToggleInvert(void) {
    /* Toggle invert */
    SSD1306.Inverted = !SSD1306.Inverted;

    /* Do memory toggle */
    for (uint16_t i = 0; i < sizeof(SSD1306_Buffer); ++i) {
        SSD1306_Buffer[i] = ~SSD1306_Buffer[i];
    }
}

void SSD1306_Fill(SSD1306_COLOR_t color) {
    /* Set memory */
    memset(SSD1306_Buffer, (color == SSD1306_COLOR_BLACK) ? 0x00 : 0xFF, sizeof(SSD1306_Buffer));
}

void SSD1306_DrawPixel(uint16_t x, uint16_t y, SSD1306_COLOR_t color) {
    if (
        x >= SSD1306_WIDTH ||
        y >= SSD1306_HEIGHT
    ) {
        /* Error */
        return;
    }

    /* Check if pixels are inverted */
    if (SSD1306.Inverted) {
        color = (SSD1306_COLOR_t)!color;
    }

    /* Set color */
    if (color == SSD1306_COLOR_WHITE) {
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] |= 1 << (y % 8);
    } else {
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
    }
}

void SSD1306_GotoXY(uint16_t x, uint16_t y) {
    /* Set write pointers */
    SSD1306.CurrentX = x;
    SSD1306.CurrentY = y;
}

char SSD1306_Putc(char ch, SSD1306_Font_t* Font, SSD1306_COLOR_t color) {
    /* Check available space in LCD */
    if (
        SSD1306_WIDTH <= (SSD1306.CurrentX + Font->FontWidth) ||
        SSD1306_HEIGHT <= (SSD1306.CurrentY + Font->FontHeight)
    ) {
        /* Error */
        return 0;
    }

    /* Go through font */
    for (uint32_t i = 0; i < Font->FontHeight; ++i) {
        uint32_t b = Font->data[(ch - 32) * Font->FontHeight + i];
        for (uint32_t j = 0; j < Font->FontWidth; ++j) {
            if ((b << j) & 0x8000) {
                SSD1306_DrawPixel(SSD1306.CurrentX + j, (SSD1306.CurrentY + i), (SSD1306_COLOR_t) color);
            } else {
                SSD1306_DrawPixel(SSD1306.CurrentX + j, (SSD1306.CurrentY + i), (SSD1306_COLOR_t)!color);
            }
        }
    }

    /* Increase pointer */
    SSD1306.CurrentX += Font->FontWidth;

    /* Return character written */
    return ch;
}

char SSD1306_Puts(char* str, SSD1306_Font_t* Font, SSD1306_COLOR_t color) {
    /* Write characters */
    while (*str) {
        /* Write character by character */
        if (SSD1306_Putc(*str, Font, color) != *str) {
            /* Return error */
            return *str;
        }

        /* Increase string pointer */
        str++;
    }

    /* Everything OK, zero should be returned */
    return *str;
}


void SSD1306_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, SSD1306_COLOR_t c) {
    int16_t dx, dy, sx, sy, err, e2, tmp;

    /* Check for overflow */
    if (x0 >= SSD1306_WIDTH) {
        x0 = SSD1306_WIDTH - 1;
    }
    if (x1 >= SSD1306_WIDTH) {
        x1 = SSD1306_WIDTH - 1;
    }
    if (y0 >= SSD1306_HEIGHT) {
        y0 = SSD1306_HEIGHT - 1;
    }
    if (y1 >= SSD1306_HEIGHT) {
        y1 = SSD1306_HEIGHT - 1;
    }

    dx = (x0 < x1) ? (x1 - x0) : (x0 - x1);
    dy = (y0 < y1) ? (y1 - y0) : (y0 - y1);
    sx = (x0 < x1) ? 1 : -1;
    sy = (y0 < y1) ? 1 : -1;
    err = ((dx > dy) ? dx : -dy) / 2;

    if (dx == 0) {
        if (y1 < y0) {
            tmp = y1;
            y1 = y0;
            y0 = tmp;
        }

        if (x1 < x0) {
            tmp = x1;
            x1 = x0;
            x0 = tmp;
        }

        /* Vertical line */
        for (int16_t i = y0; i <= y1; ++i) {
            SSD1306_DrawPixel(x0, i, c);
        }

        /* Return from function */
        return;
    }

    if (dy == 0) {
        if (y1 < y0) {
            tmp = y1;
            y1 = y0;
            y0 = tmp;
        }

        if (x1 < x0) {
            tmp = x1;
            x1 = x0;
            x0 = tmp;
        }

        /* Horizontal line */
        for (int16_t i = x0; i <= x1; ++i) {
            SSD1306_DrawPixel(i, y0, c);
        }

        /* Return from function */
        return;
    }

    while (1) {
        SSD1306_DrawPixel(x0, y0, c);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        e2 = err;
        if (e2 > -dx) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dy) {
            err += dx;
            y0 += sy;
        }
    }
}

void SSD1306_DrawRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, SSD1306_COLOR_t c) {
    /* Check input parameters */
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) {
        /* Return error */
        return;
    }

    /* Check width and height */
    if ((x + w) >= SSD1306_WIDTH) {
        w = SSD1306_WIDTH - x;
    }
    if ((y + h) >= SSD1306_HEIGHT) {
        h = SSD1306_HEIGHT - y;
    }

    /* Draw 4 lines */
    SSD1306_DrawLine(x, y, x + w, y, c);         /* Top line */
    SSD1306_DrawLine(x, y + h, x + w, y + h, c); /* Bottom line */
    SSD1306_DrawLine(x, y, x, y + h, c);         /* Left line */
    SSD1306_DrawLine(x + w, y, x + w, y + h, c); /* Right line */
}

void SSD1306_DrawFilledRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, SSD1306_COLOR_t c) {
    /* Check input parameters */
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) {
        /* Return error */
        return;
    }

    /* Check width and height */
    if ((x + w) >= SSD1306_WIDTH) {
        w = SSD1306_WIDTH - x;
    }
    if ((y + h) >= SSD1306_HEIGHT) {
        h = SSD1306_HEIGHT - y;
    }

    /* Draw lines */
    for (uint8_t i = 0; i <= h; ++i) {
        /* Draw lines */
        SSD1306_DrawLine(x, y + i, x + w, y + i, c);
    }
}

void SSD1306_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, SSD1306_COLOR_t color) {
    /* Draw lines */
    SSD1306_DrawLine(x1, y1, x2, y2, color);
    SSD1306_DrawLine(x2, y2, x3, y3, color);
    SSD1306_DrawLine(x3, y3, x1, y1, color);
}


void SSD1306_DrawFilledTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, SSD1306_COLOR_t color) {
    int16_t deltax = 0, deltay = 0, x = 0, y = 0, xinc1 = 0, xinc2 = 0,
    yinc1 = 0, yinc2 = 0, den = 0, num = 0, numadd = 0, numpixels = 0;

    deltax = ABS(x2 - x1);
    deltay = ABS(y2 - y1);
    x = x1;
    y = y1;

    if (x2 >= x1) {
        xinc1 = 1;
        xinc2 = 1;
    } else {
        xinc1 = -1;
        xinc2 = -1;
    }

    if (y2 >= y1) {
        yinc1 = 1;
        yinc2 = 1;
    } else {
        yinc1 = -1;
        yinc2 = -1;
    }

    if (deltax >= deltay){
        xinc1 = 0;
        yinc2 = 0;
        den = deltax;
        num = deltax / 2;
        numadd = deltay;
        numpixels = deltax;
    } else {
        xinc2 = 0;
        yinc1 = 0;
        den = deltay;
        num = deltay / 2;
        numadd = deltax;
        numpixels = deltay;
    }

    for (int16_t curpixel = 0; curpixel <= numpixels; ++curpixel) {
        SSD1306_DrawLine(x, y, x3, y3, color);

        num += numadd;
        if (num >= den) {
            num -= den;
            x += xinc1;
            y += yinc1;
        }
        x += xinc2;
        y += yinc2;
    }
}

void SSD1306_DrawCircle(int16_t x0, int16_t y0, int16_t r, SSD1306_COLOR_t c) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    SSD1306_DrawPixel(x0, y0 + r, c);
    SSD1306_DrawPixel(x0, y0 - r, c);
    SSD1306_DrawPixel(x0 + r, y0, c);
    SSD1306_DrawPixel(x0 - r, y0, c);

    while (x < y) {
        if (f >= 0) {
            --y;
            ddF_y += 2;
            f += ddF_y;
        }
        ++x;
        ddF_x += 2;
        f += ddF_x;

        SSD1306_DrawPixel(x0 + x, y0 + y, c);
        SSD1306_DrawPixel(x0 - x, y0 + y, c);
        SSD1306_DrawPixel(x0 + x, y0 - y, c);
        SSD1306_DrawPixel(x0 - x, y0 - y, c);

        SSD1306_DrawPixel(x0 + y, y0 + x, c);
        SSD1306_DrawPixel(x0 - y, y0 + x, c);
        SSD1306_DrawPixel(x0 + y, y0 - x, c);
        SSD1306_DrawPixel(x0 - y, y0 - x, c);
    }
}

void SSD1306_DrawFilledCircle(int16_t x0, int16_t y0, int16_t r, SSD1306_COLOR_t c) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    SSD1306_DrawPixel(x0, y0 + r, c);
    SSD1306_DrawPixel(x0, y0 - r, c);
    SSD1306_DrawPixel(x0 + r, y0, c);
    SSD1306_DrawPixel(x0 - r, y0, c);
    SSD1306_DrawLine(x0 - r, y0, x0 + r, y0, c);

    while (x < y) {
        if (f >= 0) {
            --y;
            ddF_y += 2;
            f += ddF_y;
        }
        ++x;
        ddF_x += 2;
        f += ddF_x;

        SSD1306_DrawLine(x0 - x, y0 + y, x0 + x, y0 + y, c);
        SSD1306_DrawLine(x0 + x, y0 - y, x0 - x, y0 - y, c);

        SSD1306_DrawLine(x0 + y, y0 + x, x0 - y, y0 + x, c);
        SSD1306_DrawLine(x0 + y, y0 - x, x0 - y, y0 - x, c);
    }
}

void SSD1306_ON(void) {
    SSD1306_WRITECOMMAND(0x8D);
    SSD1306_WRITECOMMAND(0x14);
    SSD1306_WRITECOMMAND(0xAF);
}

void SSD1306_OFF(void) {
    SSD1306_WRITECOMMAND(0x8D);
    SSD1306_WRITECOMMAND(0x10);
    SSD1306_WRITECOMMAND(0xAE);
}
