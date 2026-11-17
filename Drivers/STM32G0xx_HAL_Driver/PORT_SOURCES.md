Added ADC, DAC and I2C HAL/LL files are unmodified from STMicroelectronics
stm32g0xx-hal-driver v1.4.7, commit a0cf8a8b96183fdcc2e3b1cf0bcf0825f27bd0c9:
https://github.com/STMicroelectronics/stm32g0xx-hal-driver/tree/v1.4.7
This matches the version reported by the existing stm32g0xx_hal.c.
See LICENSE.md. OLED/fonts and GPIO input helpers were ported from the sibling
STM32G031_PCF project. The four example display frames, 24 FM station
presets, divider formula and two-PCF8575 wiring/protocol are also ported
from that project; radio/PCF logic now lives in separate application modules.
