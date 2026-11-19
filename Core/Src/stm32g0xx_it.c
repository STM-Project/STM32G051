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
static volatile uint32_t pb3_upper_count;
static volatile uint32_t pulse_hz[2];
static volatile uint8_t pulse_ready;
static uint32_t pulse_gate_start;
static uint32_t previous_pa0_edges;
static uint32_t previous_pb3_edges;
static uint8_t pulse_running;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void ConfigurePulseTimer(TIM_TypeDef *timer, uint32_t channel_selection,
                                uint32_t trigger_selection, uint32_t period)
{
  timer->CR1 = TIM_CR1_URS; /* Only counter overflow requests an update IRQ. */
  timer->DIER = 0;
  timer->SMCR = 0;
  timer->CCER = 0; /* Rising polarity. */
  timer->CCMR1 = channel_selection;
  timer->PSC = 0;
  timer->ARR = period;
  timer->EGR = TIM_EGR_UG;
  timer->SR = 0;
  timer->SMCR = trigger_selection | TIM_SMCR_SMS_2 | TIM_SMCR_SMS_1 | TIM_SMCR_SMS_0;
}

void Pulse_Init(void)
{
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_TIM1_CLK_ENABLE();
  __HAL_RCC_TIM2_CLK_ENABLE();

  GPIO_InitTypeDef pin = {
    .Pin = GPIO_PIN_0,
    .Mode = GPIO_MODE_AF_PP,
    .Pull = GPIO_NOPULL,
    .Alternate = GPIO_AF2_TIM2
  };
  HAL_GPIO_Init(GPIOA, &pin);
  pin.Pin = GPIO_PIN_3;
  pin.Alternate = GPIO_AF1_TIM1;
  HAL_GPIO_Init(GPIOB, &pin);

  /* External clock mode 1: PA0 uses TIM2 TI1FP1, PB3 uses TIM1 TI2FP2. */
  TIM1->RCR = 0;
  ConfigurePulseTimer(TIM2, TIM_CCMR1_CC1S_0,
                      TIM_SMCR_TS_2 | TIM_SMCR_TS_0, UINT32_MAX);
  ConfigurePulseTimer(TIM1, TIM_CCMR1_CC2S_0,
                      TIM_SMCR_TS_2 | TIM_SMCR_TS_1, UINT16_MAX);

  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  HAL_NVIC_ClearPendingIRQ(TIM1_BRK_UP_TRG_COM_IRQn);
  HAL_NVIC_SetPriority(TIM1_BRK_UP_TRG_COM_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(TIM1_BRK_UP_TRG_COM_IRQn);
  TIM1->DIER = TIM_DIER_UIE;
  pulse_gate_start = HAL_GetTick();
  TIM2->CR1 |= TIM_CR1_CEN;
  TIM1->CR1 |= TIM_CR1_CEN;
  pulse_running = 1;
  __set_PRIMASK(primask);
}

static uint32_t ReadPB3Counter(void)
{
  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  uint32_t high = pb3_upper_count;
  uint32_t low = TIM1->CNT;
  /* Account for a rollover whose update interrupt is still pending. */
  if ((TIM1->SR & TIM_SR_UIF) != 0U) {
    high += UINT16_MAX + 1U;
    low = TIM1->CNT;
  }
  __set_PRIMASK(primask);
  return high + low;
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
  uint32_t pa0 = TIM2->CNT;
  uint32_t pb3 = ReadPB3Counter();
  pulse_hz[0] = (pa0 - previous_pa0_edges) * 2U;
  pulse_hz[1] = (pb3 - previous_pb3_edges) * 2U;
  previous_pa0_edges = pa0;
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
/* The TIM1 update vector is owned here so CubeMX can preserve this handler. */
void TIM1_BRK_UP_TRG_COM_IRQHandler(void)
{
  if ((TIM1->SR & TIM_SR_UIF) != 0U) {
    TIM1->SR = ~(uint32_t)TIM_SR_UIF;
    pb3_upper_count += UINT16_MAX + 1U;
  }
}

/* USER CODE END 1 */
