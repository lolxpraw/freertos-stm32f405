#ifndef __XPT2046_H
#define __XPT2046_H

#include "main.h"
#include "LCD_Driver.h"

// Định nghĩa Macro điều khiển chân CS (Chip Select) bằng HAL
#define XPT2046_CS_H()      HAL_GPIO_WritePin(TP_CS_GPIO_Port, TP_CS_Pin, GPIO_PIN_SET)
#define XPT2046_CS_L()      HAL_GPIO_WritePin(TP_CS_GPIO_Port, TP_CS_Pin, GPIO_PIN_RESET)

// Định nghĩa Macro đọc chân IRQ (Ngắt cảm ứng) bằng HAL
#define XPT2046_IRQ_READ()  HAL_GPIO_ReadPin(TP_IRQ_GPIO_Port, TP_IRQ_Pin)

// Khai báo hàm truyền nhận SPI
uint8_t xpt2046_spi_transfer(uint8_t data);
#define XPT2046_WRITE_BYTE(__DATA) xpt2046_spi_transfer(__DATA)

extern void xpt2046_init(void);
extern void xpt2046_read_xy(uint16_t *phwXpos, uint16_t *phwYpos);
extern uint8_t xpt2046_twice_read_xy(uint16_t *phwXpos, uint16_t *phwYpos);

#endif