/*
 * LCD_Driver.c
 *
 *  Created on: Apr 6, 2026
 *      Author: nguyenhoa
 */

#include "LCD_Driver.h"
#include "LCD_lib.h"
#include <stdlib.h>

extern SPI_HandleTypeDef hspi1;

#define delay_ms(x) HAL_Delay(x)

static void lcd_write_byte(uint8_t chByte, uint8_t chCmd);
static void lcd_write_word(uint16_t hwData);
static void lcd_write_command(uint8_t chRegister, uint8_t chValue);
static void lcd_ctrl_port_init(void);
static void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
static uint32_t _pow(uint8_t m, uint8_t n);

/******************************************************************************/
void spi1_init(void)
{
    /* SPI1 da duoc khoi tao trong MX_SPI1_Init() */
}

/******************************************************************************/
uint8_t spi1_communication(uint8_t send_char)
{
    uint8_t rx_data = 0;
    HAL_SPI_TransmitReceive(&hspi1, &send_char, &rx_data, 1, 100);
    return rx_data;
}

/******************************************************************************/
static void lcd_write_byte(uint8_t chByte, uint8_t chCmd)
{
    if (chCmd) {
        LCD_DC_H();
    } else {
        LCD_DC_L();
    }

    LCD_CS_L();
    spi1_communication(chByte);
    LCD_CS_H();
}

/******************************************************************************/
static void lcd_write_word(uint16_t hwData)
{
    LCD_DC_H();
    LCD_CS_L();
    spi1_communication((uint8_t)(hwData >> 8));
    spi1_communication((uint8_t)(hwData & 0xFF));
    LCD_CS_H();
}

/******************************************************************************/
static void lcd_write_command(uint8_t chRegister, uint8_t chValue)
{
    lcd_write_byte(chRegister, LCD_CMD);
    lcd_write_byte(chValue, LCD_DATA);
}

/******************************************************************************/
static void lcd_ctrl_port_init(void)
{
    LCD_RST_H();
    LCD_BKL_H();
    LCD_CS_H();
    LCD_DC_H();
}

/******************************************************************************/
static void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    if (x0 >= LCD_WIDTH || y0 >= LCD_HEIGHT || x1 >= LCD_WIDTH || y1 >= LCD_HEIGHT) {
        return;
    }

    lcd_write_byte(0x2A, LCD_CMD);
    lcd_write_byte((uint8_t)(x0 >> 8), LCD_DATA);
    lcd_write_byte((uint8_t)(x0 & 0xFF), LCD_DATA);
    lcd_write_byte((uint8_t)(x1 >> 8), LCD_DATA);
    lcd_write_byte((uint8_t)(x1 & 0xFF), LCD_DATA);

    lcd_write_byte(0x2B, LCD_CMD);
    lcd_write_byte((uint8_t)(y0 >> 8), LCD_DATA);
    lcd_write_byte((uint8_t)(y0 & 0xFF), LCD_DATA);
    lcd_write_byte((uint8_t)(y1 >> 8), LCD_DATA);
    lcd_write_byte((uint8_t)(y1 & 0xFF), LCD_DATA);

    lcd_write_byte(0x2C, LCD_CMD);
}

