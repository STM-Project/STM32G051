#include "pcf8575.h"
#include "i2c.h"

HAL_StatusTypeDef PCF8575_Write(uint8_t address, uint16_t value)
{
  if (address > 7) return HAL_ERROR;
  uint8_t bytes[2] = {(uint8_t)value, (uint8_t)(value >> 8)};
  return HAL_I2C_Master_Transmit(&hi2c2, (0x20U + address) << 1, bytes, 2, 10);
}

HAL_StatusTypeDef PCF8575_Read(uint8_t address, uint16_t *value)
{
  if (address > 7 || value == NULL) return HAL_ERROR;
  uint8_t bytes[2];
  HAL_StatusTypeDef status = HAL_I2C_Master_Receive(&hi2c2,
                              (0x20U + address) << 1, bytes, 2, 10);
  if (status == HAL_OK) *value = (uint16_t)(bytes[0] | ((uint16_t)bytes[1] << 8));
  return status;
}

uint8_t PCF8575_SetValues(uint16_t step, uint16_t divider)
{
  uint8_t errors = PCF8575_Write(0, step) == HAL_OK ? 0 : 1;
  if (PCF8575_Write(1, divider) != HAL_OK) errors |= 2;
  return errors;
}
