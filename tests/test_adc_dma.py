#!/usr/bin/env python3
"""Exercise real ADC init and DMA callbacks with a host HAL mock (no hardware)."""
import pathlib
import re
import shutil
import subprocess
import tempfile
root = pathlib.Path(__file__).resolve().parents[1]
source = '\n'.join((root/'Core/Src'/name).read_text() for name in ('analog.c','adc.c','dac.c','dma.c'))
with tempfile.TemporaryDirectory() as tmp:
    out = pathlib.Path(tmp)
    shutil.copy(root/'Core/Src/analog.c', out/'analog.c')
    shutil.copy(root/'Core/Inc/analog.h', out/'analog.h')
    for name in ('adc','dac','dma'):
        shutil.copy(root/'Core/Src'/(name+'.c'), out/(name+'.c'))
        shutil.copy(root/'Core/Inc'/(name+'.h'), out/(name+'.h'))
    # Opaque HAL constants: unique values let assertions distinguish settings.
    constants = sorted(set(re.findall(r'\b(?:ADC|DAC|DMA|GPIO)_[A-Z][A-Z_0-9]*\b', source)) -
                       {'ADC_INPUT_COUNT', 'ADC_DMA_SCAN_COUNT', 'ADC_DMA_SAMPLE_COUNT', 'ADC_DMA_HALF_SAMPLE_COUNT'})
    register_bits = [name for name in constants if name.startswith(('ADC_CR_', 'ADC_CFGR1_'))]
    defines = '\n'.join(f'#define {name} {i+1}U' for i, name in enumerate(constants)
                        if name not in register_bits)
    defines += '\n' + '\n'.join(f'#define {name} (1U << {i})'
                                 for i, name in enumerate(register_bits))
    fields = '''Request Direction PeriphInc MemInc PeriphDataAlignment MemDataAlignment Mode Priority
ClockPrescaler Resolution DataAlign ScanConvMode EOCSelection ContinuousConvMode NbrOfConversion
DiscontinuousConvMode ExternalTrigConv ExternalTrigConvEdge DMAContinuousRequests Overrun
SamplingTimeCommon1 SamplingTimeCommon2 TriggerFrequencyMode'''.split()
    (out/'main.h').write_text('''#ifndef MOCK_MAIN_H
#define MOCK_MAIN_H
#include <stdint.h>
#include <stddef.h>
typedef enum { HAL_OK, HAL_ERROR } HAL_StatusTypeDef;
#define ENABLE 1U
#define DISABLE 0U
typedef struct { uint32_t CHSELR, CR, CFGR1; } MockADC;
static MockADC adc_registers;
#define ADC1 (&adc_registers)
#define SET_BIT(reg, bit) ((reg) |= (bit))
#define MODIFY_REG(reg, clear, set) ((reg) = ((reg) & ~(clear)) | (set))
#define DAC1 ((void *)2)
#define DMA1_Channel1 ((void *)3)
#define GPIOA ((void *)4)
#define GPIOB ((void *)5)
#define DMA1_Channel1_IRQn 9
#define ADC1_COMP_IRQn 12
''' + defines + '\ntypedef struct { '+ ''.join('uint32_t '+f+';' for f in fields) + ''' } Config;
typedef struct DMA_HandleTypeDef { void *Instance; Config Init; void *Parent; } DMA_HandleTypeDef;
typedef struct { MockADC *Instance; Config Init; DMA_HandleTypeDef *DMA_Handle; } ADC_HandleTypeDef;
typedef struct { void *Instance; } DAC_HandleTypeDef;
typedef struct { uint32_t Pin, Mode, Pull; } GPIO_InitTypeDef;
typedef struct { uint32_t Rank, SamplingTime, Channel; } ADC_ChannelConfTypeDef;
typedef struct { uint32_t DAC_SampleAndHold, DAC_Trigger, DAC_OutputBuffer,
DAC_ConnectOnChipPeripheral, DAC_UserTrimming; } DAC_ChannelConfTypeDef;
#define __HAL_RCC_GPIOA_CLK_ENABLE() ((void)0)
#define __HAL_RCC_GPIOB_CLK_ENABLE() ((void)0)
#define __HAL_RCC_DMA1_CLK_ENABLE() ((void)0)
#define __HAL_RCC_ADC_CLK_ENABLE() ((void)0)
#define __HAL_RCC_DAC1_CLK_ENABLE() ((void)0)
#define __HAL_LINKDMA(h,f,d) ((h)->f=&(d), (d).Parent=(h))
void Error_Handler(void);
void HAL_GPIO_Init(void *, GPIO_InitTypeDef *);
HAL_StatusTypeDef HAL_DMA_Init(DMA_HandleTypeDef *);
void HAL_NVIC_SetPriority(int, int, int);
void HAL_NVIC_EnableIRQ(int);
HAL_StatusTypeDef HAL_ADC_Init(ADC_HandleTypeDef *);
HAL_StatusTypeDef HAL_ADC_ConfigChannel(ADC_HandleTypeDef *, ADC_ChannelConfTypeDef *);
HAL_StatusTypeDef HAL_DAC_Init(DAC_HandleTypeDef *);
HAL_StatusTypeDef HAL_DAC_ConfigChannel(DAC_HandleTypeDef *, DAC_ChannelConfTypeDef *, uint32_t);
HAL_StatusTypeDef HAL_DAC_Start(DAC_HandleTypeDef *, uint32_t);
HAL_StatusTypeDef HAL_DAC_SetValue(DAC_HandleTypeDef *, uint32_t, uint32_t, uint32_t);
HAL_StatusTypeDef HAL_ADC_Start_DMA(ADC_HandleTypeDef *, uint32_t *, uint32_t);
HAL_StatusTypeDef HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef *);
HAL_StatusTypeDef HAL_DACEx_SelfCalibrate(DAC_HandleTypeDef *, DAC_ChannelConfTypeDef *, uint32_t);
void HAL_DMA_IRQHandler(DMA_HandleTypeDef *);
void HAL_ADC_IRQHandler(ADC_HandleTypeDef *);
uint32_t __get_PRIMASK(void);
void __disable_irq(void);
void __set_PRIMASK(uint32_t);
#endif
''')
    (out/'test.c').write_text('''
#include "adc.c"
#include "dac.c"
#include "dma.c"
#include "analog.c"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
static unsigned configured, dma_irq, adc_irq, started, analog_pb0;
static uint32_t mask;
static unsigned scenario, adc_calibrated, dac_calibrated, dac_started;
void Error_Handler(void) { abort(); }
void HAL_GPIO_Init(void *p, GPIO_InitTypeDef *g) {
  if (p == GPIOB) {
    assert(g->Pin == GPIO_PIN_0 && g->Mode == GPIO_MODE_ANALOG && g->Pull == GPIO_NOPULL);
    analog_pb0 = 1;
  }
}
HAL_StatusTypeDef HAL_DMA_Init(DMA_HandleTypeDef *d) {
  assert(d->Instance==DMA1_Channel1 && d->Init.Request==DMA_REQUEST_ADC1);
  assert(d->Init.Mode==DMA_CIRCULAR && d->Init.Direction==DMA_PERIPH_TO_MEMORY);
  assert(d->Init.MemInc==DMA_MINC_ENABLE && d->Init.PeriphInc==DMA_PINC_DISABLE);
  assert(d->Init.MemDataAlignment==DMA_MDATAALIGN_HALFWORD);
  assert(d->Init.PeriphDataAlignment==DMA_PDATAALIGN_HALFWORD);
  return HAL_OK;
}
void HAL_NVIC_SetPriority(int irq,int p,int s) { (void)irq; (void)p; (void)s; }
void HAL_NVIC_EnableIRQ(int irq) { if(irq==9) dma_irq=1; if(irq==12) adc_irq=1; }
HAL_StatusTypeDef HAL_ADC_Init(ADC_HandleTypeDef *a) {
  HAL_ADC_MspInit(a);
  a->Instance->CFGR1 = ADC_CFGR1_DMACFG;
  assert(a->DMA_Handle==&hdma_adc1 && a->DMA_Handle->Parent==a);
  assert(a->Init.ContinuousConvMode==ENABLE && a->Init.DMAContinuousRequests==ENABLE);
  assert(a->Init.ScanConvMode==ADC_SCAN_ENABLE && a->Init.NbrOfConversion==5);
  return HAL_OK;
}
HAL_StatusTypeDef HAL_ADC_ConfigChannel(ADC_HandleTypeDef *a,ADC_ChannelConfTypeDef *c) {
  const uint32_t ch[]={ADC_CHANNEL_1,ADC_CHANNEL_2,ADC_CHANNEL_3,ADC_CHANNEL_6,ADC_CHANNEL_7,ADC_CHANNEL_8};
  const uint32_t rank[]={ADC_REGULAR_RANK_1,ADC_REGULAR_RANK_2,ADC_REGULAR_RANK_3,ADC_REGULAR_RANK_4,ADC_REGULAR_RANK_5,ADC_REGULAR_RANK_6};
  assert(a==&hadc1 && configured<ADC_INPUT_COUNT);
  assert(c->Channel==ch[configured] && c->Rank==rank[configured]); ++configured;
  return HAL_OK;
}
HAL_StatusTypeDef HAL_DAC_Init(DAC_HandleTypeDef *d) { HAL_DAC_MspInit(d); return HAL_OK; }
HAL_StatusTypeDef HAL_DAC_ConfigChannel(DAC_HandleTypeDef *d,DAC_ChannelConfTypeDef *c,uint32_t ch) { (void)d;(void)c;(void)ch; return HAL_OK; }
HAL_StatusTypeDef HAL_DACEx_SelfCalibrate(DAC_HandleTypeDef *d,DAC_ChannelConfTypeDef *c,uint32_t ch) {
  assert(d==&hdac1 && c && !dac_started);
  assert(ch==(dac_calibrated==0 ? DAC_CHANNEL_1 : DAC_CHANNEL_2));
  ++dac_calibrated;
  return ((scenario==1 && ch==DAC_CHANNEL_1) ||
          (scenario==4 && ch==DAC_CHANNEL_2)) ? HAL_ERROR : HAL_OK;
}
HAL_StatusTypeDef HAL_DAC_Start(DAC_HandleTypeDef *d,uint32_t ch) {
  assert(d==&hdac1 && dac_calibrated==2);
  assert(ch==(dac_started==0 ? DAC_CHANNEL_1 : DAC_CHANNEL_2));
  ++dac_started;
  return HAL_OK;
}
HAL_StatusTypeDef HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef *a) {
  assert(a==&hadc1 && !started);
  ++adc_calibrated;
  if (scenario==1 || scenario==2) {
    a->Instance->CFGR1 = 0;
    if (scenario==2) a->Instance->CR = ADC_CR_ADCAL;
    return HAL_ERROR;
  }
  return HAL_OK;
}
HAL_StatusTypeDef HAL_DAC_SetValue(DAC_HandleTypeDef *d,uint32_t ch,uint32_t al,uint32_t v) { (void)d;(void)ch;(void)al;(void)v; return HAL_OK; }
HAL_StatusTypeDef HAL_ADC_Start_DMA(ADC_HandleTypeDef *a,uint32_t *buf,uint32_t n) {
  assert(a==&hadc1 && configured==ADC_INPUT_COUNT && dma_irq && adc_irq && analog_pb0);
  assert(a->Init.NbrOfConversion==ADC_INPUT_COUNT && (ADC1->CHSELR & ADC_CHSELR_SQ7));
  assert((void *)buf==(void *)adc_dma && n==120 && ((uintptr_t)buf%4)==0);
  assert(adc_calibrated==1 && (ADC1->CFGR1 & ADC_CFGR1_DMACFG));
  ++started;
  return scenario==3 ? HAL_ERROR : HAL_OK;
}
void HAL_DMA_IRQHandler(DMA_HandleTypeDef *d) { assert(d==&hdma_adc1); }
void HAL_ADC_IRQHandler(ADC_HandleTypeDef *a) { assert(a==&hadc1); }
uint32_t __get_PRIMASK(void) { return mask; }
void __disable_irq(void) { mask=1; }
void __set_PRIMASK(uint32_t m) { mask=m; }
int main(int argc, char **argv) {
  assert(argc==2);
  scenario=(unsigned)atoi(argv[1]);
  uint16_t result[ADC_INPUT_COUNT];
  MX_DMA_Init(); MX_ADC1_Init(); MX_DAC1_Init();
  const uint8_t expected[] = {0, ANALOG_CAL_ADC1 | ANALOG_CAL_DAC_CH1,
                              ANALOG_CAL_ADC1, 0, ANALOG_CAL_DAC_CH2};
  assert(Analog_Init()==expected[scenario]);
  assert(dac_started==2 && adc_calibrated==1 && !Analog_Read(result));
  assert(started==(scenario==2 ? 0 : 1));
  if (scenario==2 || scenario==3) {
    assert(adc_fault);
    puts("PASS: ADC startup fault keeps DAC active and ADC readings unavailable");
    return 0;
  }
  assert(!adc_fault);
  ADC_HandleTypeDef unrelated={0};
  HAL_ADC_ConvCpltCallback(&unrelated); assert(!Analog_Read(result));
  for(unsigned round=0;round<4;round++) {
    for(unsigned i=0;i<60;i++) adc_dma[i]=120*round+i;
    HAL_ADC_ConvHalfCpltCallback(&hadc1);
    assert(Analog_Read(result));
    for(unsigned i=0;i<ADC_INPUT_COUNT;i++) assert(result[i]==120*round+54+i);
    for(unsigned i=60;i<120;i++) adc_dma[i]=120*round+i;
    HAL_ADC_ConvCpltCallback(&hadc1);
    mask=1; assert(Analog_Read(result) && mask==1); mask=0;
    for(unsigned i=0;i<ADC_INPUT_COUNT;i++) assert(result[i]==120*round+114+i);
  }
  HAL_DMA_IRQHandler(&hdma_adc1); HAL_ADC_IRQHandler(&hadc1);
  HAL_ADC_ErrorCallback(&unrelated); assert(Analog_Read(result));
  HAL_ADC_ErrorCallback(&hadc1); assert(!Analog_Read(result));
  puts("PASS: calibration order and fallback, circular DMA init, scan ranks, repeated half/full callbacks, coherent snapshots, IRQ routing, errors");
}
''')
    subprocess.run(['cc','-std=gnu11','-Wall','-Wextra','-Werror',str(out/'test.c'),'-o',str(out/'test')],check=True)
    for scenario in range(5):
        subprocess.run([str(out/'test'), str(scenario)], check=True)
