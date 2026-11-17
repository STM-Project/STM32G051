#!/usr/bin/env python3
"""Run actual radio and PCF8575 modules against a simulated I2C transport."""
import pathlib
import shutil
import subprocess
import tempfile
root = pathlib.Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as tmp:
    out = pathlib.Path(tmp)
    for name in ('radio', 'pcf8575'):
        for folder, ext in [('Src', 'c'), ('Inc', 'h')]:
            shutil.copy(root/'Core'/folder/(name+'.'+ext), out/(name+'.'+ext))
    (out/'main.h').write_text('''#ifndef MOCK_MAIN_H
#define MOCK_MAIN_H
#include <stdint.h>
#include <stddef.h>
typedef enum { HAL_OK, HAL_ERROR, HAL_BUSY, HAL_TIMEOUT } HAL_StatusTypeDef;
#endif
''')
    (out/'i2c.h').write_text('''#include "main.h"
extern int hi2c2;
HAL_StatusTypeDef HAL_I2C_Master_Transmit(int *, uint16_t, uint8_t *, uint16_t, uint32_t);
HAL_StatusTypeDef HAL_I2C_Master_Receive(int *, uint16_t, uint8_t *, uint16_t, uint32_t);
''')
    (out/'test.c').write_text('''
#include "radio.h"
#include "pcf8575.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
int hi2c2;
static unsigned calls;
static uint8_t failures;
static uint16_t addresses[2], values[2];
HAL_StatusTypeDef HAL_I2C_Master_Transmit(int *bus, uint16_t address,
  uint8_t *bytes, uint16_t count, uint32_t timeout) {
  assert(bus == &hi2c2 && count == 2 && timeout == 10);
  addresses[calls%2] = address;
  values[calls%2] = bytes[0] | ((uint16_t)bytes[1] << 8);
  ++calls;
  return (failures & (address == 0x40 ? 1 : 2)) ? HAL_ERROR : HAL_OK;
}
HAL_StatusTypeDef HAL_I2C_Master_Receive(int *bus, uint16_t address,
  uint8_t *bytes, uint16_t count, uint32_t timeout) {
  assert(bus == &hi2c2 && address == 0x42 && count == 2 && timeout == 10);
  bytes[0]=0x34; bytes[1]=0x12;
  return failures ? HAL_ERROR : HAL_OK;
}
int main(void) {
  assert(strcmp(Radio_Name(),"Radio Krakow")==0 && Radio_Frequency()==10160);
  Radio_Apply();
  assert(calls==2 && addresses[0]==0x40 && addresses[1]==0x42);
  assert(values[0]==0x12BF && values[1]==3179 && Radio_Errors()==0);
  Radio_Key(4); assert(Radio_Frequency()==10161 && Radio_Divider()==3181);
  for(int i=0;i<110;i++) Radio_Key(4);
  assert(Radio_Frequency()==10260); /* +1 MHz limit */
  for(int i=0;i<220;i++) Radio_Key(8);
  assert(Radio_Frequency()==10060); /* -1 MHz limit */
  Radio_Key(1); assert(Radio_Frequency()==8780 && strcmp(Radio_Name(),"RMF Classic")==0);
  Radio_Key(2); assert(Radio_Frequency()==10060); /* per-station tuning retained */
  for(int i=0;i<30;i++) Radio_Key(1);
  assert(strcmp(Radio_Name(),"Radio Famka")==0 && Radio_Frequency()==10050);
  for(int i=0;i<30;i++) Radio_Key(2);
  assert(strcmp(Radio_Name(),"Radio Krakow")==0);
  unsigned before=calls; Radio_Key(3); assert(calls==before);
  failures=1; Radio_Apply(); assert(Radio_Errors()==1 && calls==before+2);
  failures=2; Radio_Apply(); assert(Radio_Errors()==2);
  failures=3; Radio_Apply(); assert(Radio_Errors()==3);
  failures=0; Radio_Apply(); assert(Radio_Errors()==0); /* retry recovers */
  uint16_t value=0;
  assert(PCF8575_Read(1,&value)==HAL_OK && value==0x1234);
  failures=1; value=0xABCD;
  assert(PCF8575_Read(1,&value)==HAL_ERROR && value==0xABCD);
  assert(PCF8575_Read(8,&value)==HAL_ERROR && PCF8575_Read(1,NULL)==HAL_ERROR);
  before=calls; assert(PCF8575_Write(8,0)==HAL_ERROR && calls==before);
  puts("PASS: station list, tuning, dividers, PCF byte order/addresses, I2C errors");
}
''')
    subprocess.run(['cc','-std=gnu11','-Wall','-Wextra','-Werror',str(out/'radio.c'),str(out/'pcf8575.c'),str(out/'test.c'),'-o',str(out/'test')],check=True)
    subprocess.run([str(out/'test')],check=True)
