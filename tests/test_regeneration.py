#!/usr/bin/env python3
"""Check preservation contracts; simulate GPIO regeneration using USER blocks.
This is not a replacement for running STM32CubeMX itself.
"""
import pathlib
import re
import shutil
import subprocess
import tempfile
root=pathlib.Path(__file__).resolve().parents[1]
def sections(path):
    text=path.read_text()
    pairs={}
    stack=[]
    for match in re.finditer(r'/\* USER CODE (BEGIN|END) (.*?) \*/',text):
        action,name=match.groups()
        if action=='BEGIN':
            assert not stack,(path,'nested section',name)
            stack.append((name,match.end()))
        else:
            assert stack and stack[-1][0]==name,(path,'unpaired section',name)
            _,start=stack.pop()
            assert name not in pairs,(path,'duplicate section',name)
            pairs[name]=text[start:match.start()]
    assert not stack,(path,'unclosed section')
    return pairs
for name in ['main.c','gpio.c','i2c.c','adc.c','dac.c','dma.c','stm32g0xx_it.c','stm32g0xx_hal_msp.c']:
    sections(root/'Core/Src'/name)
assert 'INPUT_PollPress' in sections(root/'Core/Src/gpio.c')['2']
assert 'INPUT_PollPress' in sections(root/'Core/Inc/gpio.h')['Prototypes']
assert 'SSD1306_WriteData' in sections(root/'Core/Inc/i2c.h')['Prototypes']
assert 'SSD1306_WriteData' in sections(root/'Core/Src/i2c.c')['1']
assert 'Analog_Init();' in sections(root/'Core/Src/main.c')['2']
assert 'MX_ADC1_Init();' not in sections(root/'Core/Src/main.c')['2']
for irq in ('ADC1_COMP_IRQHandler','DMA1_Channel1_IRQHandler'):
    definitions=[]
    for path in (root/'Core/Src').glob('*.c'):
        if re.search(r'void\s+'+irq+r'\s*\(void\)\s*\{',path.read_text()): definitions.append(path.name)
    assert definitions==['stm32g0xx_it.c'],(irq,definitions)
with tempfile.TemporaryDirectory() as tmp:
    target=pathlib.Path(tmp)
    shutil.copytree(root/'Core',target/'Core')
    (target/'tests').mkdir()
    shutil.copy(root/'tests/test_keys.py',target/'tests/test_keys.py')
    src=sections(root/'Core/Src/gpio.c')
    (target/'Core/Src/gpio.c').write_text('#include "gpio.h"\n'+
        '\n'.join(src[n] for n in ('0','1'))+'\nvoid MX_GPIO_Init(void) {}\n'+src['2'])
    hdr=sections(root/'Core/Inc/gpio.h')
    (target/'Core/Inc/gpio.h').write_text('#ifndef GPIO_H\n#define GPIO_H\n#include "main.h"\nvoid MX_GPIO_Init(void);\n'+hdr['Prototypes']+'\n#endif\n')
    subprocess.run(['python3',str(target/'tests/test_keys.py')],check=True)
print('PASS: paired USER sections, GPIO regeneration model, single IRQ ownership, I2C declarations')
