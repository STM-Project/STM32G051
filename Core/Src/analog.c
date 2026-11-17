#include "analog.h"

static ADC_HandleTypeDef hadc1;
static DMA_HandleTypeDef hdma_adc1;
static DAC_HandleTypeDef hdac1;
/* Twenty complete scans, split into two DMA halves. */
static uint16_t adc_dma[100];
static volatile uint16_t adc_latest[ADC_INPUT_COUNT];
static volatile uint8_t adc_ready;
static volatile uint8_t adc_fault;

static void Check(HAL_StatusTypeDef status)
{
  if (status != HAL_OK) Error_Handler();
}

void Analog_Init(void)
{
  GPIO_InitTypeDef gpio = {0};
  ADC_ChannelConfTypeDef adc_channel = {0};
  DAC_ChannelConfTypeDef dac_channel = {0};
  const uint32_t channels[ADC_INPUT_COUNT] = {
    ADC_CHANNEL_1, ADC_CHANNEL_2, ADC_CHANNEL_3, ADC_CHANNEL_6, ADC_CHANNEL_7
  };
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();
  __HAL_RCC_ADC_CLK_ENABLE();
  __HAL_RCC_DAC1_CLK_ENABLE();
  gpio.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 |
             GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
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
  Check(HAL_DMA_Init(&hdma_adc1));
  __HAL_LINKDMA(&hadc1, DMA_Handle, hdma_adc1);
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.ScanConvMode = ADC_SCAN_SEQ_FIXED;
  hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.NbrOfConversion = ADC_INPUT_COUNT;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.SamplingTimeCommon1 = ADC_SAMPLETIME_160CYCLES_5;
  hadc1.Init.SamplingTimeCommon2 = ADC_SAMPLETIME_160CYCLES_5;
  hadc1.Init.TriggerFrequencyMode = ADC_TRIGGER_FREQ_HIGH;
  Check(HAL_ADC_Init(&hadc1));
  adc_channel.Rank = ADC_RANK_CHANNEL_NUMBER;
  adc_channel.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
  for (uint8_t i = 0; i < ADC_INPUT_COUNT; ++i) {
    adc_channel.Channel = channels[i];
    Check(HAL_ADC_ConfigChannel(&hadc1, &adc_channel));
  }
  /* Calibration intentionally skipped while diagnosing the startup stall.
   * ADC conversions still run, with potentially increased offset error. */
  HAL_NVIC_SetPriority(ADC1_COMP_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(ADC1_COMP_IRQn);

  hdac1.Instance = DAC1;
  Check(HAL_DAC_Init(&hdac1));
  dac_channel.DAC_SampleAndHold = DAC_SAMPLEANDHOLD_DISABLE;
  dac_channel.DAC_Trigger = DAC_TRIGGER_NONE;
  dac_channel.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  dac_channel.DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_DISABLE;
  dac_channel.DAC_UserTrimming = DAC_TRIMMING_FACTORY;
  Check(HAL_DAC_ConfigChannel(&hdac1, &dac_channel, DAC_CHANNEL_1));
  Check(HAL_DAC_ConfigChannel(&hdac1, &dac_channel, DAC_CHANNEL_2));
  Check(Analog_SetDAC(0, 0));
  Check(Analog_SetDAC(1, 0));
  Check(HAL_DAC_Start(&hdac1, DAC_CHANNEL_1));
  Check(HAL_DAC_Start(&hdac1, DAC_CHANNEL_2));
  Check(HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_dma, 100));
}

HAL_StatusTypeDef Analog_SetDAC(uint8_t channel, uint16_t value)
{
  if (channel > 1 || value > 4095) return HAL_ERROR;
  return HAL_DAC_SetValue(&hdac1, channel == 0 ? DAC_CHANNEL_1 : DAC_CHANNEL_2,
                          DAC_ALIGN_12B_R, value);
}

uint8_t Analog_Read(uint16_t values[ADC_INPUT_COUNT])
{
  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  uint8_t ready = adc_ready && !adc_fault;
  for (uint8_t i = 0; i < ADC_INPUT_COUNT; ++i) values[i] = adc_latest[i];
  __set_PRIMASK(primask);
  return ready;
}

static void Publish(uint32_t offset)
{
  for (uint8_t i = 0; i < ADC_INPUT_COUNT; ++i) adc_latest[i] = adc_dma[offset + i];
  adc_ready = 1;
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *adc)
{
  if (adc == &hadc1) Publish(45);
}
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *adc)
{
  if (adc == &hadc1) Publish(95);
}
void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *adc)
{
  if (adc == &hadc1) adc_fault = 1;
}
void DMA1_Channel1_IRQHandler(void) { HAL_DMA_IRQHandler(&hdma_adc1); }
void ADC1_COMP_IRQHandler(void) { HAL_ADC_IRQHandler(&hadc1); }
