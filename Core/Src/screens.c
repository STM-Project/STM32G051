#include "screens.h"
#include "ssd1306.h"
#include "radio.h"
#include <stdio.h>

/* Four individual frames from G031 EXAMPLE_DrawTxt, now on one display.
 * The caller clears/selects the display and flushes it; no delays or I2C
 * address changes here, so all frames work with either OLED address. */
void Screen_DrawExample(uint8_t example)
{
  switch (example) {
    case 0:
      SSD1306_Txt(0,2,"Radio krakow",Font_7x10,Invert);
      SSD1306_Txt(0,25,"*!a@c&",Font_7x10,NoInvert);
      SSD1306_Txt(0,45,"?ay|W",Font_7x10,Invert);
      SSD1306_Txt(70,45,"1+2=3",Font_7x10,NoInvert);
      SSD1306_roundRect(100,0,10,10);
      SSD1306_rect(100,20,10,10);
      break;
    case 1:
      SSD1306_Txt(0,15,"1234+5!",Font_7x10,NoInvert);
      SSD1306_Txt(0,35,"1234+5!",Font_7x10,Invert);
      SSD1306_roundRect(100,0,10,10);
      SSD1306_rect(100,20,10,10);
      break;
    case 2:
      ssd1306_InvertColors();
      SSD1306_Txt(0,15,"Rafa",Font_7x10,Invert);
      SSD1306_Txt(0,35,"Rafa",Font_7x10,NoInvert);
      ssd1306_InvertColors();
      SSD1306_roundRect(100,20,10,10);
      break;
    case 3:
      ssd1306_InvertColors();
      ssd1306_Clr();
      SSD1306_Txt(0,15,"!@#I|$%^&*()_+yX",Font_7x10,NoInvert);
      SSD1306_Txt(0,35,"!@#I|$%^&*()_+yX",Font_7x10,Invert);
      SSD1306_roundRect(100,0,10,10);
      ssd1306_InvertColors(); /* Restore logical color state for other screens. */
      break;
    default: break;
  }
}

void Screen_DrawRadioIntro(void)
{
  SSD1306_Txt(0,0,"Radio",Font_16x26,NoInvert);
  SSD1306_Txt(SSD1306_posX()+5,0,"FM",Font_16x26,NoInvert);
  SSD1306_TxtMiddX(0,32,"Synteza",Font_7x10,NoInvert);
  SSD1306_TxtMiddX(0,45,"czestotliwosci",Font_7x10,NoInvert);
}

void Screen_DrawRadio(void)
{
  char line[24];
  uint16_t frequency = Radio_Frequency();
  uint16_t divider = Radio_Divider();
  SSD1306_Txt(0,0,Radio_Name(),Font_7x10,NoInvert);
  if (Radio_Errors()) {
    snprintf(line,sizeof(line),"PCF ERR:%s%s",
             (Radio_Errors() & 1) ? " 20" : "",
             (Radio_Errors() & 2) ? " 21" : "");
    SSD1306_Txt(0,12,line,Font_7x10,NoInvert);
  } else {
    snprintf(line,sizeof(line),"%04X",(unsigned)RADIO_STEP_DIVIDER);
    SSD1306_Txt(0,12,line,Font_7x10,NoInvert);
    snprintf(line,sizeof(line),"%04X",(unsigned)divider);
    SSD1306_Txt(64,12,line,Font_7x10,NoInvert);
  }
  for (uint8_t i = 0; i < 16; ++i) {
    if (divider & (1U << i)) SSD1306_roundRect(8*i,24,7,7);
    else SSD1306_roundFrame(8*i,24,7,7);
  }
  snprintf(line,sizeof(line),"%3u.%02u",frequency / 100U,frequency % 100U);
  SSD1306_Txt(0,37,line,Font_16x26,NoInvert);
  SSD1306_Txt(106,45,"MHz",Font_7x10,NoInvert);
}

/* USER CODE BEGIN MEASUREMENTS */
void Screen_DrawCalibrationWarning(uint8_t failed)
{
  const uint8_t adc = failed & ANALOG_CAL_ADC1;
  const uint8_t dac = failed & (ANALOG_CAL_DAC_CH1 | ANALOG_CAL_DAC_CH2);
  SSD1306_TxtMiddBK(0, 2, SSD1306_WIDTH, 18, "BRAK", Font_11x18, NoInvert);
  SSD1306_TxtMiddBK(0, 21, SSD1306_WIDTH, 18, "KALIBRACJI", Font_11x18, NoInvert);
  if (adc)
    SSD1306_TxtMiddBK(0, 42, SSD1306_WIDTH, 10, "ADC1 IN1,2,3,6,7,8", Font_7x10, NoInvert);
  if (dac) {
    const char *detail = dac == ANALOG_CAL_DAC_CH1 ? "DAC CH 1" :
                         dac == ANALOG_CAL_DAC_CH2 ? "DAC CH 2" : "DAC CH 1,2";
    SSD1306_TxtMiddBK(0, adc ? 53 : 42, SSD1306_WIDTH, 10, detail, Font_7x10, NoInvert);
  }
}


