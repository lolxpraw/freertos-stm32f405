#ifndef __TOUCH_H_
#define __TOUCH_H_

#include "main.h"

#define TP_PRESS_DOWN           0x80
#define TP_PRESSED              0x40

typedef struct {
	uint16_t hwXpos0;
	uint16_t hwYpos0;
	uint16_t hwXpos;
	uint16_t hwYpos;
	uint8_t chStatus;
	uint8_t chType;
	short iXoff;
	short iYoff;
	float fXfac;
	float fYfac;
} tp_dev_t;

extern void tp_init(void);
extern void tp_adjust(void);
extern void tp_dialog(void);
extern void tp_draw_board(void);
extern uint8_t tp_scan(uint8_t chCoordType);

// Hàm được thêm vào để dễ lấy tọa độ (dành cho vòng lặp while(1) của bạn)
extern uint8_t Get_Touch_XY(uint16_t *x, uint16_t *y);

#endif