/******************************************************************************/
void lcd_init(void)
{
    lcd_ctrl_port_init();
    spi1_init();

    LCD_RST_H();
    delay_ms(5);
    LCD_RST_L();
    delay_ms(5);
    LCD_RST_H();
    delay_ms(20);

    LCD_CS_H();
    LCD_BKL_H();

#ifdef ST7789_DEVICE
    lcd_write_byte(0x11, LCD_CMD);
    delay_ms(120);

    lcd_write_command(0x36, 0x00);
    lcd_write_command(0x3A, 0x05);

    lcd_write_byte(0xB2, LCD_CMD);
    lcd_write_byte(0x0C, LCD_DATA);
    lcd_write_byte(0x0C, LCD_DATA);
    lcd_write_byte(0x00, LCD_DATA);
    lcd_write_byte(0x33, LCD_DATA);
    lcd_write_byte(0x33, LCD_DATA);

    lcd_write_command(0xB7, 0x35);
    lcd_write_command(0xBB, 0x28);
    lcd_write_command(0xC0, 0x3C);
    lcd_write_command(0xC2, 0x01);
    lcd_write_command(0xC3, 0x0B);
    lcd_write_command(0xC4, 0x20);
    lcd_write_command(0xC6, 0x0F);

    lcd_write_byte(0xD0, LCD_CMD);
    lcd_write_byte(0xA4, LCD_DATA);
    lcd_write_byte(0xA1, LCD_DATA);

    lcd_write_byte(0xE0, LCD_CMD);
    lcd_write_byte(0xD0, LCD_DATA);
    lcd_write_byte(0x01, LCD_DATA);
    lcd_write_byte(0x08, LCD_DATA);
    lcd_write_byte(0x0F, LCD_DATA);
    lcd_write_byte(0x11, LCD_DATA);
    lcd_write_byte(0x2A, LCD_DATA);
    lcd_write_byte(0x36, LCD_DATA);
    lcd_write_byte(0x55, LCD_DATA);
    lcd_write_byte(0x44, LCD_DATA);
    lcd_write_byte(0x3A, LCD_DATA);
    lcd_write_byte(0x0B, LCD_DATA);
    lcd_write_byte(0x06, LCD_DATA);
    lcd_write_byte(0x11, LCD_DATA);
    lcd_write_byte(0x20, LCD_DATA);

    lcd_write_byte(0xE1, LCD_CMD);
    lcd_write_byte(0xD0, LCD_DATA);
    lcd_write_byte(0x02, LCD_DATA);
    lcd_write_byte(0x07, LCD_DATA);
    lcd_write_byte(0x0A, LCD_DATA);
    lcd_write_byte(0x0B, LCD_DATA);
    lcd_write_byte(0x18, LCD_DATA);
    lcd_write_byte(0x34, LCD_DATA);
    lcd_write_byte(0x43, LCD_DATA);
    lcd_write_byte(0x4A, LCD_DATA);
    lcd_write_byte(0x2B, LCD_DATA);
    lcd_write_byte(0x1B, LCD_DATA);
    lcd_write_byte(0x1C, LCD_DATA);
    lcd_write_byte(0x22, LCD_DATA);
    lcd_write_byte(0x1F, LCD_DATA);

    lcd_write_byte(0x29, LCD_CMD);
    lcd_write_command(0x51, 0xFF);
    lcd_write_command(0x55, 0xB0);
#endif

    lcd_clear_screen(WHITE);
}

/******************************************************************************/
void lcd_clear_screen(uint16_t hwColor)
{
    uint32_t i;
    uint32_t wCount = (uint32_t)LCD_WIDTH * (uint32_t)LCD_HEIGHT;

    lcd_set_window(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);

    LCD_CS_L();
    LCD_DC_H();
    for (i = 0; i < wCount; i++) {
        spi1_communication((uint8_t)(hwColor >> 8));
        spi1_communication((uint8_t)(hwColor & 0xFF));
    }
    LCD_CS_H();
}

/******************************************************************************/
void lcd_set_cursor(uint16_t hwXpos, uint16_t hwYpos)
{
    if (hwXpos >= LCD_WIDTH || hwYpos >= LCD_HEIGHT) {
        return;
    }
    lcd_set_window(hwXpos, hwYpos, hwXpos, hwYpos);
}

/******************************************************************************/
void lcd_display_char(uint16_t hwXpos,
                      uint16_t hwYpos,
                      uint8_t chChr,
                      uint8_t chSize,
                      uint16_t hwColor)
{
    uint8_t i, j, chTemp;
    uint16_t hwYpos0 = hwYpos;

    if (hwXpos >= LCD_WIDTH || hwYpos >= LCD_HEIGHT) {
        return;
    }

    if (chChr < 0x20 || chChr > 0x7E) {
        return;
    }

    for (i = 0; i < chSize; i++) {
        if (FONT_1206 == chSize) {
            chTemp = c_chFont1206[chChr - 0x20][i];
        } else if (FONT_1608 == chSize) {
            chTemp = c_chFont1608[chChr - 0x20][i];
        } else {
            return;
        }

        for (j = 0; j < 8; j++) {
            if (chTemp & 0x80) {
                lcd_draw_dot(hwXpos, hwYpos, hwColor);
            }
            chTemp <<= 1;
            hwYpos++;

            if ((hwYpos - hwYpos0) == chSize) {
                hwYpos = hwYpos0;
                hwXpos++;
                break;
            }
        }
    }
}

/******************************************************************************/
void lcd_display_string(uint16_t hwXpos,
                        uint16_t hwYpos,
                        const uint8_t *pchString,
                        uint8_t chSize,
                        uint16_t hwColor)
{
    if (hwXpos >= LCD_WIDTH || hwYpos >= LCD_HEIGHT) {
        return;
    }

    while (*pchString != '\0') {
        if (hwXpos > (LCD_WIDTH - chSize / 2)) {
            hwXpos = 0;
            hwYpos += chSize;
            if (hwYpos > (LCD_HEIGHT - chSize)) {
                hwYpos = 0;
                hwXpos = 0;
                lcd_clear_screen(BLACK);
            }
        }

        lcd_display_char(hwXpos, hwYpos, *pchString, chSize, hwColor);
        hwXpos += chSize / 2;
        pchString++;
    }
}

