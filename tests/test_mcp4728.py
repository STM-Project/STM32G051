#!/usr/bin/env python3
"""Verify MCP4728A1 channel writes using the real driver and a mock HAL."""
import pathlib
import shutil
import subprocess
import tempfile

root = pathlib.Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as tmp:
    out = pathlib.Path(tmp)
    shutil.copy(root / 'Core/Inc/mcp4728.h', out / 'mcp4728.h')
    (out / 'main.h').write_text('''
#pragma once
#include <stdint.h>
typedef enum { HAL_OK, HAL_ERROR, HAL_TIMEOUT } HAL_StatusTypeDef;
typedef struct { int unused; } I2C_HandleTypeDef;
''')
    (out / 'i2c.h').write_text('''
#include "main.h"
extern I2C_HandleTypeDef hi2c2;
HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef *, uint16_t, uint8_t *,
                                         uint16_t, uint32_t);
''')
    (out / 'test.c').write_text('''
#include "mcp4728.h"
#include "i2c.h"
#include <assert.h>
#include <string.h>

I2C_HandleTypeDef hi2c2;
static uint8_t packet[3];
static unsigned calls;
static HAL_StatusTypeDef status;

HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef *bus, uint16_t address,
                                         uint8_t *bytes, uint16_t length, uint32_t timeout)
{
    assert(bus == &hi2c2 && address == 0xC2 && length == 3 && timeout == 5);
    memcpy(packet, bytes, sizeof(packet));
    ++calls;
    return status;
}

int main(void)
{
    static const uint16_t codes[] = {0, 0x123, 0xABC, 4095};
    static const uint8_t expected[][3] = {
        {0x40, 0x00, 0x00}, {0x42, 0x01, 0x23},
        {0x44, 0x0A, 0xBC}, {0x46, 0x0F, 0xFF}
    };
    for (uint8_t channel = 0; channel < MCP4728_CHANNEL_COUNT; ++channel) {
        assert(MCP4728_WriteChannel(channel, codes[channel]) == HAL_OK);
        assert(memcmp(packet, expected[channel], sizeof(packet)) == 0);
    }
    assert(MCP4728_WriteChannel(4, 0) == HAL_ERROR);
    assert(MCP4728_WriteChannel(0, 4096) == HAL_ERROR);
    assert(calls == 4);
    status = HAL_TIMEOUT;
    assert(MCP4728_WriteChannel(3, 1) == HAL_TIMEOUT);
    assert(calls == 5);
}
''')
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror',
                    '-I', str(out),
                    str(root / 'Core/Src/mcp4728.c'), str(out / 'test.c'),
                    '-o', str(out / 'test')], check=True)
    subprocess.run([str(out / 'test')], check=True)
    print('PASS: MCP4728A1 channel packets, range checks and I2C timeout')
