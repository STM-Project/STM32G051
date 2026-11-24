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
    shutil.copy(root/'Core/Inc/mcp4728.h',out/'mcp4728.h')
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
  uint16_t adc[ADC_INPUT_COUNT]={111,222,333,666,777,4095};
  uint16_t dac[DAC_VALUE_COUNT]={0,4095,1234,789,2345,67};
  uint32_t hz[2]={1234,4294967294U};
  for (unsigned selected=0; selected<DAC_VALUE_COUNT; ++selected) {
    SSD1306_DispBK(address,NoInvert);
    Screen_DrawAnalog(adc,1,dac,selected,hz,selected==5);
    ssd1306_UpdateScreen();
    for (unsigned i=0; i<ADC_INPUT_COUNT; ++i)
      assert(pixels((i%3)*42,(i/3)*12,28,10)>0);
    assert(pixels(0,22,126,10)>0 && pixels(0,32,126,10)>0);
    assert((pixels(119,12,7,10)>0)==(selected==5));
    for (unsigned i=0; i<DAC_VALUE_COUNT; ++i) {
      unsigned x=(i%3)*42, y=42+(i/3)*11;
      assert(pixels(x,y,28,10)>0);
      assert(pixels(x+28,y,14,11)==0);
      assert(pixels(x,y+10,28,1)==(i==selected ? 28 : 0));
    }
  }
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
  station_divider=1;
  SSD1306_DispBK(address,NoInvert);
  Screen_DrawRadio();
  ssd1306_UpdateScreen();
  memcpy(frames[2],frame,sizeof(frame));
  const uint16_t ports[2] = {0x8000,0x0001};
  SSD1306_DispBK(address,NoInvert);
  Screen_DrawPCF(ports,3,0);
  ssd1306_UpdateScreen();
  assert(pixels(0,23,22,18)>0 && pixels(27,23,22,18)>0);
  assert(pixels(79,23,22,18)>0 && pixels(106,23,22,18)>0);
  assert(pixels(22,23,5,18)==0 && pixels(101,23,5,18)==0);
  assert(pixels(49,23,30,18)==0);
  assert(pixels(0,2,128,10)>0 && pixels(0,14,128,10)==0);
  assert(pixels(3,49,1,1)==1 && pixels(123,49,1,1)==0);
  assert(pixels(3,60,1,1)==0 && pixels(123,60,1,1)==1);
  /* The filled and hollow markers match Radio FM pixel for pixel. */
  for (unsigned row=0; row<7; ++row)
    for (unsigned x=0; x<16; ++x)
      assert(pixels(x,46+row,1,1)==((frames[2][3*128+x] >> row) & 1));
  assert(pixels(106,41,22,1)==22);
  assert(pixels(0,53,128,4)==0 && pixels(0,63,128,1)>0);
  SSD1306_DispBK(address,NoInvert);
  Screen_DrawPCF(ports,0,1);
  ssd1306_UpdateScreen();
  assert(pixels(0,41,22,1)==22);
  assert(pixels(0,14,128,10)>0);
  /* Verify that all timed face frames fit and the key poses visibly differ. */
  for (unsigned i=0; i<=40; ++i) {
    SSD1306_DispBK(address,NoInvert);
    Screen_DrawFace(i);
    ssd1306_UpdateScreen();
    assert(pixels(28,4,55,57)>0);
    assert(pixels(30,2,50,10)>50); /* Lit strands in the hair. */
    if (i==0) memcpy(frames[0],frame,sizeof(frame));
    if (i==3 || i==6 || i==9 || i==12) {
      const uint8_t *previous = i==3 ? frames[0] : frames[1];
      assert(memcmp(previous,frame,sizeof(frame))!=0);
      memcpy(frames[1],frame,sizeof(frame));
    }
    if (i==14 || i==20) {
      const uint8_t *smile = frames[1]+3*128;
      const uint8_t *eyes = frame+3*128;
      assert((memcmp(smile+37,eyes+37,18)!=0)==(i==14));
      assert((memcmp(smile+56,eyes+56,18)!=0)==(i==20));
    }
    if (i==27) {
      assert(pixels(87,42,11,9)>0);
      for (unsigned y=0; y<64; ++y) for (unsigned x=0; x<86; ++x) {
        if (x>=44 && x<67 && y>=40 && y<54) continue;
        unsigned byte=(y/8)*128+x;
        assert(((frame[byte]^frames[0][byte]) & (1U<<(y%8)))==0);
      }
    }
    if (i==40) assert(pixels(111,18,11,9)>0);
  }
  puts("PASS: six ADC values, two frequencies and two DAC rows, selection, edges, demos, PCF editor, face animation, 24 FM presets/tuning/errors, no clipping");
}
'''
    (out/'test.c').write_text(harness.replace('PRESET_ROWS', presets))
    subprocess.run(['cc','-std=gnu11','-Wall','-Wextra','-Werror',str(out/'screens.c'),str(out/'ssd1306.c'),str(out/'fonts.c'),str(out/'test.c'),'-o',str(out/'test')],check=True)
    subprocess.run([str(out/'test')],check=True)
