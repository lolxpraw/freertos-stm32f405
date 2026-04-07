/*
 * LCD_Driver.h
 *
 *  Created on: Apr 6, 2026
 *      Author: nguyenhoa
 */

#ifndef __LCD_H__
#define __LCD_H__

#include "main.h"
#include "stm32f4xx_hal.h"

#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))

#define LCD_WIDTH    240
#define LCD_HEIGHT   320

#define FONT_1206    12
#define FONT_1608    16
#define FONT_GB2312  16

#define WHITE    0xFFFF
#define BLACK    0x0000
#define BLUE     0x001F
#define BRED     0xF81F
#define GRED     0xFFE0
#define GBLUE    0x07FF
#define RED      0xF800
#define MAGENTA  0xF81F
#define GREEN    0x07E0
#define CYAN     0x7FFF
#define YELLOW   0xFFE0
#define BROWN    0xBC40
#define BRRED    0xFC07
#define GRAY     0x8430

#define LCD_DC_H()   HAL_GPIO_WritePin(LCD_DC_GPIO_Port,  LCD_DC_Pin,  GPIO_PIN_SET)
#define LCD_DC_L()   HAL_GPIO_WritePin(LCD_DC_GPIO_Port,  LCD_DC_Pin,  GPIO_PIN_RESET)

#define LCD_CS_H()   HAL_GPIO_WritePin(LCD_CS_GPIO_Port,  LCD_CS_Pin,  GPIO_PIN_SET)
#define LCD_CS_L()   HAL_GPIO_WritePin(LCD_CS_GPIO_Port,  LCD_CS_Pin,  GPIO_PIN_RESET)

#define LCD_BKL_H()  HAL_GPIO_WritePin(LCD_BL_GPIO_Port,  LCD_BL_Pin,  GPIO_PIN_SET)
#define LCD_BKL_L()  HAL_GPIO_WritePin(LCD_BL_GPIO_Port,  LCD_BL_Pin,  GPIO_PIN_RESET)

#define LCD_RST_H()  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET)
#define LCD_RST_L()  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET)

#define LCD_CMD   0
#define LCD_DATA  1

#define ST7789_DEVICE
/* #define HX8347D_DEVICE */

void spi1_init(void);
uint8_t spi1_communication(uint8_t send_char);
void lcd_init(void);
void lcd_set_cursor(uint16_t hwXpos, uint16_t hwYpos);
void lcd_display_GB2312(uint8_t gb, uint16_t color_front, uint16_t postion_x, uint16_t postion_y);
void lcd_draw_dot(uint16_t hwXpos, uint16_t hwYpos, uint16_t hwColor);
void lcd_draw_bigdot(uint32_t color_front, uint32_t x, uint32_t y);
void lcd_clear_screen(uint16_t hwColor);
void lcd_display_char(uint16_t hwXpos, uint16_t hwYpos, uint8_t chChr, uint8_t chSize, uint16_t hwColor);
void lcd_display_num(uint16_t hwXpos, uint16_t hwYpos, uint32_t chNum, uint8_t chLen, uint8_t chSize, uint16_t hwColor);
void lcd_display_string(uint16_t hwXpos, uint16_t hwYpos, const uint8_t *pchString, uint8_t chSize, uint16_t hwColor);
void lcd_draw_line(uint16_t hwXpos0, uint16_t hwYpos0, uint16_t hwXpos1, uint16_t hwYpos1, uint16_t hwColor);
void lcd_draw_circle(uint16_t hwXpos, uint16_t hwYpos, uint16_t hwRadius, uint16_t hwColor);
void lcd_fill_rect(uint16_t hwXpos, uint16_t hwYpos, uint16_t hwWidth, uint16_t hwHeight, uint16_t hwColor);
void lcd_draw_v_line(uint16_t hwXpos, uint16_t hwYpos, uint16_t hwHeight, uint16_t hwColor);
void lcd_draw_h_line(uint16_t hwXpos, uint16_t hwYpos, uint16_t hwWidth, uint16_t hwColor);
void lcd_draw_rect(uint16_t hwXpos, uint16_t hwYpos, uint16_t hwWidth, uint16_t hwHeight, uint16_t hwColor);
void lcd_clear_Rect(uint32_t color_front, uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1);

#endif
