# Portable build; STM32CubeIDE can also regenerate its own Debug makefiles.
CROSS_COMPILE ?= arm-none-eabi-
BUILD_DIR ?= build
CC := $(CROSS_COMPILE)gcc
OBJCOPY := $(CROSS_COMPILE)objcopy
SIZE := $(CROSS_COMPILE)size
TARGET := $(BUILD_DIR)/STM32G051
C_SOURCES := $(wildcard Core/Src/*.c) $(filter-out %_template.c,$(wildcard Drivers/STM32G0xx_HAL_Driver/Src/*.c))
S_SOURCES := $(wildcard Core/Startup/*.s)
OBJECTS := $(addprefix $(BUILD_DIR)/,$(C_SOURCES:.c=.o) $(S_SOURCES:.s=.o))
CPU := -mcpu=cortex-m0plus -mthumb -mfloat-abi=soft
CPPFLAGS := -DDEBUG -DUSE_HAL_DRIVER -DSTM32G051xx $(addprefix -I,Core/Inc Drivers/STM32G0xx_HAL_Driver/Inc Drivers/STM32G0xx_HAL_Driver/Inc/Legacy Drivers/CMSIS/Device/ST/STM32G0xx/Include Drivers/CMSIS/Include)
CFLAGS := $(CPU) -std=gnu11 -g3 -O0 -Wall -ffunction-sections -fdata-sections --specs=nano.specs -MMD -MP
LDFLAGS := $(CPU) -TSTM32G051F8PX_FLASH.ld --specs=nosys.specs --specs=nano.specs -Wl,--gc-sections -Wl,-Map=$(TARGET).map
.PHONY: all clean
all: $(TARGET).elf $(TARGET).hex $(TARGET).bin
	$(SIZE) $(TARGET).elf
$(TARGET).elf: $(OBJECTS) STM32G051F8PX_FLASH.ld
	$(CC) $(LDFLAGS) $(OBJECTS) -Wl,--start-group -lc -lm -Wl,--end-group -o $@
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
$(BUILD_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CPU) -g3 -x assembler-with-cpp -MMD -MP -c $< -o $@
$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@
$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@
clean:
	rm -rf $(BUILD_DIR)
-include $(OBJECTS:.o=.d)
