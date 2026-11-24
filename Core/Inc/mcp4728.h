#ifndef MCP4728_H
#define MCP4728_H

#include "main.h"

#define MCP4728_CHANNEL_COUNT 4U

/* Channels A–D are numbered 0–3. Value is a 12-bit DAC code. */
HAL_StatusTypeDef MCP4728_WriteChannel(uint8_t channel, uint16_t value);

#endif