/******************************************************************************/
void lcd_display_GB2312(uint8_t gb, uint16_t color_front,
                        uint16_t postion_x, uint16_t postion_y)
{
    uint8_t i, j, chTemp;
    uint16_t hwYpos0 = postion_y;

    if (postion_x >= LCD_WIDTH || postion_y >= LCD_HEIGHT) {
        return;
    }

    for (i = 0; i < 32; i++) {
        chTemp = GB2312[gb][i];
        for (j = 0; j < 8; j++) {
            if (chTemp & 0x80) {
                if (i < 15) {
                    lcd_draw_dot(postion_x, postion_y, color_front);
                } else {
                    lcd_draw_dot(postion_x - 16, postion_y + 8, color_front);
                }
            }
            chTemp <<= 1;
            postion_y++;
            if ((postion_y - hwYpos0) == 8) {
                postion_y = hwYpos0;
                postion_x++;
                break;
            }
        }
    }
}

/******************************************************************************/
void lcd_draw_dot(uint16_t hwXpos, uint16_t hwYpos, uint16_t hwColor)
{
    if (hwXpos >= LCD_WIDTH || hwYpos >= LCD_HEIGHT) {
        return;
    }

    lcd_set_cursor(hwXpos, hwYpos);
    lcd_write_word(hwColor);
}

/******************************************************************************/
void lcd_draw_bigdot(uint32_t color_front, uint32_t x, uint32_t y)
{
    lcd_draw_dot((uint16_t)x,     (uint16_t)y,     (uint16_t)color_front);
    lcd_draw_dot((uint16_t)x,     (uint16_t)(y+1), (uint16_t)color_front);
    lcd_draw_dot((uint16_t)x,     (uint16_t)(y-1), (uint16_t)color_front);

    lcd_draw_dot((uint16_t)(x+1), (uint16_t)y,     (uint16_t)color_front);
    lcd_draw_dot((uint16_t)(x+1), (uint16_t)(y+1), (uint16_t)color_front);
    lcd_draw_dot((uint16_t)(x+1), (uint16_t)(y-1), (uint16_t)color_front);

    lcd_draw_dot((uint16_t)(x-1), (uint16_t)y,     (uint16_t)color_front);
    lcd_draw_dot((uint16_t)(x-1), (uint16_t)(y+1), (uint16_t)color_front);
    lcd_draw_dot((uint16_t)(x-1), (uint16_t)(y-1), (uint16_t)color_front);
}

/******************************************************************************/
static uint32_t _pow(uint8_t m, uint8_t n)
{
    uint32_t result = 1;
    while (n--) {
        result *= m;
    }
    return result;
}

/******************************************************************************/
void lcd_display_num(uint16_t hwXpos, uint16_t hwYpos,
                     uint32_t chNum, uint8_t chLen,
                     uint8_t chSize, uint16_t hwColor)
{
    uint8_t i;
    uint8_t chTemp, chShow = 0;

    if (hwXpos >= LCD_WIDTH || hwYpos >= LCD_HEIGHT) {
        return;
    }

    for (i = 0; i < chLen; i++) {
        chTemp = (chNum / _pow(10, chLen - i - 1)) % 10;
        if (chShow == 0 && i < (chLen - 1)) {
            if (chTemp == 0) {
                lcd_display_char(hwXpos + (chSize / 2) * i, hwYpos, ' ', chSize, hwColor);
                continue;
            } else {
                chShow = 1;
            }
        }
        lcd_display_char(hwXpos + (chSize / 2) * i, hwYpos, chTemp + '0', chSize, hwColor);
    }
}

/******************************************************************************/
void lcd_draw_line(uint16_t hwXpos0, uint16_t hwYpos0,
                   uint16_t hwXpos1, uint16_t hwYpos1,
                   uint16_t hwColor)
{
    int x = hwXpos1 - hwXpos0;
    int y = hwYpos1 - hwYpos0;
    int dx = abs(x), sx = hwXpos0 < hwXpos1 ? 1 : -1;
    int dy = -abs(y), sy = hwYpos0 < hwYpos1 ? 1 : -1;
    int err = dx + dy, e2;

    if (hwXpos0 >= LCD_WIDTH || hwYpos0 >= LCD_HEIGHT ||
        hwXpos1 >= LCD_WIDTH || hwYpos1 >= LCD_HEIGHT) {
        return;
    }

    for (;;) {
        lcd_draw_dot(hwXpos0, hwYpos0, hwColor);
        e2 = 2 * err;
        if (e2 >= dy) {
            if (hwXpos0 == hwXpos1) {
                break;
            }
            err += dy;
            hwXpos0 += sx;
        }
        if (e2 <= dx) {
            if (hwYpos0 == hwYpos1) {
                break;
            }
            err += dx;
            hwYpos0 += sy;
        }
    }
}

