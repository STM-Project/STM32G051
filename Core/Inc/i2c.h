#ifndef __I2C_H__
#define __I2C_H__
#ifdef __cplusplus
extern "C" {
#endif
#include "main.h"
extern I2C_HandleTypeDef hi2c2;
void MX_I2C2_Init(void);
/* USER CODE BEGIN Prototypes */
uint8_t SSD1306_WriteCommand(uint16_t addr, uint8_t command);
uint8_t SSD1306_WriteData(uint16_t addr, uint8_t *data, uint16_t size);
/* USER CODE END Prototypes */
#ifdef __cplusplus
}
#endif
#endif
