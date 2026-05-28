TARGET = main
# Папка для сборки
BUILD_DIR = build

# Настройки компилятора
CC = arm-none-eabi-gcc
CP = arm-none-eabi-objcopy
SZ = arm-none-eabi-size

# Настройки архитектуры ядра Cortex-M3 (STM32F103)
MCU = -mcpu=cortex-m3 -mthumb

# Флаг для Medium Density чипов (например, STM32F103C8T6)
DEFINES = -DSTM32F103xB

# Пути к заголовочным файлам
INCLUDES = -Iinc \
           -Icmsis/inc

# Флаги компиляции
CFLAGS = $(MCU) $(DEFINES) $(INCLUDES) -O0 -g3 -Wall -fdata-sections -ffunction-sections

# Скрипт линкера (лежит в корне) и флаги линковки
LDSCRIPT = STM32F103XB_FLASH.ld
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) -Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(TARGET).map

# Список исходных файлов проекта
C_SOURCES = src/main.c \
            cmsis/src/system_stm32f1xx.c

ASM_SOURCES = cmsis/src/startup_stm32f103xb.s

# Формируем список объектных файлов в папке build
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))

OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

# Правило по умолчанию
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

# Компиляция C файлов
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

# Компиляция ASM файлов
$(BUILD_DIR)/%.o: %.s | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

# Линковка проекта
$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) | $(BUILD_DIR)
	$(CC) $(OBJECTS) $(CFLAGS) $(LDFLAGS) -o $@
	$(SZ) $@

# Генерация HEX и BIN
$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf
	$(CP) -O ihex $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf
	$(CP) -O binary -S $< $@

# Создание папки build, если её нет
$(BUILD_DIR):
	mkdir $@

# Команда автоматической прошивки (DAPLink, 100 кГц)
flash: all
	openocd -f interface/cmsis-dap.cfg -f target/stm32f1x.cfg -c "adapter speed 100; init; reset halt; flash write_image erase $(BUILD_DIR)/$(TARGET).hex; reset run; shutdown"

# Очистка проекта (удаляет всю папку build)
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean flash