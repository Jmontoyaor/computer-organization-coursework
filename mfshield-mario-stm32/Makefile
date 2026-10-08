# Multi-Function Shield: tema de Super Mario con el zumbador (TIM2 PWM en PB3) + frecuencia en el display
# Instalación de STM32CubeIDE 2.2.0 (toolchain y programador); en otro equipo: make CUBEIDE=/ruta/al/ide
CUBEIDE ?= $(HOME)/st/stm32cubeide_2.2.0
TC   := $(CUBEIDE)/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.linux64_1.0.100.202602081740/tools/bin
PROG := $(CUBEIDE)/plugins/com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.linux64_2.2.500.202603051304/tools/bin/STM32_Programmer_CLI

CC      := $(TC)/arm-none-eabi-gcc
SIZE    := $(TC)/arm-none-eabi-size
OBJCOPY := $(TC)/arm-none-eabi-objcopy

TARGET := mario
BUILD  := build
MCU    := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

ASRCS := Src/main.s Startup/startup_stm32l476rgtx.s
CSRCS := Src/syscalls.c Src/sysmem.c
OBJS  := $(addprefix $(BUILD)/,$(ASRCS:.s=.o) $(CSRCS:.c=.o))

CFLAGS  := $(MCU) -g3 -O0 -Wall -ffunction-sections -fdata-sections -DSTM32L476xx
ASFLAGS := $(MCU) -g3 -x assembler-with-cpp
LDFLAGS := $(MCU) -TSTM32L476RGTX_FLASH.ld --specs=nosys.specs --specs=nano.specs \
           -Wl,--gc-sections -Wl,-Map=$(BUILD)/$(TARGET).map

all: $(BUILD)/$(TARGET).elf

$(BUILD)/%.o: %.s
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/$(TARGET).elf: $(OBJS) STM32L476RGTX_FLASH.ld
	$(CC) $(OBJS) $(LDFLAGS) -o $@
	$(SIZE) $@

flash: all
	$(PROG) -c port=SWD mode=UR reset=HWrst -w $(BUILD)/$(TARGET).elf -v -rst

clean:
	rm -rf $(BUILD)

.PHONY: all flash clean
