# stm32f103c8t6_my_new_template

Шаблон проекта для STM32F103C8T6: FreeRTOS, CMSIS, сборка через Makefile,
анализ памяти и IntelliSense (clangd) в VS Code.

Без HAL — только CMSIS и регистры. Без CubeIDE — только arm-none-eabi-gcc.

---

## Требования

| Компонент | Версия | Проверка |
|---|---|---|
| ARM GCC Toolchain | 13+ (проверено 15.2.1 xPack) | `arm-none-eabi-gcc --version` |
| GNU Make | любая | `make --version` |
| Python | 3.8+ | `python --version` |
| OpenOCD | 0.11+ | `openocd --version` |
| VS Code | + расширение clangd | `llvm-vs-code-extensions.vscode-clangd` |

Пути к тулчейну и OpenOCD должны быть в `PATH` — тогда `make` и задачи
VS Code найдут их без дополнительных настроек.

---

## Параметры МК

| Параметр | Значение |
|---|---|
| MCU | STM32F103C8T6 |
| Ядро | Cortex-M3, 72 МГц (HSE 8 MHz × 9) |
| Flash | 64 KB |
| RAM | 20 KB |
| FreeRTOS | V11.1.0+ |
| CMSIS-Core | 6.3.0 |

---

## Структура проекта

```
core/                     CMSIS-Core (только файлы для M3 + GCC)
├── cmsis_compiler.h
├── cmsis_gcc.h
├── cmsis_version.h
├── core_cm3.h
├── stm32f103.svd          периферия для отладчика
├── stm32f103c8tx_flash.ld линкер-скрипт
└── m-profile/
    └── cmsis_gcc_m.h

device/                   STM32F1: заголовки, startup, syscalls
├── Include/
│   ├── stm32f103xb.h
│   ├── stm32f1xx.h
│   └── system_stm32f1xx.h
└── Source/
    ├── startup_stm32f103c8tx.s
    ├── syscalls.c
    ├── sysmem.c
    └── system_stm32f1xx.c

rtos/                     ядро FreeRTOS
├── include/              заголовки
├── portable/
│   ├── GCC/ARM_CM3/      port.c, portmacro.h
│   └── MemMang/heap_4.c  динамическая память
└── *.c                   tasks, queue, list, timers, event_groups, ...

src/
├── FreeRTOSConfig.h      конфигурация ядра
└── main.c

.vscode/                  настройки VS Code, скрипты
├── tasks.json
├── settings.json
├── launch.json
└── py_scripts/
    ├── build_analyzer.py     анализ памяти по ELF и .ld
    └── gen_compile_db.py     генератор compile_commands.json

update_cmsis_rtos.ps1     обновление CMSIS и FreeRTOS из локальных репозиториев
makefile                  сборка
.clangd                   настройки clangd
```

---

## Сборка

```sh
make              # сборка
make clean        # очистка
make -j12 all     # параллельная сборка
```

Результат в `build/`:

| Файл | Содержимое |
|---|---|
| `main.elf` | образ для отладки и прошивки |
| `main.bin` | сырой образ для программатора |
| `main.map` | карта линковки (адреса секций, символы) |
| `*.o`, `*.d` | объектные файлы и зависимости |

---

## Анализ памяти

Две отдельные цели:

```sh
make ld_analyzer     # карта памяти из .ld -> .vscode/memory_config.json
make memory_report   # отчёт по занятости Flash/RAM
```

`ld_analyzer` разбирает `core/stm32f103c8tx_flash.ld`: блок `MEMORY`
(регионы и их размеры) и блок `SECTIONS` (какая секция в каком регионе,
с учётом `AT>`). Результат — `.vscode/memory_config.json`.

`memory_report` читает ELF через `arm-none-eabi-size -A`, сопоставляет
секции с картой памяти и печатает отчёт:

```
Region     Start address   End address     Size       Free       Used       Usage (%)
FLASH:     0x08000000      0x08010000      64 KB      57.33 KB   6.67 KB    10.42%
RAM:       0x20000000      0x20005000      20 KB      10.48 KB   9.52 KB    47.62%
```

Секция `.data` учитывается в обоих регионах: в Flash она лежит как образ
(LMA), в RAM — как работающие данные (VMA).

Дополнительно:

```sh
make memory_report SIZE_TOOL=C:/путь/к/arm-none-eabi-size.exe
```

