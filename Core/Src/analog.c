#include "analog.h"
#include "adc.h"
#include "dac.h"

/* Twenty complete scans, split into two DMA halves. Each half must end
 * on a scan boundary so callbacks never publish a partially updated scan. */
#define ADC_DMA_SCAN_COUNT 20U
#define ADC_DMA_SAMPLE_COUNT (ADC_INPUT_COUNT * ADC_DMA_SCAN_COUNT)
#define ADC_DMA_HALF_SAMPLE_COUNT (ADC_DMA_SAMPLE_COUNT / 2U)
_Static_assert(ADC_DMA_SCAN_COUNT % 2U == 0U, "DMA halves must contain complete scans");
_Alignas(uint32_t) static volatile uint16_t adc_dma[ADC_DMA_SAMPLE_COUNT];
static volatile uint16_t adc_latest[ADC_INPUT_COUNT];
static volatile uint8_t adc_ready;
static volatile uint8_t adc_fault;

static void Check(HAL_StatusTypeDef status)
{
  if (status != HAL_OK) Error_Handler();
}

void Analog_Init(void)
{
  Check(Analog_SetDAC(0, 0));
  Check(Analog_SetDAC(1, 0));
  Check(HAL_DAC_Start(&hdac1, DAC_CHANNEL_1));
  Check(HAL_DAC_Start(&hdac1, DAC_CHANNEL_2));
  Check(HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_dma, ADC_DMA_SAMPLE_COUNT));
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
  if (adc == &hadc1) Publish(ADC_DMA_HALF_SAMPLE_COUNT - ADC_INPUT_COUNT);
}
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *adc)
{
  if (adc == &hadc1) Publish(ADC_DMA_SAMPLE_COUNT - ADC_INPUT_COUNT);
}
void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *adc)
{
  if (adc == &hadc1) adc_fault = 1;
}
