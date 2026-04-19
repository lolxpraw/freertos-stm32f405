#include "xpt2046.h"

// Sử dụng bộ SPI1 đã khởi tạo trong main.c
extern SPI_HandleTypeDef hspi1; 

// Hàm truyền nhận SPI sử dụng thư viện HAL
uint8_t xpt2046_spi_transfer(uint8_t data)
{
    uint8_t rx_data = 0;
    HAL_SPI_TransmitReceive(&hspi1, &data, &rx_data, 1, HAL_MAX_DELAY);
    return rx_data;
}

uint8_t xpt2046_write_byte(uint8_t chData)
{
    return XPT2046_WRITE_BYTE(chData);
}

void xpt2046_init(void)
{
	uint16_t hwXpos, hwYpos;
	XPT2046_CS_H();
	xpt2046_read_xy(&hwXpos, &hwYpos);
}

uint16_t xpt2046_read_ad_value(uint8_t chCmd)
{
    uint16_t hwData = 0;
    
    XPT2046_CS_L();
    xpt2046_write_byte(chCmd);
    hwData = xpt2046_write_byte(0x00);
    hwData <<= 8;
    hwData |= xpt2046_write_byte(0x00);
    hwData >>= 3;
    XPT2046_CS_H();
    
    return hwData;
}

#define READ_TIMES  5
#define LOST_NUM    1
uint16_t xpt2046_read_average(uint8_t chCmd)
{
    uint8_t i, j;
    uint16_t hwbuffer[READ_TIMES], hwSum = 0, hwTemp;

    for (i = 0; i < READ_TIMES; i ++) {
        hwbuffer[i] = xpt2046_read_ad_value(chCmd);
    }
    for (i = 0; i < READ_TIMES - 1; i ++) {
        for (j = i + 1; j < READ_TIMES; j ++) {
            if (hwbuffer[i] > hwbuffer[j]) {
                hwTemp = hwbuffer[i];
                hwbuffer[i] = hwbuffer[j];
                hwbuffer[j] = hwTemp;
            }
        }
    }
    for (i = LOST_NUM; i < READ_TIMES - LOST_NUM; i ++) {
        hwSum += hwbuffer[i];
    }
    hwTemp = hwSum / (READ_TIMES - 2 * LOST_NUM);

    return hwTemp;
}

void xpt2046_read_xy(uint16_t *phwXpos, uint16_t *phwYpos)
{
	*phwXpos = xpt2046_read_average(0x90);
	*phwYpos = xpt2046_read_average(0xD0);
}

#define ERR_RANGE 50
uint8_t xpt2046_twice_read_xy(uint16_t *phwXpos, uint16_t *phwYpos)
{
	uint16_t hwXpos1, hwYpos1, hwXpos2, hwYpos2;

	xpt2046_read_xy(&hwXpos1, &hwYpos1);
	xpt2046_read_xy(&hwXpos2, &hwYpos2);

	if (((hwXpos2 <= hwXpos1 && hwXpos1 < hwXpos2 + ERR_RANGE) || (hwXpos1 <= hwXpos2 && hwXpos2 < hwXpos1 + ERR_RANGE))
	&& ((hwYpos2 <= hwYpos1 && hwYpos1 < hwYpos2 + ERR_RANGE) || (hwYpos1 <= hwYpos2 && hwYpos2 < hwYpos1 + ERR_RANGE))) {
		*phwXpos = (hwXpos1 + hwXpos2) >> 1;
		*phwYpos = (hwYpos1 + hwYpos2) >> 1;
		return 1;
	}

	return 0;
}