Скрипту можно передать и другие аргументы, например `--details`
для сырого вывода `size -A`:

```sh
python .vscode/py_scripts/build_analyzer.py --details --elf build/main.elf
```

---

## IntelliSense (clangd)

```sh
make compile_commands
```

Генерирует `.vscode/compile_commands.json` из реальных команд компиляции.
Скрипт `.vscode/py_scripts/gen_compile_db.py` использует `make -n -B`,
поэтому достаточно стандартной библиотеки Python — `compiledb` и `bear`
не нужны.

clangd сам отслеживает изменения базы и перечитывает её, перезапуск обычно
не требуется. В VS Code за полным обновлением (база + рестарт сервера)
отвечает задача `update_clangd`.

Если IntelliSense «залип» после обновления CMSIS:

```sh
make kill-clangd
```

или `Ctrl+Shift+P` → `clangd: Restart language server`.

### Настройки clangd

`.clangd` в корне проекта:

```yaml
CompileFlags:
  CompilationDatabase: .vscode
  Compiler: arm-none-eabi-gcc
  Add:
    - -std=gnu11
    - -DSTM32F103xB
    - -DDEBUG
    - -Icore
    - -Icore/m-profile
    - -Idevice/Include
    - -Isrc
    - -Irtos
    - -Irtos/include
    - -Irtos/portable/GCC/ARM_CM3
    - -U__clang__
    - -Wno-builtin-macro-redefined
    - -Wno-unknown-pragmas
    - -Wno-unknown-attributes
```

Пути Include продублированы из `CFLAGS` в Makefile — это нужно, потому что
clangd применяет флаги из `CompileFlags.Add` ко всем файлам, включая
заголовки, которых нет в базе компиляции.

Флаг `-U__clang__` убирает макрос `__clang__`. Без него `cmsis_compiler.h`
подключает `cmsis_clang.h` — файл для Arm Compiler 6, который clangd не
может разобрать. С `-U__clang__` clangd работает по той же ветке GCC,
что и реальная сборка, и расхождений между редактором и компилятором нет.

`.vscode/settings.json`:

```json
{
  "editor.formatOnSave": true,
  "editor.defaultFormatter": "llvm-vs-code-extensions.vscode-clangd",
  "C_Cpp.intelliSenseEngine": "disabled",
  "clangd.arguments": [
    "--background-index",
    "--clang-tidy=false",
    "--header-insertion=never",
    "--query-driver=arm-none-eabi-gcc*"
  ],
  "files.associations": { "*.h": "c" },
  "files.exclude": { "build": true }
}
```

`--query-driver` разрешает clangd спросить у компилятора пути к системным
заголовкам newlib (`stdint.h`, `string.h` и т.д.) — без него они не находятся.

Расширение Microsoft C/C++ должно быть отключено (или не установлено):
`C_Cpp.intelliSenseEngine: disabled` выше. Файл `.vscode/c_cpp_properties.json`
в этом проекте не нужен.

---

## Задачи VS Code

| Задача | Действие |
|---|---|
| `build` (Ctrl+Shift+B) | сборка → карта памяти → отчёт |
| `build:fast` | только сборка |
| `rebuild` | очистка → сборка → карта памяти → отчёт |
| `clean` | очистка |
| `ld_analyzer` | пересоздать `.vscode/memory_config.json` |
| `Build Analyzer` | отчёт по памяти |
| `compile_commands.json_update` | обновить базу для clangd |
| `update_clangd` | обновить базу и перезапустить clangd |
| `flash` | прошить `build/main.elf` через OpenOCD |
| `Monitor resume` | снять МК с паузы после отладки |

Порядок в композитной задаче `build` задан через `dependsOn` +
`dependsOrder: "sequence"`.

---

## Прошивка и отладка

```sh
openocd -f interface/cmsis-dap.cfg -f target/stm32f1x.cfg \
        -c "program build/main.elf verify reset exit"
```

Отладка — через `launch.json` (расширение Cortex-Debug), точка входа `main`.
Отладочная сессия поднимает OpenOCD сам, поэтому задача `flash` не должна
работать одновременно с открытой отладкой.

После остановки в отладчике МК остаётся на паузе — снять с паузы:

```sh
openocd -f interface/cmsis-dap.cfg -f target/stm32f1x.cfg \
        -c "init" -c "resume" -c "sleep 200" -c "exit"
```

---

