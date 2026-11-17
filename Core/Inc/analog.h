#ifndef ANALOG_H
#define ANALOG_H
#include "main.h"
#define ADC_INPUT_COUNT 5U
/* Order: PA1/IN1, PA2/IN2, PA3/IN3, PA6/IN6, PA7/IN7. */
void Analog_Init(void);
uint8_t Analog_Read(uint16_t values[ADC_INPUT_COUNT]);
HAL_StatusTypeDef Analog_SetDAC(uint8_t channel, uint16_t value);
#endif
