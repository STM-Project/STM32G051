#ifndef SCREENS_H
#define SCREENS_H
#include "main.h"
#include "analog.h"
/* USER CODE BEGIN MEASUREMENTS */
void Screen_DrawAnalog(const uint16_t adc[ADC_INPUT_COUNT], uint8_t valid,
                       const uint16_t dac[2], uint8_t selected, const uint32_t hz[2]);
/* USER CODE END MEASUREMENTS */
/* USER CODE BEGIN FACE */
void Screen_DrawFace(uint8_t frame);
/* USER CODE END FACE */
void Screen_DrawExample(uint8_t example);
void Screen_DrawRadio(void);
void Screen_DrawRadioIntro(void);
#endif
