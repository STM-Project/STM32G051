/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */
#define TIME_FOR_PRESS		10
#define TIME_FOR_RELEASE	20

/* USER CODE END 1 */

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
*/
void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef g = {0};
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  g.Mode = GPIO_MODE_INPUT;
  g.Pull = GPIO_PULLUP;
  g.Pin = GPIO_PIN_7; HAL_GPIO_Init(GPIOB, &g);
  g.Pin = GPIO_PIN_14 | GPIO_PIN_15; HAL_GPIO_Init(GPIOC, &g);
  g.Pin = GPIO_PIN_2; HAL_GPIO_Init(GPIOF, &g);

}

/* USER CODE BEGIN 2 */
int MX_GPIO_IsPress(GPIO_TypeDef const *GPIOx, uint16_t GPIO_Pin){
	if(GPIO_PIN_RESET==HAL_GPIO_ReadPin(GPIOx,GPIO_Pin)){
		HAL_Delay(TIME_FOR_PRESS);
		if(GPIO_PIN_RESET==HAL_GPIO_ReadPin(GPIOx,GPIO_Pin)){
			HAL_Delay(TIME_FOR_PRESS);
			if(GPIO_PIN_RESET==HAL_GPIO_ReadPin(GPIOx,GPIO_Pin)){
				HAL_Delay(TIME_FOR_PRESS);
				if(GPIO_PIN_RESET==HAL_GPIO_ReadPin(GPIOx,GPIO_Pin)){
					return 1;
				}
			}
		}
	}
	return 0;
}

int MX_GPIO_IsRelease(GPIO_TypeDef const *GPIOx, uint16_t GPIO_Pin){
	return HAL_GPIO_ReadPin(GPIOx,GPIO_Pin);
}

void MX_GPIO_WaitForRelease(GPIO_TypeDef const *GPIOx, uint16_t GPIO_Pin){
	while(!MX_GPIO_IsRelease(GPIOx,GPIO_Pin));
	HAL_Delay(TIME_FOR_RELEASE);
}

int INPUT_ChekState(INPUT_PIN *in, int size){
	#define MAX_PROBES	3
	int temp[size], nmbrReset=0, nrPress=1;
	LOOP_FOR(i,size){ temp[i]=HAL_GPIO_ReadPin(in[i].gpio,in[i].pin); }
	LOOP_FOR(j,MAX_PROBES){	  HAL_Delay(TIME_FOR_PRESS);    LOOP_FOR(i,size){ if(temp[i]!=HAL_GPIO_ReadPin(in[i].gpio,in[i].pin)) return 0; }  }	/* if return 0 then the press key action is still ongoing  */
	LOOP_FOR(i,size){ if(GPIO_PIN_RESET==temp[i]){ nmbrReset++; nrPress*=(i+1); } }
	if	   (0==nmbrReset) return press_0;
	else if(1==nmbrReset) return press_0 + nrPress;
	else if(2==nmbrReset) return press_3 + nrPress;
	else if(3==nmbrReset) return press_10 + nrPress;
	else				  return press_35;	/* here all input are pressed (size = 4) */
	#undef MAX_PROBES
}

INPUT_PRESS INPUT_GetPressPin(INPUT_PIN *in, int size)
{
	INPUT_PRESS retVal = {0};
	int pressCode = INPUT_ChekState(in,size);

	if	   (IS_RANGE(pressCode,press_0, press_0))	{ retVal.mode=0; retVal.code=0; 		}
	else if(IS_RANGE(pressCode,press_1, press_4))	{ retVal.mode=1; retVal.code=pressCode; }
	else if(IS_RANGE(pressCode,press_5, press_15))	{ retVal.mode=2; retVal.code=pressCode; }
	else if(IS_RANGE(pressCode,press_16,press_34))	{ retVal.mode=3; retVal.code=pressCode; }
	else if(IS_RANGE(pressCode,press_35,press_35))	{ retVal.mode=4; retVal.code=pressCode; }
	else											{ retVal.mode=0; retVal.code=0; 		}
	return retVal;

	/* Description of the return codes */
	/* nPress  pins	  -> return code

		   0:  1111   -> 0

		   1:  1110   -> 1
		   1:  1101   -> 2
		   1:  1011   -> 3
		   1:  0111   -> 4

		   2:  1100   -> 5
		   2:  1010   -> 6
		   2:  0110   -> 7
		   2:  0101   -> 11
		   2:  0011   -> 15
		   2:  1001   -> 9

		   3:  1000   -> 16
		   3:  0001   -> 34
		   3:  0010   -> 22
		   3:  0100   -> 18

		   4:  0000   -> 35
	*/
}

