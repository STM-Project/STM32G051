/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "adc.h"
#include "dac.h"
#include "dma.h"
#include "gpio.h"
#include "i2c.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "gpio.h"
#include "i2c.h"
#include "analog.h"
#include "stm32g0xx_it.h"
#include "ssd1306.h"
#include "screens.h"
#include "radio.h"
#include "pcf8575.h"
#include "mcp4728.h"
#include <stdio.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DAC_DISPLAY_REFRESH_MS 125U
#define DAC_RETRY_MS 250U
#define OLED_RETRY_MS 250U
#define I2C_WARNING_MS 2000U
#define FACE_FRAME_MS 250U
#define FACE_DURATION_MS 10000U
#define OLED_PAGE_COUNT (SSD1306_HEIGHT / 8U)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static uint16_t dac_value[DAC_VALUE_COUNT];
static uint8_t external_dac_pending = (1U << MCP4728_CHANNEL_COUNT) - 1U;
static uint8_t external_dac_error;
static uint32_t external_dac_retry_tick;
static uint8_t selected_dac;
static uint8_t oled_ready;
static uint16_t oled_address;
static uint32_t oled_retry_tick;
static uint8_t i2c_errors_pending;
typedef enum { SCREEN_ADC, SCREEN_FM, SCREEN_DEMO0, SCREEN_DEMO1,
               SCREEN_DEMO2, SCREEN_DEMO3, SCREEN_INTRO, SCREEN_FACE, SCREEN_PCF } Screen;
static Screen screen;
static uint8_t display_dirty;
static uint8_t dac_display_dirty;
static uint16_t adc_values[ADC_INPUT_COUNT];
static uint8_t adc_valid;
static uint32_t pulse_hz[2];
static uint32_t face_started;
static uint8_t face_frame;
static uint16_t pcf_values[2] = {0xFFFFU, 0xFFFFU};
static uint8_t pcf_byte;
static uint8_t pcf_errors;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void OLED_Failed(void)
{
  oled_ready = 0U;
  oled_retry_tick = HAL_GetTick();
  i2c_errors_pending |= SCREEN_I2C_ERROR_OLED;
}

static void OLED_Init(void)
{
  const uint16_t addresses[] = {0x78, 0x7A};
  for (uint8_t i = 0; i < 2; ++i) {
    if (HAL_I2C_IsDeviceReady(&hi2c2, addresses[i], 2, 20) == HAL_OK) {
      oled_address = addresses[i];
      ssd1306_SetDevAddr(oled_address);
      oled_ready = (ssd1306_Init() == 0);
      if (oled_ready) return;
      i2c_errors_pending |= SCREEN_I2C_ERROR_OLED;
    }
  }
  OLED_Failed();
}

static void ServicePCFKey(uint8_t keys)
{
  if (keys == 4U) {
    pcf_byte = (pcf_byte + 1U) % 4U;
    return;
  }

  uint8_t chip = pcf_byte / 2U;
  uint8_t shift = pcf_byte % 2U == 0U ? 8U : 0U;
  uint8_t value = (uint8_t)(pcf_values[chip] >> shift);
  switch (keys) {
    case 1: if (value < UINT8_MAX) ++value; break;
    case 2: if (value > 0U) --value; break;
    case 8: value = 0; break;
    default: return;
  }

  uint8_t error_mask = 1U << chip;
  uint16_t byte_mask = (uint16_t)(0xFFU << shift);
  uint16_t next = (pcf_values[chip] & ~byte_mask) | ((uint16_t)value << shift);
  if (next == pcf_values[chip] && (pcf_errors & error_mask) == 0U)
    return;

  if (PCF8575_Write(chip, next) == HAL_OK) {
    pcf_values[chip] = next;
    pcf_errors &= ~error_mask;
  } else {
    pcf_errors |= error_mask;
  }
}

static void DrawDisplay(void)
{
  SSD1306_DispBK(oled_address, NoInvert);
  switch (screen) {
    case SCREEN_ADC: Screen_DrawAnalog(adc_values, adc_valid, dac_value, selected_dac,
                                       pulse_hz); break;
    case SCREEN_FM: Screen_DrawRadio(); break;
    case SCREEN_INTRO: Screen_DrawRadioIntro(); break;
    case SCREEN_FACE: Screen_DrawFace(face_frame); break;
    case SCREEN_PCF: Screen_DrawPCF(pcf_values, pcf_byte, pcf_errors); break;
    default: Screen_DrawExample((uint8_t)(screen - SCREEN_DEMO0)); break;
  }
}

static void SetSelectedDAC(uint16_t value)
{
  dac_value[selected_dac] = value;
  if (selected_dac < INTERNAL_DAC_COUNT) {
    if (Analog_SetDAC(selected_dac, value) != HAL_OK)
      Error_Handler();
  } else {
    external_dac_pending |= 1U << (selected_dac - INTERNAL_DAC_COUNT);
  }
}

static void ServiceExternalDAC(uint32_t now)
{
  if (external_dac_pending == 0U ||
      (external_dac_error && (uint32_t)(now - external_dac_retry_tick) < DAC_RETRY_MS))
    return;

  for (uint8_t channel = 0; channel < MCP4728_CHANNEL_COUNT; ++channel) {
    uint8_t mask = 1U << channel;
    if ((external_dac_pending & mask) == 0U)
      continue;
    if (MCP4728_WriteChannel(channel, dac_value[INTERNAL_DAC_COUNT + channel]) == HAL_OK) {
      external_dac_pending &= ~mask;
    } else {
      external_dac_retry_tick = HAL_GetTick();
      if (!external_dac_error) {
        external_dac_error = 1U;
        i2c_errors_pending |= SCREEN_I2C_ERROR_MCP;
      }
      return;
    }
  }
  external_dac_error = 0U;
}

