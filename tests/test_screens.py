#!/usr/bin/env python3
"""Render actual fonts/screens/SSD1306 code into a simulated OLED framebuffer."""
import pathlib
import shutil
import subprocess
import tempfile
root = pathlib.Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as tmp:
    out = pathlib.Path(tmp)
    for name in ('screens', 'ssd1306', 'fonts'):
        for folder, ext in [('Src', 'c'), ('Inc', 'h')]:
            shutil.copy(root/'Core'/folder/(name+'.'+ext), out/(name+'.'+ext))
    shutil.copy(root/'Core/Inc/radio.h',out/'radio.h')
    (out/'main.h').write_text('''#ifndef MOCK_MAIN_H
#define MOCK_MAIN_H
#include <stdint.h>
#define MIDDLE(start,bg,width) ((start)+((bg)-(width))/2)
void HAL_Delay(uint32_t);
#endif
''')
    (out/'i2c.h').write_text('''#include "main.h"
uint8_t SSD1306_WriteCommand(uint16_t, uint8_t);
uint8_t SSD1306_WriteData(uint16_t, uint8_t *, uint16_t);
''')
    (out/'test.c').write_text('''
#include "screens.h"
#include "ssd1306.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static uint8_t frame[1024], frames[4][1024], page, error;
static uint16_t address=0x78;
static unsigned delays, writes;
void HAL_Delay(uint32_t ms) { (void)ms; ++delays; }
const char *Radio_Name(void) { return "Radio Krakow"; }
uint16_t Radio_Frequency(void) { return 10160; }
uint16_t Radio_Divider(void) { return 3179; }
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
int main(void) {
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
  puts("PASS: four distinct frames, repeatability/color state, OLED addresses, FM errors, no delays");
}
''')
    subprocess.run(['cc','-std=gnu11','-Wall','-Wextra','-Werror',str(out/'screens.c'),str(out/'ssd1306.c'),str(out/'fonts.c'),str(out/'test.c'),'-o',str(out/'test')],check=True)
    subprocess.run([str(out/'test')],check=True)