void Screen_DrawAnalog(const uint16_t adc[ADC_INPUT_COUNT], uint8_t valid,
                       const uint16_t dac[DAC_VALUE_COUNT], uint8_t selected,
                       const uint32_t hz[2], uint8_t external_dac_error)
{
  char line[18];
  /* The text driver also clears two rows above each glyph. Draw bottom-up. */
  for (uint8_t i = DAC_VALUE_COUNT; i > 0U;) {
    --i;
    uint8_t x = (i % 3U) * 42U;
    uint8_t y = 42U + (i / 3U) * 11U;
    snprintf(line, sizeof(line), "%4u", (unsigned)dac[i]);
    SSD1306_Txt(x, y, line, Font_7x10, NoInvert);
  }
  SSD1306_rect((selected % 3U) * 42U, 52U + (selected / 3U) * 11U,
               4U * Font_7x10.FontWidth, 1U);

  snprintf(line, sizeof(line), "PB3:%lu Hz", (unsigned long)hz[1]);
  SSD1306_Txt(0, 32, line, Font_7x10, NoInvert);
  snprintf(line, sizeof(line), "PA0:%lu Hz", (unsigned long)hz[0]);
  SSD1306_Txt(0, 22, line, Font_7x10, NoInvert);

  for (uint8_t i = 0; i < ADC_INPUT_COUNT; ++i) {
    if (valid)
      snprintf(line, sizeof(line), "%4u", (unsigned)adc[i]);
    else
      snprintf(line, sizeof(line), "----");
    SSD1306_Txt((i % 3U) * 42U, (i / 3U) * 12U, line, Font_7x10, NoInvert);
  }
  if (external_dac_error)
    SSD1306_Txt(119, 12, "!", Font_7x10, NoInvert);
}
/* USER CODE END MEASUREMENTS */

/* USER CODE BEGIN FACE */
#include "face_frames.h"

static void FaceDrawPortrait(const uint8_t portrait[FACE_HEIGHT][FACE_ROW_BYTES])
{
  for (uint8_t y = 0; y < FACE_HEIGHT; ++y)
    for (uint8_t x = 0; x < FACE_ROW_BYTES * 8U; ++x)
      if ((portrait[y][x / 8U] & (0x80U >> (x % 8U))) != 0U)
        ssd1306_Pixel(FACE_LEFT + x, y);
}

static void FaceHeart(uint8_t x, uint8_t y)
{
  static const uint16_t rows[] = {
    0x186, 0x3CF, 0x7FF, 0x7FF, 0x3FE, 0x1FC, 0x0F8, 0x070, 0x020
  };
  for (uint8_t row = 0; row < sizeof(rows) / sizeof(rows[0]); ++row)
    for (uint8_t col = 0; col < 11U; ++col)
      if ((rows[row] >> col) & 1U)
        ssd1306_Pixel(x + col, y + row);
}

void Screen_DrawFace(uint8_t frame)
{
  if (frame < 3U) {
    FaceDrawPortrait(face_neutral);
  } else if (frame < 6U) {
    FaceDrawPortrait(face_smile_small);
  } else if (frame < 9U) {
    FaceDrawPortrait(face_smile_medium);
  } else if (frame < 12U) {
    FaceDrawPortrait(face_smile_wide);
  } else if (frame >= 14U && frame < 16U) {
    FaceDrawPortrait(face_wink_left);
  } else if (frame >= 20U && frame < 22U) {
    FaceDrawPortrait(face_wink_right);
  } else if ((frame >= 24U && frame < 27U) || (frame >= 34U && frame < 36U)) {
    FaceDrawPortrait(face_kiss_prepare);
  } else if (frame >= 27U && frame < 34U) {
    FaceDrawPortrait(face_kiss);
    uint8_t step = frame - 27U;
    FaceHeart(87U + step * 4U, 42U - step * 4U);
  } else {
    FaceDrawPortrait(face_smile);
    if (frame >= 36U)
      FaceHeart(111, frame < 40U ? 22U - (frame - 36U) : 18U);
  }
}
/* USER CODE END FACE */

/* USER CODE BEGIN PCF */
void Screen_DrawPCF(const uint16_t values[2], uint8_t selected_byte,
                    uint8_t errors)
{
  const uint8_t byte_spacing = 27U;
  char hex[3];
  SSD1306_TxtMiddBK(0, 2, SSD1306_WIDTH, 10, "Syntezer PLL", Font_7x10, NoInvert);
  if (errors) {
    const char *message = errors == 3U ? "ERR 20/21" :
                          errors == 1U ? "ERR 20" : "ERR 21";
    SSD1306_Txt(0, 14, message, Font_7x10, NoInvert);
  }
  for (uint8_t chip = 0; chip < 2; ++chip) {
    uint8_t x = chip == 0 ? 0 : 79;
    uint8_t y = chip == 0 ? 46 : 57;
    snprintf(hex, sizeof(hex), "%02X", (unsigned)(values[chip] >> 8));
    SSD1306_Txt(x, 23, hex, Font_11x18, NoInvert);
    snprintf(hex, sizeof(hex), "%02X", (unsigned)(values[chip] & 0xFFU));
    SSD1306_Txt(x + byte_spacing, 23, hex, Font_11x18, NoInvert);
    if (chip == selected_byte / 2U)
      SSD1306_rect(x + byte_spacing * (selected_byte % 2U), 41, 22, 1);

    for (uint8_t col = 0; col < 16; ++col) {
      if ((values[chip] & (1U << (15U - col))) != 0U)
        SSD1306_roundRect(8U * col, y, 7, 7);
      else
        SSD1306_roundFrame(8U * col, y, 7, 7);
    }
  }
}
/* USER CODE END PCF */
