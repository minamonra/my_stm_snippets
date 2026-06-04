TARGET = main
BUILD_DIR = build
SHELL := cmd.exe

# Утилиты компилятора ARM GCC
CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

# Флаги архитектуры Cortex-M4 с аппаратным FPU
MCU_FLAGS = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16

# Инклуды и дефайны (-O0 для стабильной задержки delay)
CFLAGS = $(MCU_FLAGS) -Os -Wall -g -std=c99
CFLAGS += -DSTM32F412Cx
CFLAGS += -Icmsis/inc -Isrc -Iinc -Idisp
CFLAGS += -DF_CPU=100000000UL

# Настройки линкера
LDSCRIPT = STM32F412CE_FLASH.ld
LDFLAGS = $(MCU_FLAGS) -T$(LDSCRIPT) --specs=nano.specs -Wl,--gc-sections
#       disp/stm32f4xx_io.c 
SRCS = src/main.c \
       src/common.c \
       disp/lcd_io.c \
       cmsis/src/system_stm32f4xx.c \
       cmsis/src/startup_stm32f412cx.s
SRCS += disp/ili9488x.c
SRCS += disp/lcd_font.c
SRCS += src/demo.c

# ИСПРАВЛЕНИЕ: Добавляем файлы как исходный код .c, а не объекты .o
SRCS += disp/lcd_draw_u8g2.c
SRCS += disp/u8g2_font_10x20_t_cyrillic.c
SRCS += disp/u8g2_font.c

# Формируем пути к объектным файлам внутри каталога build
# (Превращаем, например, cmsis/src/startup.s в build/cmsis/src/startup.o)
OBJS = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(basename $(SRCS))))

# Главное правило сборки
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin

# Правило для компиляции Си-файлов
$(BUILD_DIR)/%.o: %.c
	@if not exist "$(dir $@)" mkdir "$(dir $@)"
	$(CC) $(CFLAGS) -c $< -o $@

# Правило для компиляции Ассемблерных файлов
$(BUILD_DIR)/%.o: %.s
	@if not exist "$(dir $@)" mkdir "$(dir $@)"
	$(CC) $(CFLAGS) -c $< -o $@

# Линковка ELF-файла
$(BUILD_DIR)/$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) -Wl,--start-group $(OBJS) -Wl,--end-group -o $@
	$(SIZE) $@

# Создание BIN-файла
$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

# Очистка каталога сборки, совместимая с Windows (PowerShell/CMD)
clean:
	@if exist $(BUILD_DIR) rmdir /s /q $(BUILD_DIR)

# Прошивка через DAP-Link (теперь берет ELF из папки build)
flash: all
	openocd -f interface/cmsis-dap.cfg -f target/stm32f4x.cfg -c "program $(BUILD_DIR)/$(TARGET).elf verify reset exit"