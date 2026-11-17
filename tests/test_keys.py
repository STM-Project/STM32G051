#!/usr/bin/env python3
"""Host test of the actual gpio.c debounce logic with simulated pins/ticks."""
import pathlib
import shutil
import subprocess
import tempfile
root = pathlib.Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as tmp:
    out = pathlib.Path(tmp)
    for name in ('gpio.c', 'gpio.h'):
        shutil.copy(root / 'Core' / ('Src' if name.endswith('.c') else 'Inc') / name, out / name)
    types = (root / 'Core/Inc/main.h').read_text().split('/* USER CODE BEGIN ET */')[1].split('/* USER CODE END ET */')[0]
    (out / 'main.h').write_text('''
#ifndef MOCK_MAIN_H
#define MOCK_MAIN_H
#include <stdint.h>
typedef struct { unsigned unused; } GPIO_TypeDef;
typedef struct { unsigned Pin, Mode, Pull, Speed, Alternate; } GPIO_InitTypeDef;
extern GPIO_TypeDef ports[4];
#define GPIOA (&ports[0])
#define GPIOB (&ports[1])
#define GPIOC (&ports[2])
#define GPIOF (&ports[3])
#define GPIO_PIN_0 1
#define GPIO_PIN_2 4
#define GPIO_PIN_3 8
#define GPIO_PIN_7 128
#define GPIO_PIN_14 16384
#define GPIO_PIN_15 32768
#define GPIO_PIN_RESET 0
#define GPIO_MODE_INPUT 0
#define GPIO_MODE_AF_PP 1
#define GPIO_PULLUP 1
#define GPIO_NOPULL 0
#define GPIO_SPEED_FREQ_LOW 0
#define GPIO_AF2_TIM2 2
#define GPIO_AF1_TIM3 1
#define __HAL_RCC_GPIOA_CLK_ENABLE() ((void)0)
#define __HAL_RCC_GPIOB_CLK_ENABLE() ((void)0)
#define __HAL_RCC_GPIOC_CLK_ENABLE() ((void)0)
#define __HAL_RCC_GPIOF_CLK_ENABLE() ((void)0)
int HAL_GPIO_ReadPin(const GPIO_TypeDef *, uint16_t);
void HAL_GPIO_Init(GPIO_TypeDef *, GPIO_InitTypeDef *);
void HAL_Delay(uint32_t);
uint32_t HAL_GetTick(void);
''' + types + '\n#endif\n')
    (out / 'test.c').write_text('''
#include "gpio.h"
#include <assert.h>
#include <stdio.h>
GPIO_TypeDef ports[4];
static uint32_t tick;
static uint8_t keys;
uint32_t HAL_GetTick(void) { return tick; }
void HAL_Delay(uint32_t ms) { tick += ms; }
void HAL_GPIO_Init(GPIO_TypeDef *p, GPIO_InitTypeDef *g) { (void)p; (void)g; }
int HAL_GPIO_ReadPin(const GPIO_TypeDef *p, uint16_t pin) {
  unsigned mask = p == GPIOB && pin == GPIO_PIN_7 ? 1 :
                  p == GPIOC && pin == GPIO_PIN_14 ? 2 :
                  p == GPIOC && pin == GPIO_PIN_15 ? 4 :
                  p == GPIOF && pin == GPIO_PIN_2 ? 8 : 0;
  return (keys & mask) ? 0 : 1;
}
static uint8_t poll(uint32_t t, uint8_t k) { tick=t; keys=k; return INPUT_PollPress(); }
int main(void) {
  assert(poll(0,0)==0);
  assert(poll(1,1)==0); assert(poll(20,0)==0); /* bounce */
  assert(poll(25,1)==0); assert(poll(54,1)==0); assert(poll(55,1)==1);
  assert(poll(1000,1)==0); /* held key never repeats */
  assert(poll(1001,0)==0); assert(poll(1020,1)==0); assert(poll(1050,1)==0);
  assert(poll(1051,0)==0); assert(poll(1081,0)==0);
  assert(poll(1082,2)==0); assert(poll(1112,2)==2);
  assert(poll(1113,0)==0); assert(poll(1143,0)==0);
  assert(poll(1144,4)==0); assert(poll(1174,4)==4);
  assert(poll(1175,0)==0); assert(poll(1205,0)==0);
  assert(poll(1206,8)==0); assert(poll(1236,8)==8);
  assert(poll(1237,0)==0); assert(poll(1267,0)==0);
  assert(poll(1268,3)==0); assert(poll(1298,3)==3); /* chord once */
  assert(poll(1299,1)==0); assert(poll(1329,1)==0); /* no action until release */
  assert(poll(1330,0)==0); assert(poll(1360,0)==0);
  assert(poll(UINT32_MAX-10,1)==0); assert(poll(18,1)==0);
  assert(poll(19,1)==1); /* debounce across tick wrap */
  puts("PASS: pin mapping, bounce, hold, release, chords, tick wrap");
}
''')
    subprocess.run(['cc', '-std=gnu11', '-Wall', '-Wextra', '-Werror', str(out/'gpio.c'), str(out/'test.c'), '-o', str(out/'test')], check=True)
    subprocess.run([str(out/'test')], check=True)
