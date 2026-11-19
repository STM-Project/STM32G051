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

/* USER CODE BEGIN CALIBRATION */
static uint8_t StartADC(void)
{
  const uint32_t settings = ADC_CFGR1_DMAEN | ADC_CFGR1_DMACFG | ADC_CFGR1_AUTOFF;
  uint32_t saved_settings = hadc1.Instance->CFGR1 & settings;

  uint8_t failed = HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK ? ANALOG_CAL_ADC1 : 0U;
  if (failed) {
    /* Some HAL timeout paths leave calibration active or DMA settings cleared. */
    const uint32_t busy = ADC_CR_ADCAL | ADC_CR_ADEN | ADC_CR_ADDIS |
                          ADC_CR_ADSTART | ADC_CR_ADSTP;
    if ((hadc1.Instance->CR & busy) != 0U) {
      adc_fault = 1;
      return failed;
    }
    MODIFY_REG(hadc1.Instance->CFGR1, settings, saved_settings);
  }

  if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_dma, ADC_DMA_SAMPLE_COUNT) != HAL_OK)
    adc_fault = 1;
  return failed;
}

static uint8_t CalibrateDAC(void)
{
  /* HAL trims the output buffer with a fixed number of steps. A rejected
   * calibration leaves the original factory trim in place for that channel. */
  DAC_ChannelConfTypeDef trim = {0};
  uint8_t failed = 0;
  if (HAL_DACEx_SelfCalibrate(&hdac1, &trim, DAC_CHANNEL_1) != HAL_OK)
    failed |= ANALOG_CAL_DAC_CH1;
  if (HAL_DACEx_SelfCalibrate(&hdac1, &trim, DAC_CHANNEL_2) != HAL_OK)
    failed |= ANALOG_CAL_DAC_CH2;
  return failed;
}
/* USER CODE END CALIBRATION */

uint8_t Analog_Init(void)
{
  /* USER CODE BEGIN ANALOG_STARTUP */
  uint8_t failed = CalibrateDAC();
  Check(Analog_SetDAC(0, 0));
  Check(Analog_SetDAC(1, 0));
  Check(HAL_DAC_Start(&hdac1, DAC_CHANNEL_1));
  Check(HAL_DAC_Start(&hdac1, DAC_CHANNEL_2));
  return failed | StartADC();
  /* USER CODE END ANALOG_STARTUP */
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