/******************************************************************************/
void lcd_draw_circle(uint16_t hwXpos, uint16_t hwYpos,
                     uint16_t hwRadius, uint16_t hwColor)
{
    int x = -hwRadius, y = 0, err = 2 - 2 * hwRadius, e2;

    if (hwXpos >= LCD_WIDTH || hwYpos >= LCD_HEIGHT) {
        return;
    }

    do {
        lcd_draw_dot(hwXpos - x, hwYpos + y, hwColor);
        lcd_draw_dot(hwXpos + x, hwYpos + y, hwColor);
        lcd_draw_dot(hwXpos + x, hwYpos - y, hwColor);
        lcd_draw_dot(hwXpos - x, hwYpos - y, hwColor);
        e2 = err;
        if (e2 <= y) {
            err += ++y * 2 + 1;
            if (-x == y && e2 <= x) {
                e2 = 0;
            }
        }
        if (e2 > x) {
            err += ++x * 2 + 1;
        }
    } while (x <= 0);
}

/******************************************************************************/
void lcd_fill_rect(uint16_t hwXpos, uint16_t hwYpos,
                   uint16_t hwWidth, uint16_t hwHeight,
                   uint16_t hwColor)
{
    uint16_t i, j;

    if (hwXpos >= LCD_WIDTH || hwYpos >= LCD_HEIGHT) {
        return;
    }

    for (i = 0; i < hwHeight; i++) {
        for (j = 0; j < hwWidth; j++) {
            lcd_draw_dot(hwXpos + j, hwYpos + i, hwColor);
        }
    }
}

/******************************************************************************/
void lcd_draw_v_line(uint16_t hwXpos, uint16_t hwYpos,
                     uint16_t hwHeight, uint16_t hwColor)
{
    uint16_t i, y1 = MIN(hwYpos + hwHeight, LCD_HEIGHT - 1);

    if (hwXpos >= LCD_WIDTH || hwYpos >= LCD_HEIGHT) {
        return;
    }

    for (i = hwYpos; i < y1; i++) {
        lcd_draw_dot(hwXpos, i, hwColor);
    }
}

/******************************************************************************/
void lcd_draw_h_line(uint16_t hwXpos, uint16_t hwYpos,
                     uint16_t hwWidth, uint16_t hwColor)
{
    uint16_t i, x1 = MIN(hwXpos + hwWidth, LCD_WIDTH - 1);

    if (hwXpos >= LCD_WIDTH || hwYpos >= LCD_HEIGHT) {
        return;
    }

    for (i = hwXpos; i < x1; i++) {
        lcd_draw_dot(i, hwYpos, hwColor);
    }
}

/******************************************************************************/
void lcd_draw_rect(uint16_t hwXpos, uint16_t hwYpos,
                   uint16_t hwWidth, uint16_t hwHeight,
                   uint16_t hwColor)
{
    if (hwXpos >= LCD_WIDTH || hwYpos >= LCD_HEIGHT) {
        return;
    }

    lcd_draw_h_line(hwXpos, hwYpos, hwWidth, hwColor);
    lcd_draw_h_line(hwXpos, hwYpos + hwHeight, hwWidth, hwColor);
    lcd_draw_v_line(hwXpos, hwYpos, hwHeight, hwColor);
    lcd_draw_v_line(hwXpos + hwWidth, hwYpos, hwHeight + 1, hwColor);
}

/******************************************************************************/
void lcd_clear_Rect(uint32_t color_front,
                    uint32_t hwXpos, uint32_t hwYpos,
                    uint32_t hwXpos1, uint32_t hwYpos1)
{
    uint16_t i, j;

    if (hwXpos1 >= LCD_WIDTH || hwYpos1 >= LCD_HEIGHT) {
        return;
    }

    for (i = 0; i < hwYpos1 - hwYpos + 1; i++) {
        for (j = 0; j < hwXpos1 - hwXpos + 1; j++) {
            lcd_draw_dot((uint16_t)(hwXpos + j), (uint16_t)(hwYpos + i), (uint16_t)color_front);
        }
    }
}