## Настройка FreeRTOS

Всё в `src/FreeRTOSConfig.h`. При переносе шаблона на другой МК проверьте:

| Параметр | Что менять |
|---|---|
| `configCPU_CLOCK_HZ` | реальная частота ядра, из которой считается SysTick |
| `configTICK_RATE_HZ` | частота тиков; при 1000 Hz обязательно `configUSE_16_BIT_TICKS 0` |
| `configPRIO_BITS` | разрядность приоритетов NVIC (для STM32F1 — 4) |
| `configTOTAL_HEAP_SIZE` | размер кучи FreeRTOS в байтах; куча лежит в `.bss` |
| `configMAX_PRIORITIES` | число приоритетов; каждый добавляет ~60 B к `.bss` |
| `configMINIMAL_STACK_SIZE` | стек задачи в словах (StackType_t = 4 B) |
| `configUSE_TIMERS` | 1 — добавляет таймерную задачу (~1.5 KB Flash) |
| `configUSE_EVENT_GROUPS` | 1 — добавляет поддержку групп событий |

Приоритеты прерываний задаются через `configPRIO_BITS`:

```c
#define configPRIO_BITS                         4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5
```

Значения — это номера приоритетов NVIC, а не готовые байты: FreeRTOS
сдвигает их на `(8 - configPRIO_BITS)` сам.

`configASSERT` использует внешнюю функцию — её нужно объявить в `main.c`:

```c
void vAssertHandler( const char * pcFile, unsigned long ulLine ) {
    ( void ) pcFile;
    ( void ) ulLine;
    taskDISABLE_INTERRUPTS();
    for( ;; ) {
        __BKPT( 0 );
    }
}
```

Полезно также включить проверки (в `FreeRTOSConfig.h`):

```c
#define configCHECK_FOR_STACK_OVERFLOW          2
#define configUSE_MALLOC_FAILED_HOOK            1
```

и добавить соответствующие хуки в `main.c`.

---

## Обновление CMSIS и FreeRTOS

Скрипт `update_cmsis_rtos.ps1` берёт файлы из локальных клонов репозиториев
и синхронизирует их с проектом по белому списку — только то, что нужно
для STM32F103C8T6. Всё остальное удаляется.

```sh
# посмотреть, что изменится
pwsh -ExecutionPolicy Bypass -File .\update_cmsis_rtos.ps1 -DryRun

# применить
pwsh -ExecutionPolicy Bypass -File .\update_cmsis_rtos.ps1

# удаляемое сохранить в .cmsis_backup\
pwsh -ExecutionPolicy Bypass -File .\update_cmsis_rtos.ps1 -Backup

# обновить только CMSIS или только FreeRTOS
pwsh -ExecutionPolicy Bypass -File .\update_cmsis_rtos.ps1 -SkipRtos
pwsh -ExecutionPolicy Bypass -File .\update_cmsis_rtos.ps1 -SkipCore
```

Пути к источникам задаются в начале скрипта или параметрами:

```powershell
pwsh -ExecutionPolicy Bypass -File .\update_cmsis_rtos.ps1 `
     -CmsisInclude   "C:\_0GitHub\CMSIS_6\CMSIS\Core\Include" `
     -FreeRtosKernel "C:\_0GitHub\FreeRTOS-Kernel"
```

### Что копируется

**CMSIS-Core** → `core/`:

```
core_cm3.h
cmsis_compiler.h
cmsis_gcc.h
cmsis_version.h
m-profile/cmsis_gcc_m.h
```

**FreeRTOS** → `rtos/`: все `*.c` из корня ядра, все заголовки из `include/`,
порт `portable/GCC/ARM_CM3/`, менеджер кучи `portable/MemMang/heap_4.c`.

### Что намеренно не копируется

| Файл | Почему |
|---|---|
| `cmsis_clang.h` | рассчитан на Arm Compiler 6, clangd не может его разобрать |
| `m-profile/cmsis_clang_m.h` | то же |
| `m-profile/armv7m_mpu.h` | STM32F103 не имеет MPU, файл не включается в сборку, но ломает разбор clangd |
| `include/CMakeLists.txt`, `*.readme` | не нужны для сборки через Makefile |
| `portable/MemMang/heap_5.c` | используется `heap_4.c` |

Файлы `stm32f103c8tx_flash.ld` и `stm32f103.svd` — проектные, скрипт их
не удаляет.

---

