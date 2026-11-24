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
static uint8_t repeat_mask;
static uint32_t repeat_held_ms;
static uint8_t poll(uint32_t t, uint8_t k) {
  tick = t; keys = k; return INPUT_PollPress(repeat_mask, &repeat_held_ms);
}
int main(void) {
  assert(poll(0,0)==0);
  assert(poll(1,1)==0); assert(poll(7,0)==0); /* bounce */
  assert(poll(10,1)==0); assert(poll(17,1)==0); assert(poll(18,1)==0);
  assert(poll(57,1)==0); assert(poll(58,1)==1);
  assert(poll(1100,1)==0); /* FM and other screens do not repeat */
  assert(poll(1101,0)==0); assert(poll(1109,0)==0);
  repeat_mask = 3;
  assert(poll(1110,2)==0); assert(poll(1118,2)==0);
  assert(poll(1120,0)==0); assert(poll(1127,0)==0); assert(poll(1128,0)==2);
  assert(poll(1129,1)==0); assert(poll(1137,1)==0); assert(poll(1177,1)==1);
  assert(poll(2176,1)==0); assert(poll(2177,1)==1);
  assert(poll(2181,1)==0); assert(poll(2182,1)==1);
  assert(poll(2187,0)==0); /* release stops repeat immediately */
  assert(poll(2195,0)==0);
  assert(poll(2196,2)==0); assert(poll(2204,2)==0); assert(poll(2244,2)==2);
  assert(poll(3244,2)==2); assert(poll(3249,2)==2);
  assert(poll(3254,3)==0); assert(poll(3262,3)==0); /* extra key cancels repeat */
  assert(poll(3263,2)==0); assert(poll(3271,2)==0); assert(poll(3280,2)==0);
  assert(poll(3281,0)==0); assert(poll(3289,0)==0);
  assert(poll(3300,4)==0); assert(poll(3308,4)==0); assert(poll(3348,4)==4);
  assert(poll(4400,4)==0);
  assert(poll(4401,0)==0); assert(poll(4409,0)==0);
  assert(poll(4410,8)==0); assert(poll(4418,8)==0); assert(poll(4458,8)==8);
  assert(poll(5500,8)==0);
  assert(poll(5501,0)==0); assert(poll(5509,0)==0);
  /* A second or third key arriving at the deadline can still join a chord. */
  assert(poll(5600,1)==0); assert(poll(5608,1)==0);
  assert(poll(5648,3)==0); assert(poll(5656,3)==3);
  assert(poll(6660,3)==0);
  assert(poll(6661,1)==0); assert(poll(6669,1)==0); /* partial release */
  assert(poll(6670,0)==0); assert(poll(6678,0)==0);
  assert(poll(6700,5)==0); assert(poll(6708,5)==0);
  assert(poll(6748,7)==0); assert(poll(6756,7)==7);
  assert(poll(6757,0)==0); assert(poll(6765,0)==0);
  /* The animation uses the previously free pin1+pin2+pin4 chord. */
  assert(poll(6800,11)==0); assert(poll(6808,11)==0); assert(poll(6848,11)==11);
  assert(poll(7900,11)==0);
  assert(poll(7901,0)==0); assert(poll(7909,0)==0);
  assert(poll(7910,3)==0); assert(poll(7918,3)==0); assert(poll(7958,3)==3);
  assert(poll(7959,0)==0); assert(poll(7967,0)==0);
  assert(poll(8000,13)==0); assert(poll(8008,13)==0); assert(poll(8048,13)==13);
  assert(poll(8050,0)==0); assert(poll(8058,0)==0);
  /* Debounce and repeat timing remain valid across tick rollover. */
  assert(poll(UINT32_MAX-55,1)==0); assert(poll(UINT32_MAX-47,1)==0);
  assert(poll(UINT32_MAX-7,1)==1);
  assert(poll(991,1)==0); assert(poll(992,1)==1); assert(poll(997,1)==1);
  assert(poll(4952,1)==1 && repeat_held_ms==5000U);
  assert(poll(4957,1)==1 && repeat_held_ms==5005U);
  assert(poll(4962,0)==0 && repeat_held_ms==0U);
  assert(poll(4970,0)==0);
  assert(poll(5000,2)==0); assert(poll(5008,2)==0);
  assert(poll(5048,2)==2 && repeat_held_ms==0U);
  assert(poll(10008,2)==2 && repeat_held_ms==5000U);
  assert(poll(10013,2)==2 && repeat_held_ms==5005U);
  puts("PASS: pins, debounce, rapid taps, chords, DAC-only repeat and tick wrap");
}
''')
    subprocess.run(['cc', '-std=gnu11', '-Wall', '-Wextra', '-Werror', str(out/'gpio.c'), str(out/'test.c'), '-o', str(out/'test')], check=True)
    subprocess.run([str(out/'test')], check=True)
