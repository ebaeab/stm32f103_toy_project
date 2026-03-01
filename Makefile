# Makefile for STM32F103C8T6 Hello World Project

# 工具链定义
CC = arm-none-eabi-gcc
AS = arm-none-eabi-as
LD = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE = arm-none-eabi-size

# 编译选项
MCU = cortex-m3
CFLAGS = -mcpu=$(MCU) -mthumb -mfloat-abi=soft
CFLAGS += -g
CFLAGS += -Wall -Wextra -Wpedantic
#CFLAGS += -fdata-sections -ffunction-sections
CFLAGS += -nostartfiles

# 设备型号和外设宏定义（编译LL库源文件时需要）
CFLAGS += -DSTM32F103C8T6
CFLAGS += -DSTM32F103xB
CFLAGS += -DUSE_FULL_LL_DRIVER

# LCD功能使能 - 注释掉下面两行可禁用LCD功能
#USE_LCD := 1
ifdef USE_LCD
CFLAGS += -DUSE_LCD_UC1701
endif

# WS2812B功能使能 - 注释掉下面两行可禁用WS2812B
#USE_WS2812B := 1
ifdef USE_WS2812B
CFLAGS += -DUSE_WS2812B
endif

# 头文件包含路径
CFLAGS += -I.
CFLAGS += -I./STM32CubeF1/Drivers/CMSIS/Device/ST/STM32F1xx/Include
CFLAGS += -I./STM32CubeF1/Drivers/CMSIS/Include
CFLAGS += -I./STM32CubeF1/Drivers/STM32F1xx_HAL_Driver/Inc

# 链接选项
LDFLAGS = -mcpu=$(MCU) -mthumb -mfloat-abi=soft
# 可以大概较少200字节
# LDFLAGS += -specs=nosys.specs -specs=nano.specs
LDFLAGS += -Wl,--gc-sections -Wl,-Map=$(TARGET).map
LDFLAGS += -T$(LDSCRIPT)

# 目标文件
TARGET = stm32f103_helloworld
LDSCRIPT = stm32f103c8t6.ld

# 源文件
SRCS = main.c
# LCD功能源文件 - 仅在USE_LCD定义时编译
ifdef USE_LCD
SRCS += lcd_uc1701.c
endif

# WS2812B功能源文件 - 仅在USE_WS2812B定义时编译
ifdef USE_WS2812B
SRCS += ws2812b.c
endif

SRCS += STM32CubeF1/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/system_stm32f1xx.c
SRCS += STM32CubeF1/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_ll_rcc.c
SRCS += STM32CubeF1/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_ll_gpio.c
SRCS += STM32CubeF1/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_ll_usart.c
SRCS += STM32CubeF1/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_ll_utils.c
ASM_SRCS = startup_stm32f10x_md.s

# 生成的目标文件（保持原路径）
OBJS = $(SRCS:.c=.o) $(ASM_SRCS:.s=.o)

# 默认目标
all: $(TARGET).bin $(TARGET).hex

# 生成二进制文件
$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

# 生成hex文件
$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

# 生成elf文件
$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ -lm
	$(SIZE) $@

# 编译C文件
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# 编译汇编文件
%.o: %.s
	$(CC) $(CFLAGS) -c -o $@ $<

# 烧录目标（需要stlink工具）
flash: $(TARGET).bin
	st-flash --reset write $(TARGET).bin 0x08000000

# 清理
clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).bin $(TARGET).hex $(TARGET).map

# 调试信息
debug: $(TARGET).elf
	arm-none-eabi-objdump -S $< > $(TARGET).lst

# 显示大小信息
size: $(TARGET).elf
	$(SIZE) $@

.PHONY: all clean flash debug size
