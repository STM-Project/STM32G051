#ifndef PCF8575_H
#define PCF8575_H
#include "main.h"
/* Address selectors 0..7 correspond to 7-bit addresses 0x20..0x27.
 * PCF8575 uses two data bytes, low port first, with no register address. */
HAL_StatusTypeDef PCF8575_Write(uint8_t address, uint16_t value);
HAL_StatusTypeDef PCF8575_Read(uint8_t address, uint16_t *value);
/* Same wiring as G031: step divider at 0x20, frequency divider at 0x21.
 * Return error bits: bit0 = 0x20, bit1 = 0x21. Both writes are attempted. */
uint8_t PCF8575_SetValues(uint16_t step, uint16_t divider);
#endif
