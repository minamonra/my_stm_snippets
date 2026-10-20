SHELL = cmd.exe

TARGET = main
MCU = -mcpu=cortex-m3 -mthumb

CC = arm-none-eabi-gcc
AS = arm-none-eabi-gcc -x assembler-with-cpp
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

BUILD_DIR = build

CFLAGS = $(MCU) -Os -Wall -g3 -std=gnu11 -DSTM32F103xB -DDEBUG
# Все инклуды проекта
CFLAGS += -Icore -Icore/m-profile -Idevice/Include -Isrc
CFLAGS += -Irtos -Irtos/include -Irtos/portable/GCC/ARM_CM3
CFLAGS += -MMD -MP -fdata-sections -ffunction-sections

ASFLAGS = $(MCU) -g3 -O0

#LDFLAGS = $(MCU) -Tcore/stm32f103c8tx_flash.ld --specs=nano.specs -Wl,--gc-sections -Wl,--no-warn-rwx-segments -Wl,-Map=$(BUILD_DIR)/$(TARGET).map
LDFLAGS = $(MCU) -Tcore/stm32f103c8tx_flash.ld --specs=nano.specs -Wl,--gc-sections -Wl,--no-warn-rwx-segments -Wl,--print-memory-usage -Wl,-Map=$(BUILD_DIR)/$(TARGET).map
# ============================================================================
# АВТОМАТИЧЕСКИЙ ПОИСК ФАЙЛОВ
# ============================================================================

# Все .c файлы из src, device/Source и ядра FreeRTOS.
# croutine.c исключён: при configUSE_CO_ROUTINES = 0 он компилируется в пустой
# объектный файл и только засоряет build/.
SRCS = $(wildcard src/*.c) \
       $(wildcard device/Source/*.c) \
       $(filter-out rtos/croutine.c,$(wildcard rtos/*.c)) \
       rtos/portable/GCC/ARM_CM3/port.c \
       rtos/portable/MemMang/heap_4.c

# Автоматически подхватываем стартап
ASM_SRCS = $(wildcard device/Source/*.s)

# Плоский список объектных файлов в папке build (без подпапок)
OBJS = $(addprefix $(BUILD_DIR)/, $(notdir $(patsubst %.c, %.o, $(SRCS))))
OBJS += $(addprefix $(BUILD_DIR)/, $(notdir $(patsubst %.s, %.o, $(ASM_SRCS))))

DEPS = $(patsubst %.o, %.d, $(OBJS))

# Пути поиска для утилиты make
vpath %.c src device/Source rtos rtos/portable/GCC/ARM_CM3 rtos/portable/MemMang
vpath %.s device/Source

# ============================================================================
# ПРАВИЛА СБОРКИ
# ============================================================================

all: $(BUILD_DIR) $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.s
	$(AS) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@
	$(SIZE) $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR):
	@if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)

clean:
	@if exist $(BUILD_DIR) rmdir /s /q $(BUILD_DIR)
	@if exist .cache rmdir /s /q .cache
	@if exist .vscode\.cache rmdir /s /q .vscode\.cache

-include $(DEPS)

# ============================================================================
# ИНСТРУМЕНТЫ VS CODE
# ============================================================================

PYTHON        = python
LD_SCRIPT     = core/stm32f103c8tx_flash.ld
MEMORY_CONFIG = .vscode/memory_config.json
ELF_FILE      = build/main.elf
MAP_FILE      = build/main.map
SIZE_TOOL     = arm-none-eabi-size
ANALYZER      = .vscode/py_scripts/build_analyzer.py
COMPILE_DB    = .vscode/compile_commands.json

.PHONY: all clean compile_commands ld_analyzer memory_report size

# База компиляции для clangd: генерируется из реальных команд Makefile
compile_commands:
	@$(PYTHON) .vscode/py_scripts/gen_compile_db.py

# Карта памяти из линкерного скрипта -> .vscode/memory_config.json
ld_analyzer:
	@$(PYTHON) $(ANALYZER) --ld $(LD_SCRIPT) --config $(MEMORY_CONFIG) --regen

# Полный отчёт по занятости Flash/RAM
memory_report:
	@$(PYTHON) $(ANALYZER) --ld $(LD_SCRIPT) --elf $(ELF_FILE) --map $(MAP_FILE) --config $(MEMORY_CONFIG) --size-tool $(SIZE_TOOL)

# Только размеры
size:
	$(SIZE) -A $(ELF_FILE)