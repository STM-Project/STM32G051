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
void Screen_DrawAnalog(const uint16_t adc[ADC_INPUT_COUNT], uint8_t valid,
                       const uint16_t dac[2], uint8_t selected, const uint32_t hz[2])
{
  char line[18];
  for (uint8_t i = 0; i < ADC_INPUT_COUNT; ++i) {
    if (valid)
      snprintf(line, sizeof(line), "%4u", (unsigned)adc[i]);
    else
      snprintf(line, sizeof(line), "----");
    SSD1306_Txt((i % 3U) * 42U, (i / 3U) * 12U, line, Font_7x10, NoInvert);
  }

  snprintf(line, sizeof(line), "PA0:%lu Hz", (unsigned long)hz[0]);
  SSD1306_Txt(0, 24, line, Font_7x10, NoInvert);
  snprintf(line, sizeof(line), "PB3:%lu Hz", (unsigned long)hz[1]);
  SSD1306_Txt(0, 34, line, Font_7x10, NoInvert);

  for (uint8_t i = 0; i < 2; ++i) {
    snprintf(line, sizeof(line), "%cDAC%u:%4u", selected == i ? '>' : ' ',
             (unsigned)(i + 1U), (unsigned)dac[i]);
    SSD1306_Txt(0, 44U + i * 10U, line, Font_7x10, NoInvert);
  }
}
/* USER CODE END MEASUREMENTS */

/* USER CODE BEGIN FACE */
static void FaceEllipse(int cx, int cy, int rx, int ry, uint8_t filled)
{
  int32_t rx2 = (int32_t)rx * rx;
  int32_t ry2 = (int32_t)ry * ry;
  int x = rx;
  for (int y = 0; y <= ry; ++y) {
    int end = x;
    while ((int32_t)x * x * ry2 + (int32_t)y * y * rx2 > rx2 * ry2)
      --x;
    /* Join the outline to the previous row where the ellipse tapers. */
    int start = filled ? 0 : x;
    if (filled) end = x;
    for (int dx = start; dx <= end; ++dx) {
      ssd1306_Pixel(cx - dx, cy - y);
      ssd1306_Pixel(cx + dx, cy - y);
      ssd1306_Pixel(cx - dx, cy + y);
      ssd1306_Pixel(cx + dx, cy + y);
    }
  }
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
  FaceEllipse(55, 32, 27, 28, 0); /* Face outline. */
  FaceEllipse(27, 32, 3, 6, 0);
  FaceEllipse(83, 32, 3, 6, 0);
  for (int dx = -18; dx <= 17; ++dx) {
    int y = 11 + (dx + 4) * (dx + 4) / 76;
    SSD1306_rect(55 + dx, y, 1, 3);
  }
  SSD1306_rect(38, 22, 11, 2);
  SSD1306_rect(62, 22, 11, 2);
  FaceEllipse(43, 29, 4, 5, 1);
  if (frame >= 14U && frame < 17U) {
    SSD1306_rect(62, 31, 11, 2);
    ssd1306_Pixel(62, 30);
    ssd1306_Pixel(72, 30);
  } else {
    FaceEllipse(67, 29, 4, 5, 1);
  }
  SSD1306_rect(54, 31, 2, 7);
  SSD1306_rect(54, 37, 5, 2);

  if (frame >= 9U && frame < 14U) {
    FaceEllipse(55, 46, 4, 6, 0); /* Puckered lips and a floating heart. */
    FaceHeart(87U + (frame - 9U) * 5U, 41U - (frame - 9U) * 6U);
  } else if (frame >= 4U && frame < 9U) {
    FaceEllipse(55, 46, 8, 2U + (frame % 3U) * 2U, 0);
  } else {
    for (int dx = -12; dx <= 12; ++dx) {
      int y = 49 - dx * dx / 22;
      SSD1306_rect(55 + dx, y, 1, 2);
    }
    if (frame >= 17U)
      FaceHeart(107, frame < 20U ? 23U - (frame - 17U) * 2U : 17U);
  }
}
/* USER CODE END FACE */
