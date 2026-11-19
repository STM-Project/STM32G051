/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32g0xx_it.c
  * @brief   Interrupt Service Routines.
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
#include "main.h"
#include "stm32g0xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "adc.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
#define PULSE_GATE_MS 500U
static volatile uint32_t pf2_edges;
static volatile uint32_t pulse_hz[2];
static volatile uint8_t pulse_ready;
static uint32_t pulse_gate_start;
static uint32_t previous_pf2_edges;
static uint32_t previous_pb3_edges;
static uint8_t pulse_running;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void Pulse_Init(void)
{
  GPIO_InitTypeDef pin = {0};
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_TIM2_CLK_ENABLE();

  pin.Pin = GPIO_PIN_3;
  pin.Mode = GPIO_MODE_AF_PP;
  pin.Pull = GPIO_NOPULL;
  pin.Alternate = GPIO_AF2_TIM2;
  HAL_GPIO_Init(GPIOB, &pin);

  TIM2->CR1 = 0;
  TIM2->SMCR = 0;
  TIM2->CCER = 0;
  TIM2->CCMR1 = TIM_CCMR1_CC2S_0; /* IC2 takes the unfiltered TI2 input. */
  TIM2->PSC = 0;
  TIM2->ARR = UINT32_MAX;
  TIM2->EGR = TIM_EGR_UG;
  TIM2->SR = 0;
  /* TS=110 selects TI2FP2. SMS=111 is external clock mode 1. */
  TIM2->SMCR = TIM_SMCR_TS_2 | TIM_SMCR_TS_1 |
               TIM_SMCR_SMS_2 | TIM_SMCR_SMS_1 | TIM_SMCR_SMS_0;

  pin.Pin = GPIO_PIN_2;
  pin.Mode = GPIO_MODE_IT_RISING;
  pin.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOF, &pin);

  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  __HAL_GPIO_EXTI_CLEAR_RISING_IT(GPIO_PIN_2);
  HAL_NVIC_ClearPendingIRQ(EXTI2_3_IRQn);
  HAL_NVIC_SetPriority(EXTI2_3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI2_3_IRQn);
  pulse_gate_start = HAL_GetTick();
  TIM2->CR1 = TIM_CR1_CEN;
  pulse_running = 1;
  __set_PRIMASK(primask);
}

uint8_t Pulse_ReadHz(uint32_t hz[2])
{
  if (!pulse_ready) return 0;
  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  hz[0] = pulse_hz[0];
  hz[1] = pulse_hz[1];
  pulse_ready = 0;
  __set_PRIMASK(primask);
  return 1;
}

static void Pulse_Sample(void)
{
  uint32_t now = HAL_GetTick();
  if (!pulse_running || (uint32_t)(now - pulse_gate_start) < PULSE_GATE_MS)
    return;

  pulse_gate_start += PULSE_GATE_MS;
  uint32_t pf2 = pf2_edges;
  uint32_t pb3 = TIM2->CNT;
  pulse_hz[0] = (pf2 - previous_pf2_edges) * 2U;
  pulse_hz[1] = (pb3 - previous_pb3_edges) * 2U;
  previous_pf2_edges = pf2;
  previous_pb3_edges = pb3;
  pulse_ready = 1;
}

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M0+ Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
   while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
  /* USER CODE BEGIN SVC_IRQn 0 */

  /* USER CODE END SVC_IRQn 0 */
  /* USER CODE BEGIN SVC_IRQn 1 */

  /* USER CODE END SVC_IRQn 1 */
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
  /* USER CODE BEGIN PendSV_IRQn 0 */

  /* USER CODE END PendSV_IRQn 0 */
  /* USER CODE BEGIN PendSV_IRQn 1 */

  /* USER CODE END PendSV_IRQn 1 */
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */
  Pulse_Sample();

  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32G0xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32g0xx.s).                    */
/******************************************************************************/

void DMA1_Channel1_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel1_IRQn 0 */
  /* USER CODE END DMA1_Channel1_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_adc1);
  /* USER CODE BEGIN DMA1_Channel1_IRQn 1 */
  /* USER CODE END DMA1_Channel1_IRQn 1 */
}
void ADC1_COMP_IRQHandler(void)
{
  /* USER CODE BEGIN ADC1_COMP_IRQn 0 */
  /* USER CODE END ADC1_COMP_IRQn 0 */
  HAL_ADC_IRQHandler(&hadc1);
  /* USER CODE BEGIN ADC1_COMP_IRQn 1 */
  /* USER CODE END ADC1_COMP_IRQn 1 */
}

/* USER CODE BEGIN 1 */
/* The CubeMX EXTI vector is left disabled so this handler survives regeneration. */
void EXTI2_3_IRQHandler(void)
{
  if (__HAL_GPIO_EXTI_GET_RISING_IT(GPIO_PIN_2) != 0U) {
    __HAL_GPIO_EXTI_CLEAR_RISING_IT(GPIO_PIN_2);
    ++pf2_edges;
  }
}

/* USER CODE END 1 */
