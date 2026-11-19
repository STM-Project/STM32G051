#include "adc.h"
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */
static void ADC_Check(HAL_StatusTypeDef status)
{
  if (status != HAL_OK) Error_Handler();
}
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;
void MX_ADC1_Init(void)
{
  /* USER CODE BEGIN ADC1_Init 0 */
  /* USER CODE END ADC1_Init 0 */
  ADC_ChannelConfTypeDef adc_channel = {0};
  const uint32_t channels[5U] = {
    ADC_CHANNEL_1, ADC_CHANNEL_2, ADC_CHANNEL_3, ADC_CHANNEL_6, ADC_CHANNEL_7
  };
  const uint32_t ranks[5U] = {
    ADC_REGULAR_RANK_1, ADC_REGULAR_RANK_2, ADC_REGULAR_RANK_3,
    ADC_REGULAR_RANK_4, ADC_REGULAR_RANK_5
  };
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.NbrOfConversion = 5U;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.SamplingTimeCommon1 = ADC_SAMPLETIME_160CYCLES_5;
  hadc1.Init.SamplingTimeCommon2 = ADC_SAMPLETIME_160CYCLES_5;
  hadc1.Init.TriggerFrequencyMode = ADC_TRIGGER_FREQ_HIGH;
  ADC_Check(HAL_ADC_Init(&hadc1));
  adc_channel.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
  for (uint8_t i = 0; i < 5U; ++i) {
    adc_channel.Rank = ranks[i];
    adc_channel.Channel = channels[i];
    ADC_Check(HAL_ADC_ConfigChannel(&hadc1, &adc_channel));
  }
  /* USER CODE BEGIN ADC1_Init 2 */
  /* Support this checkout before CubeMX regenerates the six-rank init. */
  if (hadc1.Init.NbrOfConversion == 5U) {
    hadc1.Init.NbrOfConversion = 6U;
    SET_BIT(hadc1.Instance->CHSELR, ADC_CHSELR_SQ7);
    ADC_ChannelConfTypeDef channel8 = {
      .Channel = ADC_CHANNEL_8,
      .Rank = ADC_REGULAR_RANK_6,
      .SamplingTime = ADC_SAMPLINGTIME_COMMON_1
    };
    if (HAL_ADC_ConfigChannel(&hadc1, &channel8) != HAL_OK)
      Error_Handler();
  }
  /* USER CODE END ADC1_Init 2 */
}
void HAL_ADC_MspInit(ADC_HandleTypeDef *adc)
{
  GPIO_InitTypeDef gpio = {0};
  if (adc->Instance != ADC1) return;
  __HAL_RCC_ADC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  gpio.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_6 | GPIO_PIN_7;
  gpio.Mode = GPIO_MODE_ANALOG;
  gpio.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &gpio);
  hdma_adc1.Instance = DMA1_Channel1;
  hdma_adc1.Init.Request = DMA_REQUEST_ADC1;
  hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;
  hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
  hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
  hdma_adc1.Init.Mode = DMA_CIRCULAR;
  hdma_adc1.Init.Priority = DMA_PRIORITY_HIGH;
  ADC_Check(HAL_DMA_Init(&hdma_adc1));
  __HAL_LINKDMA(adc, DMA_Handle, hdma_adc1);
  HAL_NVIC_SetPriority(ADC1_COMP_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(ADC1_COMP_IRQn);
  /* USER CODE BEGIN ADC1_MspInit 1 */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  GPIO_InitTypeDef pb0 = {
    .Pin = GPIO_PIN_0,
    .Mode = GPIO_MODE_ANALOG,
    .Pull = GPIO_NOPULL
  };
  HAL_GPIO_Init(GPIOB, &pb0);
  /* USER CODE END ADC1_MspInit 1 */
}
/* USER CODE BEGIN 1 */
/* USER CODE END 1 */