## Генерируемые файлы

В `.gitignore`:

```
build/
.vscode/.cache/
.vscode/compile_commands.json
.vscode/memory_config.json
.cmsis_backup/
```

После клонирования репозитория:

```sh
make compile_commands
make ld_analyzer
```

---

## Частые проблемы

**`xTaskCreate` / `vTaskDelay` — undeclared function**

В `src/main.c` нет `#include "FreeRTOS.h"` и `#include "task.h"`. GCC 14+
считает неявное объявление ошибкой, а не предупреждением.

**`configUSE_IDLE_HOOK must be defined`**

В `FreeRTOSConfig.h` не заданы `configUSE_IDLE_HOOK` или `configUSE_TICK_HOOK`.
FreeRTOS V11 требует эти макросы явно.

**`core_cm3.h file not found` в clangd**

В `.clangd` нет `-Icore`. Все пути Include должны быть продублированы из
`CFLAGS` в Makefile.

**`cmsis_clang.h file not found`**

CMSIS 6.x подключает `cmsis_clang.h` при определённом `__clang__`. Файла нет
в проекте намеренно — clangd переведён в режим GCC флагом `-U__clang__`
в `.clangd`. Если этот флаг пропал, верните его.

**`stdint.h file not found`**

В `.vscode/settings.json` нет `--query-driver=arm-none-eabi-gcc*` в
`clangd.arguments`.

**Две системы IntelliSense, дублирующиеся ошибки**

Удалено расширение Microsoft C/C++ или выставлено
`"C_Cpp.intelliSenseEngine": "disabled"`. Файл
`.vscode/c_cpp_properties.json` не нужен.

**`make ld_analyzer` → `No rule to make target`**

В `makefile` потеряна цель `ld_analyzer`. Цели `compile_commands`,
`ld_analyzer` и `memory_report` должны быть в файле.

**`make: Nothing to be done for 'compile_commands'`**

Цель не имеет зависимостей, поэтому make считает её выполненной. Добавьте
`.PHONY: compile_commands ld_analyzer memory_report`.

**Отчёт по памяти показывает старые данные**

`make memory_report` читает `build/main.elf`. После сборки нужно заново
выполнить `make ld_analyzer` (карта памяти) и `make memory_report`.

**Ошибки в панели Problems после обновления CMSIS**

Сбросьте индекс clangd:

```sh
Remove-Item .vscode\.cache\clangd -Recurse -Force -ErrorAction SilentlyContinue
```

и перезапустите языковой сервер.

---

## Лицензия и источники

- CMSIS-Core — Apache-2.0, ARM Limited
- FreeRTOS — MIT, FreeRTOS.org
- STM32F1 headers и startup — BSD-3-Clause, STMicroelectronics

Скрипты `build_analyzer.py` и `gen_compile_db.py` написаны для этого шаблона.


### Не используйте `--exclude` в `clangd.arguments`

Официальное расширение `llvm-vs-code-extensions.vscode-clangd` (v0.6.x) не
поддерживает флаги `--exclude=<path>` и `--log=error`. С ними сервер не
стартует: в редакторе пропадают и диагностики, и автодополнение.

Эти флаги есть только в форке `clangd` от автора расширения.

Рабочий набор аргументов:

```json
"clangd.arguments": [
  "--background-index",
  "--clang-tidy=false",
  "--header-insertion=never",
  "--query-driver=arm-none-eabi-gcc*"
]
```

### Важно: не используйте `--exclude` в `clangd.arguments`

Официальное расширение `llvm-vs-code-extensions.vscode-clangd` не
поддерживает флаги `--exclude=<path>` и `--log=error`. С ними сервер
не стартует: в редакторе пропадают одновременно и диагностики, и
автодополнение, при этом в панели Output ошибок нет.

Эти флаги есть только в форке `clangd` от автора расширения.

Рабочий набор аргументов:

```json
"clangd.arguments": [
  "--background-index",
  "--clang-tidy=false",
  "--header-insertion=never",
  "--query-driver=arm-none-eabi-gcc*"
]
```

### Диагностика clangd из командной строки

```sh
clangd --check=src/main.c --compile-commands-dir=.vscode
```

Показывает все диагностики разом, без ограничения "Too many errors".
Требует `clangd` в PATH — путь к нему задаётся в настройках расширения
(`clangd.path`) и указывает на бинарь в `globalStorage` расширения.