int INPUT_IsAllRelease(INPUT_PIN *in, int size){
	LOOP_FOR(i,size){  if(GPIO_PIN_RESET==HAL_GPIO_ReadPin(in[i].gpio,in[i].pin)) return 0;  }
	return 1;
}
int INPUT_IsAnyPress(INPUT_PIN *in, int size){
	LOOP_FOR(i,size){  if(GPIO_PIN_RESET==HAL_GPIO_ReadPin(in[i].gpio,in[i].pin)) return 1;  }
	return 0;
}


#define KEY_DEBOUNCE_MS 8U
#define KEY_CHORD_MS 40U
#define KEY_REPEAT_START_MS 1000U
#define KEY_REPEAT_STEP_MS 5U

/* Nonblocking events: bit 0..3 = pin1..pin4. Only enabled single
 * keys repeat, and only while the original key stays down on its own. */
uint8_t INPUT_PollPress(uint8_t repeat_mask, uint32_t *repeat_held_ms)
{
  static uint8_t candidate, stable, armed = 1, collecting, pending;
  static uint8_t repeat_key;
  static uint32_t changed, started, last_event;
  static uint32_t repeat_delay;
  *repeat_held_ms = 0U;
  uint8_t raw = 0;
  const INPUT_PIN pins[4] = {
    {GPIOB, GPIO_PIN_7}, {GPIOC, GPIO_PIN_14},
    {GPIOC, GPIO_PIN_15}, {GPIOF, GPIO_PIN_2}
  };
  for (uint8_t i = 0; i < 4; ++i)
    if (HAL_GPIO_ReadPin(pins[i].gpio, pins[i].pin) == GPIO_PIN_RESET)
      raw |= (uint8_t)(1U << i);
  uint32_t now = HAL_GetTick();
  if (raw != candidate) { candidate = raw; changed = now; }
  if (candidate != stable && (uint32_t)(now - changed) >= KEY_DEBOUNCE_MS) {
    stable = candidate;
    repeat_key = 0;
    if (stable == 0) {
      armed = 1;
      if (collecting) { collecting = 0; return pending; }
    } else if (armed) {
      if (!collecting) { collecting = 1; pending = 0; started = now; }
      pending |= stable;
    }
  }
  /* Wait for a changing raw mask to settle, so a second key arriving near
   * the deadline cannot accidentally cause the first key to fire alone. */
  if (collecting && candidate == stable && stable != 0 &&
      (uint32_t)(now - started) >= KEY_CHORD_MS) {
    collecting = 0;
    armed = 0;
    if (pending == stable && (pending & (pending - 1U)) == 0 &&
        (pending & repeat_mask) != 0) {
      repeat_key = pending;
      last_event = now;
      repeat_delay = KEY_REPEAT_START_MS;
    }
    return pending;
  }
  if ((repeat_key & repeat_mask) != 0 && raw == repeat_key &&
      (uint32_t)(now - last_event) >= repeat_delay) {
    last_event = now;
    repeat_delay = KEY_REPEAT_STEP_MS;
    *repeat_held_ms = now - started;
    return repeat_key;
  }
  return 0;
}

/* USER CODE END 2 */
