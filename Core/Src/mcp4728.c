#include "mcp4728.h"
#include "i2c.h"

#define MCP4728A1_ADDRESS (0x61U << 1)

HAL_StatusTypeDef MCP4728_WriteChannel(uint8_t channel, uint16_t value)
{
  if (channel >= MCP4728_CHANNEL_COUNT || value > 4095U)
    return HAL_ERROR;

  /* Multi-Write: one input register, UDAC=0 for immediate update.
     VREF=VDD, power-down disabled, gain=1. */
  uint8_t bytes[] = {0x40U | (channel << 1), value >> 8, (uint8_t)value};
  return HAL_I2C_Master_Transmit(&hi2c2, MCP4728A1_ADDRESS, bytes, sizeof(bytes), 5U);
}
