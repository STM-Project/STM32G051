#ifndef ANALOG_H
#define ANALOG_H
#include "main.h"
/* USER CODE BEGIN ADC_INPUTS */
#define ADC_INPUT_COUNT 6U
/* Order: PA1/IN1, PA2/IN2, PA3/IN3, PA6/IN6, PA7/IN7, PB0/IN8. */
/* USER CODE END ADC_INPUTS */
/* USER CODE BEGIN CALIBRATION_STATUS */
enum {
  ANALOG_CAL_ADC1 = 1U << 0,
  ANALOG_CAL_DAC_CH1 = 1U << 1,
  ANALOG_CAL_DAC_CH2 = 1U << 2
};
uint8_t Analog_Init(void);
/* USER CODE END CALIBRATION_STATUS */
uint8_t Analog_Read(uint16_t values[ADC_INPUT_COUNT]);
#define INTERNAL_DAC_COUNT 2U
HAL_StatusTypeDef Analog_SetDAC(uint8_t channel, uint16_t value);
#endif
