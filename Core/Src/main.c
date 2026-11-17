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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "gpio.h"
#include "i2c.h"
#include "analog.h"
#include "ssd1306.h"
#include "screens.h"
#include "radio.h"
#include <stdio.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static uint16_t dac_value[2];
static uint8_t selected_dac;
static uint8_t oled_ready;
static uint16_t oled_address;
typedef enum { SCREEN_ADC, SCREEN_FM, SCREEN_DEMO0, SCREEN_DEMO1,
               SCREEN_DEMO2, SCREEN_DEMO3, SCREEN_INTRO } Screen;
static Screen screen;
static uint8_t display_dirty;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void OLED_Init(void)
{
  const uint16_t addresses[] = {0x78, 0x7A};
  for (uint8_t i = 0; i < 2; ++i) {
    if (HAL_I2C_IsDeviceReady(&hi2c2, addresses[i], 2, 20) == HAL_OK) {
      oled_address = addresses[i];
      ssd1306_SetDevAddr(oled_address);
      oled_ready = (ssd1306_Init() == 0);
      if (oled_ready) return;
    }
  }
}

static void DrawAnalog(void)
{
  uint16_t adc[ADC_INPUT_COUNT];
  const uint8_t channels[ADC_INPUT_COUNT] = {1, 2, 3, 6, 7};
  char line[24];
  uint8_t valid = Analog_Read(adc);
  for (uint8_t i = 0; i < ADC_INPUT_COUNT; ++i) {
    if (valid) snprintf(line, sizeof(line), "ADC%u: %4u", channels[i], adc[i]);
    else snprintf(line, sizeof(line), "ADC%u: ----", channels[i]);
    SSD1306_Txt(0, i * 10, line, Font_7x10, NoInvert);
  }
  snprintf(line, sizeof(line), "%c1:%4u %c2:%4u",
           selected_dac == 0 ? '>' : ' ', dac_value[0],
           selected_dac == 1 ? '>' : ' ', dac_value[1]);
  SSD1306_Txt(0, 54, line, Font_7x10, NoInvert);
}

static void UpdateDisplay(void)
{
  if (!oled_ready) return;
  SSD1306_DispBK(oled_address, NoInvert);
  switch (screen) {
    case SCREEN_ADC: DrawAnalog(); break;
    case SCREEN_FM: Screen_DrawRadio(); break;
    case SCREEN_INTRO: Screen_DrawRadioIntro(); break;
    default: Screen_DrawExample((uint8_t)(screen - SCREEN_DEMO0)); break;
  }
  ssd1306_UpdateScreen();
}

static void ServiceKeys(void)
{
  uint8_t keys = INPUT_PollPress();
  if (keys == 0) return;
  display_dirty = 1;
  switch (keys) {
    case 3: screen = SCREEN_ADC; return;   /* pin1+pin2 */
    case 5: screen = SCREEN_FM; Radio_Apply(); return; /* pin1+pin3 */
    case 6: screen = SCREEN_DEMO0; return; /* pin2+pin3 */
    case 9: screen = SCREEN_DEMO1; return; /* pin1+pin4 */
    case 10: screen = SCREEN_DEMO2; return; /* pin2+pin4 */
    case 12: screen = SCREEN_DEMO3; return; /* pin3+pin4 */
    case 7: screen = SCREEN_INTRO; return; /* pin1+pin2+pin3 */
    default: break;
  }
  if (screen == SCREEN_FM) { Radio_Key(keys); return; }
  if (screen != SCREEN_ADC) return;
  switch (keys) {
    case 1: dac_value[selected_dac] = dac_value[selected_dac] > 4031 ?
                                    4095 : dac_value[selected_dac] + 64; break;
    case 2: dac_value[selected_dac] = dac_value[selected_dac] < 64 ?
                                    0 : dac_value[selected_dac] - 64; break;
    case 4: selected_dac ^= 1; break;
    case 8: dac_value[selected_dac] = 0; break;
    default: return;
  }
  if (Analog_SetDAC(selected_dac, dac_value[selected_dac]) != HAL_OK)
    Error_Handler();
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
  /* USER CODE BEGIN 2 */
  MX_GPIO_Init();
  MX_I2C2_Init();
  Analog_Init();
  HAL_Delay(200);
  OLED_Init();
  uint32_t display_tick = HAL_GetTick();
  UpdateDisplay();


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    ServiceKeys();
    if (display_dirty || (uint32_t)(HAL_GetTick() - display_tick) >= 200U) {
      display_dirty = 0;
      display_tick = HAL_GetTick();
      UpdateDisplay();
    }

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
