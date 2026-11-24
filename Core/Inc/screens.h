#ifndef SCREENS_H
#define SCREENS_H
#include "main.h"
#include "analog.h"
#include "mcp4728.h"
/* USER CODE BEGIN MEASUREMENTS */
#define DAC_VALUE_COUNT (INTERNAL_DAC_COUNT + MCP4728_CHANNEL_COUNT)
void Screen_DrawCalibrationWarning(uint8_t failed);
void Screen_DrawAnalog(const uint16_t adc[ADC_INPUT_COUNT], uint8_t valid,
                       const uint16_t dac[DAC_VALUE_COUNT], uint8_t selected,
                       const uint32_t hz[2], uint8_t external_dac_error);
/* USER CODE END MEASUREMENTS */
/* USER CODE BEGIN FACE */
void Screen_DrawFace(uint8_t frame);
/* USER CODE END FACE */
/* USER CODE BEGIN PCF */
void Screen_DrawPCF(const uint16_t values[2], uint8_t selected_byte,
                    uint8_t errors);
/* USER CODE END PCF */
void Screen_DrawExample(uint8_t example);
void Screen_DrawRadio(void);
void Screen_DrawRadioIntro(void);
#endif
