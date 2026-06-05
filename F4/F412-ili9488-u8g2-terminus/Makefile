TARGET = main
MCU = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
F_CPU = 100000000UL

CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

CFLAGS = $(MCU) -Os -Wall -g -std=c99 -DSTM32F412Cx -DF_CPU=$(F_CPU)
CFLAGS += -Icmsis/inc -Isrc -Iinc -Idisp -Iu8g2x -Iu8g2x/u8g2 -Iu8g2x/u8fonts

LDFLAGS = $(MCU) -TSTM32F412CE_FLASH.ld --specs=nano.specs -Wl,--gc-sections

BUILD_DIR = build

# Исходники ядра проекта
SRCS = src/main.c \
       src/common.c \
       src/demo.c \
       cmsis/src/system_stm32f4xx.c \
       cmsis/src/startup_stm32f412cx.s

# Исходники драйвера дисплея ILI9488
SRCS += disp/ili9488x.c \
        disp/lcd_io.c

# Исходники ядра U8G2
SRCS += u8g2x/u8g2/u8g2_font.c

# Мой "форк"
SRCS += u8g2x/lcd_draw_u8g2.c

# ШРИФТЫ (в папке u8fonts)
SRCS += u8g2x/u8fonts/u8g2_font_10x20_t_cyrillic.c \
        u8g2x/u8fonts/terminus_28b_cyr.c

OBJS = $(patsubst %.c, $(BUILD_DIR)/%.o, $(filter %.c, $(SRCS)))
OBJS += $(patsubst %.s, $(BUILD_DIR)/%.o, $(filter %.s, $(SRCS)))

# Главное правило сборки
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin

# Правило для компиляции Си-файлов (Кроссплатформенное создание папок через mkdir -p)
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Правило для компиляции Ассемблерных файлов
$(BUILD_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Линковка ELF-файла
$(BUILD_DIR)/$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) -Wl,--start-group $(OBJS) -Wl,--end-group -o $@
	$(SIZE) $@

# Создание BIN-файла
$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

# Очистка каталога сборки через стандартный rm -rf
clean:
	rm -rf $(BUILD_DIR)

# Прошивка через DAP-Link
flash: all
	openocd -f interface/cmsis-dap.cfg -f target/stm32f4x.cfg -c "program $(BUILD_DIR)/$(TARGET).elf verify reset exit"
