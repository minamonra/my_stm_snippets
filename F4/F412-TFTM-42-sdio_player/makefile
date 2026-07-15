SHELL = cmd.exe

TARGET = main
MCU = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
F_CPU = 96000000UL

CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

CFLAGS = $(MCU) -Os -Wall -g -std=c99 -DSTM32F412Rx -DF_CPU=$(F_CPU)

# === ДОБАВЛЕН ПУТЬ ДЛЯ ИНКЛЮДОВ u8g2x ===
CFLAGS += -Icmsis/inc -Isrc -Ifs -Irapp -Iaudio -Idisp -Iu8g2x
CFLAGS += -MMD -MP
CFLAGS += -include u8g2.h -include common.h

LDFLAGS = $(MCU) -TSTM32F412RETx_FLASH.ld --specs=nano.specs -Wl,--gc-sections

BUILD_DIR = build

# Настройки OpenOCD
OOCD_INTERFACE = interface/cmsis-dap.cfg
OOCD_TARGET = target/stm32f4x.cfg
OOCD_SPEED = 500

# ============================================================================
# ИСХОДНИКИ (Добавлены файлы движка u8g2x и два выбранных шрифта Terminus)
# ============================================================================
SRCS = src/main.c \
       src/common.c \
       src/tm1637.c \
       src/uart.c \
       disp/lcd_io.c \
       rapp/config_parser.c \
       rapp/route_mgr.c \
       audio/audio_i2s.c \
       audio/wav_player.c \
       fs/sdcard.c \
       fs/sdcard_dma.c \
       fs/diskio.c \
       fs/ff.c \
       fs/ffunicode.c \
       u8g2x/lcd_draw_u8g2.c \
       u8g2x/u8g2_font.c \
       u8g2x/u8fonts/terminus_32b_cyr.c \
       u8g2x/u8fonts/terminus_32n_cyr.c \
       u8g2x/u8fonts/terminus_28b_cyr.c \
       u8g2x/u8fonts/terminus_28n_cyr.c \
       u8g2x/u8fonts/terminus_24b_cyr.c \
       u8g2x/u8fonts/terminus_24n_cyr.c \
       cmsis/src/system_stm32f4xx.c \
       cmsis/src/startup_stm32f412vx.s

OBJS = $(patsubst %.c, $(BUILD_DIR)/%.o, $(filter %.c, $(SRCS)))
OBJS += $(patsubst %.s, $(BUILD_DIR)/%.o, $(filter %.s, $(SRCS)))

DEPS = $(OBJS:.o=.d)

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR)/%.o: %.c
	@if not exist "$(dir $@)" mkdir "$(dir $@)"
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.s
	@if not exist "$(dir $@)" mkdir "$(dir $@)"
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) -Wl,--start-group $(OBJS) -Wl,--end-group -o $@
	$(SIZE) $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

clean:
	@if exist $(BUILD_DIR) ( \
		del /f /q /s $(BUILD_DIR) >nul 2>&1 & \
		rmdir /s /q $(BUILD_DIR) \
	)

flash: all
	openocd -f $(OOCD_INTERFACE) -f $(OOCD_TARGET) -c "adapter speed $(OOCD_SPEED)" -c "program $(BUILD_DIR)/$(TARGET).elf verify reset exit"

flash-nr:
	openocd -f $(OOCD_INTERFACE) -f $(OOCD_TARGET) -c "adapter speed $(OOCD_SPEED)" -c "reset_config srst_only srst_nogate connect_assert_srst" -c "init; reset halt; program build/main.elf verify reset exit"

# ============================================================================
# СТИРАНИЕ ЧИПА
# ============================================================================
erase-chip:
	openocd -f $(OOCD_INTERFACE) -f $(OOCD_TARGET) -c "adapter speed $(OOCD_SPEED)" -c "init; reset halt; stm32f4x mass_erase 0; exit"

# Alternative variant (if protected)
erase-chip-unlock:
	openocd -f $(OOCD_INTERFACE) -f $(OOCD_TARGET) -c "adapter speed $(OOCD_SPEED)" -c "init; reset halt; stm32f4x unlock 0; stm32f4x mass_erase 0; exit"

# Fast erase via ST-LINK CLI (if installed)
erase-stlink:
	ST-LINK_CLI -c SWD -ME

-include $(DEPS)