static void ServiceDisplay(uint32_t now)
{
  static uint8_t page = OLED_PAGE_COUNT;
  static uint8_t frame_errors;
  static uint8_t reported_errors;
  static uint32_t warning_tick;
  static uint32_t display_tick;

  if (!oled_ready) {
    if ((uint32_t)(now - oled_retry_tick) < OLED_RETRY_MS)
      return;
    OLED_Init();
    if (!oled_ready)
      return;
  }

  if ((uint32_t)(now - warning_tick) >= I2C_WARNING_MS)
    reported_errors = 0U;
  uint8_t errors = i2c_errors_pending | reported_errors;
  if (external_dac_error)
    errors |= SCREEN_I2C_ERROR_MCP;

  if (page == OLED_PAGE_COUNT &&
      (errors != frame_errors || i2c_errors_pending ||
       (!errors && (display_dirty ||
                     (screen == SCREEN_ADC && dac_display_dirty &&
                      (uint32_t)(now - display_tick) >= DAC_DISPLAY_REFRESH_MS))))) {
    frame_errors = errors;
    if (errors) {
      SSD1306_DispBK(oled_address, NoInvert);
      Screen_DrawI2CError(errors);
    } else {
      display_dirty = 0U;
      dac_display_dirty = 0U;
      display_tick = now;
      DrawDisplay();
    }
    page = 0U;
  }

  /* Poll keys between OLED pages instead of blocking for a full frame. */
  if (page < OLED_PAGE_COUNT) {
    if (ssd1306_UpdatePage(page) != 0) {
      OLED_Failed();
      page = OLED_PAGE_COUNT;
    } else if (++page == OLED_PAGE_COUNT && (i2c_errors_pending & frame_errors)) {
      reported_errors = frame_errors;
      i2c_errors_pending &= ~frame_errors;
      warning_tick = HAL_GetTick();
    }
  }
}

static void ServiceKeys(void)
{
  uint8_t keys = INPUT_PollPress((screen == SCREEN_ADC || screen == SCREEN_PCF) ? 3U : 0U);
  if (keys == 0) return;
  if (screen == SCREEN_ADC && (keys == 1U || keys == 2U)) {
    uint16_t value = dac_value[selected_dac];
    if (keys == 1U && value < 4095U) {
      ++value;
    } else if (keys == 2U && value > 0U) {
      --value;
    } else {
      return;
    }
    SetSelectedDAC(value);
    dac_display_dirty = 1;
    return;
  }
  display_dirty = 1;
  switch (keys) {
    case 3: screen = SCREEN_ADC; return;   /* pin1+pin2 */
    case 5: screen = SCREEN_FM; Radio_Apply(); return; /* pin1+pin3 */
    case 6: screen = SCREEN_DEMO0; return; /* pin2+pin3 */
    case 9: screen = SCREEN_DEMO1; return; /* pin1+pin4 */
    case 10: screen = SCREEN_DEMO2; return; /* pin2+pin4 */
    case 12: screen = SCREEN_DEMO3; return; /* pin3+pin4 */
    case 7: screen = SCREEN_INTRO; return; /* pin1+pin2+pin3 */
    case 11: /* pin1+pin2+pin4 */
      screen = SCREEN_FACE;
      face_started = HAL_GetTick();
      face_frame = 0;
      return;
    case 13: /* pin1+pin3+pin4 */
      screen = SCREEN_PCF;
      pcf_errors = PCF8575_SetValues(pcf_values[0], pcf_values[1]);
      return;
    default: break;
  }
  if (screen == SCREEN_PCF) { ServicePCFKey(keys); return; }
  if (screen == SCREEN_FM) { Radio_Key(keys); return; }
  if (screen != SCREEN_ADC) return;
  switch (keys) {
    case 4: selected_dac = (selected_dac + 1U) % DAC_VALUE_COUNT; break;
    case 8: SetSelectedDAC(0U); break;
    default: return;
  }
}


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_DAC1_Init();
  MX_I2C2_Init();
  /* USER CODE BEGIN 2 */
  uint8_t calibration_failed = Analog_Init();
  HAL_Delay(200);
  ServiceExternalDAC(HAL_GetTick());
  OLED_Init();
  if (oled_ready && calibration_failed && !i2c_errors_pending) {
    SSD1306_DispBK(oled_address, NoInvert);
    Screen_DrawCalibrationWarning(calibration_failed);
    if (ssd1306_UpdateScreen() == 0)
      HAL_Delay(2000U);
    else
      OLED_Failed();
  }
  Pulse_Init();
  adc_valid = Analog_Read(adc_values);
  display_dirty = 1;


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    ServiceKeys();
    uint32_t now = HAL_GetTick();
    ServiceExternalDAC(now);
    if (Pulse_ReadHz(pulse_hz)) {
      adc_valid = Analog_Read(adc_values);
      if (screen == SCREEN_ADC)
        display_dirty = 1;
    }
    if (screen == SCREEN_FACE) {
      uint32_t elapsed = now - face_started;
      uint8_t frame = (elapsed < FACE_DURATION_MS ? elapsed : FACE_DURATION_MS) / FACE_FRAME_MS;
      if (frame > face_frame) {
        face_frame = frame;
        display_dirty = 1;
      }
    }
    ServiceDisplay(now);

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
