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

  snprintf(line, sizeof(line), "PF2:%lu Hz", (unsigned long)hz[0]);
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
