TARGET = main
MCU = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
F_CPU = 100000000UL

CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

# Флаги компилятора: УБРАЛИ весь ад с экранированием кавычек!
CFLAGS = $(MCU) -Os -Wall -g -std=c99 -DSTM32F412Cx -DF_CPU=$(F_CPU)
CFLAGS += -Icmsis/inc -Isrc -Iinc -Idisp -Iu8g2x

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
        disp/lcd_io.c \
        disp/lcd_font.c

# Исходники ядра U8G2 и ваши шрифты
SRCS += u8g2x/lcd_draw_u8g2.c \
        u8g2x/u8g2_font.c \
        u8g2x/u8g2_font_10x20_t_cyrillic.c


OBJS = $(patsubst %.c, $(BUILD_DIR)/%.o, $(filter %.c, $(SRCS)))
OBJS += $(patsubst %.s, $(BUILD_DIR)/%.o, $(filter %.s, $(SRCS)))

all: $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) -Wl,--start-group $(OBJS) -Wl,--end-group -o $@
	$(SIZE) $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

clean:
	rm -rf $(BUILD_DIR)

flash: $(BUILD_DIR)/$(TARGET).bin
	openocd -f interface/cmsis-dap.cfg -f target/stm32f4x.cfg -c "program $(BUILD_DIR)/$(TARGET).elf verify reset exit"

.PHONY: all clean flash