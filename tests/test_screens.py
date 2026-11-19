#!/usr/bin/env python3
"""Render actual fonts/screens/SSD1306 code into a simulated OLED framebuffer."""
import pathlib
import re
import shutil
import subprocess
import tempfile
root = pathlib.Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as tmp:
    out = pathlib.Path(tmp)
    for name in ('screens', 'ssd1306', 'fonts'):
        for folder, ext in [('Src', 'c'), ('Inc', 'h')]:
            shutil.copy(root/'Core'/folder/(name+'.'+ext), out/(name+'.'+ext))
    # Instrument only the temporary driver copy to catch any clipped output.
    driver = out/'ssd1306.c'
    instrumented = driver.read_text().replace(
        'char ssd1306_WriteChar(char ch,', 'static char DrawCharImpl(char ch,').replace(
        'void ssd1306_DrawPixel(uint8_t x,', 'static void DrawPixelImpl(uint8_t x,')
    instrumented += """
#include <assert.h>
extern unsigned check_bounds;
char ssd1306_WriteChar(char ch, FontDef font, SSD1306_COLOR color) {
  char result = DrawCharImpl(ch, font, color);
  if (check_bounds) assert(result == ch);
  return result;
}
void ssd1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color) {
  if (check_bounds) assert(x < SSD1306_WIDTH && y < SSD1306_HEIGHT);
  DrawPixelImpl(x, y, color);
}
"""
    driver.write_text(instrumented)
    shutil.copy(root/'Core/Inc/radio.h',out/'radio.h')
    shutil.copy(root/'Core/Inc/analog.h',out/'analog.h')
    shutil.copy(root/'Core/Inc/face_frames.h',out/'face_frames.h')
    (out/'main.h').write_text('''#ifndef MOCK_MAIN_H
#define MOCK_MAIN_H
#include <stdint.h>
typedef enum { HAL_OK, HAL_ERROR } HAL_StatusTypeDef;
#define MIDDLE(start,bg,width) ((start)+((bg)-(width))/2)
void HAL_Delay(uint32_t);
#endif
''')
    (out/'i2c.h').write_text('''#include "main.h"
uint8_t SSD1306_WriteCommand(uint16_t, uint8_t);
uint8_t SSD1306_WriteData(uint16_t, uint8_t *, uint16_t);
''')
    stations = re.findall(r'\{"([^"\n]+)", (\d+)\}', (root/'Core/Src/radio.c').read_text())
    assert len(stations) == 24
    presets = ','.join('{"'+name+'",'+freq+'}' for name, freq in stations)
    harness = '''
#include "screens.h"
#include "ssd1306.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static uint8_t frame[1024], frames[4][1024], page, error;
static uint16_t address=0x78;
static unsigned delays, writes;
unsigned check_bounds=1;
static const char *station_name="Radio Krakow";
static uint16_t station_frequency=10160, station_divider=3179;
void HAL_Delay(uint32_t ms) { (void)ms; ++delays; }
const char *Radio_Name(void) { return station_name; }
uint16_t Radio_Frequency(void) { return station_frequency; }
uint16_t Radio_Divider(void) { return station_divider; }
uint8_t Radio_Errors(void) { return error; }
uint8_t SSD1306_WriteCommand(uint16_t addr, uint8_t command) {
  assert(addr==address);
  if (command>=0xB0 && command<=0xB7) page=command-0xB0;
  return 0;
}
uint8_t SSD1306_WriteData(uint16_t addr, uint8_t *data, uint16_t size) {
  assert(addr==address && size==128 && page<8);
  memcpy(frame+page*128,data,size); ++writes;
  return 0;
}
static void draw(unsigned n) {
  writes=0;
  SSD1306_DispBK(address,NoInvert);
  Screen_DrawExample(n);
  ssd1306_UpdateScreen();
  assert(writes==8);
}
static unsigned pixels(unsigned x0, unsigned y0, unsigned w, unsigned h) {
  unsigned count=0;
  for(unsigned y=y0;y<y0+h;y++) for(unsigned x=x0;x<x0+w;x++)
    count += (frame[(y/8)*128+x] >> (y%8)) & 1;
  return count;
}
int main(void) {
  uint16_t adc[ADC_INPUT_COUNT]={111,222,333,666,777,4095}, dac[2]={1234,4095};
  uint32_t hz[2]={1234,4294967294U};
  SSD1306_DispBK(address,NoInvert);
  Screen_DrawAnalog(adc,1,dac,0,hz); ssd1306_UpdateScreen();
  for(unsigned i=0;i<ADC_INPUT_COUNT;i++) assert(pixels((i%3)*42,(i/3)*12,28,10)>0);
  assert(pixels(0,24,126,10)>0 && pixels(0,34,126,10)>0);
  assert(pixels(7,44,63,10)>0 && pixels(7,54,63,10)>0); /* both DAC rows */
  assert(pixels(0,44,7,10)>0 && pixels(0,54,7,10)==0); /* selected DAC1 */
  SSD1306_DispBK(address,NoInvert);
  Screen_DrawAnalog(adc,0,dac,1,hz); ssd1306_UpdateScreen();
  assert(pixels(0,44,7,10)==0 && pixels(0,54,7,10)>0); /* selected DAC2 */
  /* A glyph ending exactly at the right/bottom edges is valid. */
  SSD1306_DispBK(address,NoInvert);
  ssd1306_SetCursor(121,54);
  assert(ssd1306_WriteChar('8',Font_7x10,White)=='8');
  ssd1306_UpdateScreen(); assert(pixels(121,54,7,10)>0);
  check_bounds=0; /* Deliberately out-of-bounds glyph tests. */
  ssd1306_SetCursor(122,54); assert(ssd1306_WriteChar('8',Font_7x10,White)==0);
  ssd1306_SetCursor(121,55); assert(ssd1306_WriteChar('8',Font_7x10,White)==0);
  check_bounds=1;

  for(unsigned i=0;i<4;i++) {
    draw(i); memcpy(frames[i],frame,sizeof(frame));
    draw(i); assert(memcmp(frames[i],frame,sizeof(frame))==0);
    for(unsigned j=0;j<i;j++) assert(memcmp(frames[j],frame,sizeof(frame))!=0);
  }
  for(unsigned i=0;i<4;i++) { draw(i); assert(memcmp(frames[i],frame,sizeof(frame))==0); }
  address=0x7A; draw(0); assert(memcmp(frames[0],frame,sizeof(frame))==0);
  SSD1306_DispBK(address,NoInvert); Screen_DrawRadio(); ssd1306_UpdateScreen();
  uint8_t normal[1024]; memcpy(normal,frame,sizeof(frame));
  error=3;
  SSD1306_DispBK(address,NoInvert); Screen_DrawRadio(); ssd1306_UpdateScreen();
  assert(memcmp(normal,frame,sizeof(frame))!=0); /* visible error state */
  error=0;
  SSD1306_DispBK(address,NoInvert); Screen_DrawRadio(); ssd1306_UpdateScreen();
  assert(memcmp(normal,frame,sizeof(frame))==0);
  SSD1306_DispBK(address,NoInvert); Screen_DrawRadioIntro(); ssd1306_UpdateScreen();
  assert(memcmp(normal,frame,sizeof(frame))!=0 && delays==0);

  const struct { const char *name; unsigned frequency; } presets[] = { PRESET_ROWS };
  for(unsigned i=0;i<sizeof(presets)/sizeof(presets[0]);i++) {
    station_name=presets[i].name;
    for(int offset=-100;offset<=100;offset+=100) {
      station_frequency=presets[i].frequency+offset;
      station_divider=(offset==0) ? 0xFFFF : 0;
      for(error=0;error<4;error++) {
        SSD1306_DispBK(address,NoInvert);
        Screen_DrawRadio(); ssd1306_UpdateScreen();
      }
    }
  }
  for (unsigned failed=1; failed<8; ++failed) {
    SSD1306_DispBK(address,NoInvert);
    Screen_DrawCalibrationWarning(failed);
    ssd1306_UpdateScreen();
    assert(pixels(0,2,128,18)>0 && pixels(0,21,128,18)>0);
    assert(pixels(0,42,128,10)>0);
    if ((failed & ANALOG_CAL_ADC1) && (failed & (ANALOG_CAL_DAC_CH1 | ANALOG_CAL_DAC_CH2)))
      assert(pixels(0,53,128,10)>0);
    if (failed==ANALOG_CAL_DAC_CH1) memcpy(frames[0],frame,sizeof(frame));
    if (failed==ANALOG_CAL_DAC_CH2) assert(memcmp(frames[0],frame,sizeof(frame))!=0);
  }
  /* Verify that all timed face frames fit and the key poses visibly differ. */
  for (unsigned i=0; i<=20; ++i) {
    SSD1306_DispBK(address,NoInvert);
    Screen_DrawFace(i);
    ssd1306_UpdateScreen();
    assert(pixels(28,4,55,57)>0);
    assert(pixels(30,2,50,10)>50); /* Lit strands in the hair. */
    if (i==0) memcpy(frames[0],frame,sizeof(frame));
    if (i==4) memcpy(frames[1],frame,sizeof(frame));
    if (i==5) assert(memcmp(frames[1],frame,sizeof(frame))!=0);
    if (i==4 || i==9 || i==14 || i==20)
      assert(memcmp(frames[0],frame,sizeof(frame))!=0);
    if (i==9) assert(pixels(87,41,11,9)>0);
    if (i==20) assert(pixels(107,17,11,9)>0);
  }
  puts("PASS: six ADC values, two frequencies and two DAC rows, selection, edges, demos, face animation, 24 FM presets/tuning/errors, no clipping");
}
'''
    (out/'test.c').write_text(harness.replace('PRESET_ROWS', presets))
    subprocess.run(['cc','-std=gnu11','-Wall','-Wextra','-Werror',str(out/'screens.c'),str(out/'ssd1306.c'),str(out/'fonts.c'),str(out/'test.c'),'-o',str(out/'test')],check=True)
    subprocess.run([str(out/'test')],check=True)
