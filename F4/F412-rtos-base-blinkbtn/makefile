SHELL = cmd.exe

TARGET = main
MCU = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
F_CPU = 96000000UL

CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

CFLAGS = $(MCU) -Os -Wall -g -std=c99 -DSTM32F412Rx -DF_CPU=$(F_CPU)

# Подавляем конкретное предупреждение об неиспользуемых переменных для чистоты вывода ядра ОС
CFLAGS += -Wno-unused-variable

# ============================================================================
# ПУТИ ИНКЛУДОВ
# ============================================================================
CFLAGS += -Icmsis/inc \
          -Isrc \
          -Irtos/inc \
          -Irtos/portable/GCC/ARM_CM4F

CFLAGS += -MMD -MP
CFLAGS += -include common.h

LDFLAGS = $(MCU) -TSTM32F412RETx_FLASH.ld --specs=nano.specs -Wl,--gc-sections -Wl,--no-warn-rwx-segments

BUILD_DIR = build

OOCD_INTERFACE = interface/cmsis-dap.cfg
OOCD_TARGET = target/stm32f4x.cfg
OOCD_SPEED = 500

# ============================================================================
# ИСХОДНИКИ (ОСТАВИЛИ ТОЛЬКО САМОЕ НЕОБХОДИМОЕ ЯДРО)
# ============================================================================
SRCS = src/main.c \
       src/common.c \
       cmsis/src/system_stm32f4xx.c \
       cmsis/src/startup_stm32f412vx.s \
       rtos/src/list.c \
       rtos/src/queue.c \
       rtos/src/tasks.c \
       rtos/src/timers.c \
       rtos/portable/MemMang/heap_4.c \
       rtos/portable/GCC/ARM_CM4F/port.c

OBJS = $(patsubst %.c, $(BUILD_DIR)/%.o, $(filter %.c, $(SRCS)))
OBJS += $(patsubst %.s, $(BUILD_DIR)/%.o, $(filter %.s, $(SRCS)))

DEPS = $(OBJS:.o=.d)

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin

# ПРАВИЛО ДЛЯ ВСЕХ .c ФАЙЛОВ
$(BUILD_DIR)/%.o: %.c
	@if not exist "$(dir $@)" mkdir "$(dir $@)"
	$(CC) $(CFLAGS) -c $< -o $@

# ПРАВИЛО ДЛЯ .s ФАЙЛОВ
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

-include $(DEPS)
