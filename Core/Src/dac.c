#include "dac.h"
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */
static void DAC_Check(HAL_StatusTypeDef status)
{
  if (status != HAL_OK) Error_Handler();
}
DAC_HandleTypeDef hdac1;
void MX_DAC1_Init(void)
{
  /* USER CODE BEGIN DAC1_Init 0 */
  /* USER CODE END DAC1_Init 0 */
  DAC_ChannelConfTypeDef dac_channel = {0};
  hdac1.Instance = DAC1;
  DAC_Check(HAL_DAC_Init(&hdac1));
  dac_channel.DAC_SampleAndHold = DAC_SAMPLEANDHOLD_DISABLE;
  dac_channel.DAC_Trigger = DAC_TRIGGER_NONE;
  dac_channel.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  dac_channel.DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_DISABLE;
  dac_channel.DAC_UserTrimming = DAC_TRIMMING_FACTORY;
  DAC_Check(HAL_DAC_ConfigChannel(&hdac1, &dac_channel, DAC_CHANNEL_1));
  DAC_Check(HAL_DAC_ConfigChannel(&hdac1, &dac_channel, DAC_CHANNEL_2));
  /* USER CODE BEGIN DAC1_Init 2 */
  /* USER CODE END DAC1_Init 2 */
}
void HAL_DAC_MspInit(DAC_HandleTypeDef *dac)
{
  if (dac->Instance != DAC1) return;
  GPIO_InitTypeDef gpio = {0};
  __HAL_RCC_DAC1_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  gpio.Pin = GPIO_PIN_4 | GPIO_PIN_5;
  gpio.Mode = GPIO_MODE_ANALOG;
  gpio.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &gpio);
  /* USER CODE BEGIN DAC1_MspInit 1 */
  /* USER CODE END DAC1_MspInit 1 */
}
/* USER CODE BEGIN 1 */
/* USER CODE END 1 */
