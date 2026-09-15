SHELL = cmd.exe

TARGET = main
MCU = -mcpu=cortex-m3 -mthumb

CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

CFLAGS = $(MCU) -Os -Wall -g -std=c99 -DSTM32F103xB
CFLAGS += -Icore -Idevice/Include -Isrc
CFLAGS += -MMD -MP

LDFLAGS = $(MCU) -Tstm32f103c8tx.ld --specs=nano.specs -Wl,--gc-sections

BUILD_DIR = build

OOCD_INTERFACE = interface/cmsis-dap.cfg
OOCD_TARGET = target/stm32f1x.cfg
OOCD_SPEED = 1000

# ============================================================================
# ИСХОДНИКИ (startup.c перенесен в device/Source)
# ============================================================================
SRCS = src/main.c \
       src/common.c \
       src/rs485.c \
       src/button.c \
       src/enc.c \
       src/btn_modes.c \
       src/btn_actions.c \
       src/dispmt16s.c \
       device/Source/startup.c \
       device/Source/system_stm32f1xx.c

OBJS = $(patsubst %.c, $(BUILD_DIR)/%.o, $(filter %.c, $(SRCS)))

DEPS = $(OBJS:.o=.d)

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR)/%.o: %.c
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
	openocd -f $(OOCD_INTERFACE) -f $(OOCD_TARGET) -c "adapter speed $(OOCD_SPEED)" -c \
	"program $(BUILD_DIR)/$(TARGET).elf verify reset exit"

-include $(DEPS)
