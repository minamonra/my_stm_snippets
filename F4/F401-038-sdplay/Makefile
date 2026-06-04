# --- НАСТРОЙКА КАТАЛОГА (ОРИГИНАЛЬНЫЙ, РАБОЧИЙ ПОД WINDOWS) ---
SHELL = cmd

TARGET = main
MCU = STM32F401RC

CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

# Пути к папкам
CMSIS_DIR = ./CMSIS/Include
INC_DIR = ./inc
SRC_DIR = ./src
BUILD_DIR = build

LDSCRIPT = $(MCU)_FLASH.ld
STARTUP = $(SRC_DIR)/startup_stm32f401xx.s

# --- СПИСОК ИСХОДНИКОВ ---
# main.c теперь берется напрямую из корня
SRCS = main.c \
       $(SRC_DIR)/hardware.c \
       $(SRC_DIR)/st7789v.c \
       $(SRC_DIR)/sdcard.c \
       $(SRC_DIR)/diskio.c \
       $(SRC_DIR)/audio_i2s.c \
       $(SRC_DIR)/wav_player.c \
       $(SRC_DIR)/ff.c

# Превращаем пути src/file.c и корень/main.c в build/file.o
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(filter $(SRC_DIR)/%, $(SRCS)))
OBJS += $(BUILD_DIR)/main.o
OBJS += $(BUILD_DIR)/startup.o

# Флаги компилятора (Обязательно включают пути -I)
# Добавлен флаг -I. чтобы компилятор видел заголовочные файлы в корне (main.h)
CFLAGS = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 \
         -DSTM32F401xC -DHSE_VALUE=25000000 \
         -I. -I$(CMSIS_DIR) -I$(INC_DIR) \
         -Os -Wall -g -ffunction-sections -fdata-sections

# Флаги ассемблера
ASFLAGS = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -g

LDFLAGS = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 \
          -T$(LDSCRIPT) --specs=nano.specs -ffreestanding -nostartfiles \
          -Wl,--gc-sections \
          -Wl,--no-eh-frame-hdr

.PHONY: all clean flash flash-daplink unlock

# Главная цель сборки
all: $(BUILD_DIR) $(OBJS) $(BUILD_DIR)/$(TARGET).elf

# Создание папки build
$(BUILD_DIR):
	@if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)

# Правило для компиляции main.c, который лежит в корне
$(BUILD_DIR)/main.o: main.c
	$(CC) $(CFLAGS) -c $< -o $@

# Поштучная компиляция каждого .c файла из папки src/
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Компиляция стартапа
$(BUILD_DIR)/startup.o: $(STARTUP)
	$(CC) $(ASFLAGS) -c $< -o $@

# Финальная линковка из уже готовых .o файлов
$(BUILD_DIR)/$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@
	$(OBJCOPY) -O binary $@ $(BUILD_DIR)/$(TARGET).bin
	$(SIZE) $@

clean:
	@if exist $(BUILD_DIR) rmdir /s /q $(BUILD_DIR)

flash: all
	st-flash --reset write $(BUILD_DIR)/$(TARGET).bin 0x8000000

flash-daplink: all
	openocd -f interface/cmsis-dap.cfg -f target/stm32f4x.cfg -c "program build/$(TARGET).elf verify reset exit"

unlock:
	openocd -f interface/cmsis-dap.cfg -f target/stm32f4x.cfg -c "init" -c "reset halt" -c "stm32f4x unlock 0" -c "reset halt" -c "exit"