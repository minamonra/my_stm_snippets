#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
build_analyzer.py v2.0
Универсальный анализатор памяти для встраиваемых проектов (ARM / RISC-V).

Возможности:
  * Разбор линкерного скрипта (.ld): блок MEMORY и распределение секций по регионам.
  * Генерация .vscode/memory_config.json (карта памяти).
  * Разбор ELF через <size-tool> -A.
  * Расчёт занятой памяти по каждому региону с учётом AT> (копируемых секций).
  * Красивый отчёт в терминале.

Примеры:
  # Полный анализ (конфиг пересоздастся, если его нет)
  python .vscode/py_scripts/build_analyzer.py \
      --ld core/stm32f103c8tx_flash.ld \
      --elf build/main.elf \
      --map build/main.map \
      --config .vscode/memory_config.json

  # Только пересоздать конфиг памяти из .ld
  python .vscode/py_scripts/build_analyzer.py \
      --ld core/stm32f103c8tx_flash.ld \
      --config .vscode/memory_config.json --regen

  # Подробный вывод (сырой вывод size -A)
  python .vscode/py_scripts/build_analyzer.py --details
"""

import argparse
import json
import os
import re
import subprocess
import sys

# ---------------------------------------------------------------------------
# Цвета
# ---------------------------------------------------------------------------

COLORS = {
    "reset":   "\033[0m",
    "bold":    "\033[1m",
    "dim":     "\033[2m",
    "red":     "\033[91m",
    "green":   "\033[92m",
    "yellow":  "\033[93m",
    "blue":    "\033[94m",
    "cyan":    "\033[96m",
    "white":   "\033[97m",
}
COLOR_ON = True


def enable_vt():
    """Включить поддержку ANSI-последовательностей в консоли Windows."""
    if os.name != "nt":
        return
    try:
        import ctypes
        kernel32 = ctypes.windll.kernel32
        handle = kernel32.GetStdHandle(-11)
        mode = ctypes.c_uint32()
        if kernel32.GetConsoleMode(handle, ctypes.byref(mode)):
            kernel32.SetConsoleMode(handle, mode.value | 0x0004)
    except Exception:
        pass


def colorize(text, *names):
    if not COLOR_ON:
        return text
    prefix = "".join(COLORS[n] for n in names if n in COLORS)
    if not prefix:
        return text
    return prefix + text + COLORS["reset"]


def hrule(char="-", width=85):
    return char * width


# ---------------------------------------------------------------------------
# Вспомогательные функции
# ---------------------------------------------------------------------------

def parse_memory_size(value):
    """'64K' -> 65536, '20K' -> 20480, '1M' -> 1048576."""
    m = re.search(r"([0-9]+)\s*([KMG]?)", str(value), re.IGNORECASE)
    if not m:
        return 0
    number = int(m.group(1))
    unit = (m.group(2) or "").upper()
    return number * {"K": 1024, "M": 1024 * 1024, "G": 1024 ** 3}.get(unit, 1)


def format_hex_address(value):
    """Приводит адрес к виду 0xXXXXXXXX."""
    try:
        text = str(value).strip()
        number = int(text, 16) if text.lower().startswith("0x") else int(text)
        return "0x%08X" % number
    except (ValueError, TypeError):
        return str(value)


def strip_comments(text):
    """Удаляет /* ... */ и // ... из линкерного скрипта."""
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    text = re.sub(r"//[^\n]*", "", text)
    return text


def extract_block(content, keyword):
    """Достаёт содержимое блока KEYWORD { ... } с учётом вложенных скобок."""
    match = re.search(r"^\s*" + keyword + r"\s*\{", content,
                      re.IGNORECASE | re.MULTILINE)
    if not match:
        return None
    start = match.end() - 1
    depth = 0
    for index in range(start, len(content)):
        char = content[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return content[start + 1:index]
    return None


def classify_region(attributes):
    """Определяет тип региона памяти по атрибутам из .ld."""
    attrs = (attributes or "").lower()
    if "rx" in attrs:
        return "FLASH"
    if "rw" in attrs or "xrw" in attrs or "w" in attrs:
        return "RAM"
    return "OTHER"


# ---------------------------------------------------------------------------
# Разбор линкерного скрипта
# ---------------------------------------------------------------------------

def parse_ld(path):
    """Возвращает {'memory_blocks': {...}, 'sections': {...}} из .ld файла."""
    with open(path, "r", encoding="utf-8", errors="replace") as handle:
        raw = handle.read()
    content = strip_comments(raw)

    # --- 1. Блок MEMORY -----------------------------------------------------
    memory_blocks = {}
    memory_body = extract_block(content, "MEMORY")
    if memory_body:
        entry_re = re.compile(
            r"([A-Za-z_]\w*)\s*\(([^)]*)\)\s*:\s*"
            r"ORIGIN\s*=\s*([^,]+?)\s*,\s*LENGTH\s*=\s*([0-9]+[KMGkmg]?)"
        )
        for match in entry_re.finditer(memory_body):
            name = match.group(1)
            attributes = match.group(2).strip()
            origin = format_hex_address(match.group(3))
            length = match.group(4)
            memory_blocks[name] = {
                "origin": origin,
                "length": length,
                "bytes": parse_memory_size(length),
                "attributes": attributes,
                "type": classify_region(attributes),
            }

    # --- 2. Блок SECTIONS --------------------------------------------------
    sections = {}
    sections_body = extract_block(content, "SECTIONS")
    if sections_body:
        starts = list(re.finditer(r"^[ \t]*(\.[\w.]+)[ \t]*:", sections_body,
                                  re.MULTILINE))
        for index, match in enumerate(starts):
            name = match.group(1)
            end = starts[index + 1].start() if index + 1 < len(starts) \
                else len(sections_body)
            chunk = sections_body[match.end():end]

            at_match = re.search(r">[ \t]*([A-Za-z_]\w*)[ \t]+AT>[ \t]*([A-Za-z_]\w*)",
                                 chunk)
            if at_match:
                exec_mem, load_mem = at_match.group(1), at_match.group(2)
            else:
                plain = re.search(r">[ \t]*([A-Za-z_]\w*)\b", chunk)
                if not plain:
                    continue
                exec_mem, load_mem = plain.group(1), None

            if exec_mem not in memory_blocks:
                continue

            if load_mem and load_mem != exec_mem:
                sections[name] = {
                    "location": exec_mem,
                    "load_memory": load_mem,
                    "type": "%s -> %s" % (load_mem, exec_mem),
                    "description": "Stored in %s, executed in %s"
                                   % (load_mem, exec_mem),
                    "memory_type": memory_blocks[exec_mem]["type"],
                }
            else:
                sections[name] = {
                    "location": exec_mem,
                    "load_memory": load_mem or exec_mem,
                    "type": exec_mem,
                    "description": "Only in %s" % exec_mem,
                    "memory_type": memory_blocks[exec_mem]["type"],
                }

    return {"memory_blocks": memory_blocks, "sections": sections}


# ---------------------------------------------------------------------------
# Конфиг памяти
# ---------------------------------------------------------------------------

def write_config(config, path):
    directory = os.path.dirname(os.path.abspath(path))
    if directory:
        os.makedirs(directory, exist_ok=True)
    with open(path, "w", encoding="utf-8") as handle:
        json.dump(config, handle, ensure_ascii=False, indent=2)


def load_config(path):
    """Читает memory_config.json. Поддерживает и список секций, и словарь."""
    with open(path, "r", encoding="utf-8") as handle:
        data = json.load(handle)

    blocks = {}
    for name, info in (data.get("memory_blocks") or {}).items():
        length = info.get("length", "")
        if not length:
            # Восстанавливаем из bytes, если length нет
            length = "%dK" % round(info.get("bytes", 0) / 1024.0) \
                if info.get("bytes", 0) % 1024 == 0 else str(info.get("bytes", 0))
        blocks[name] = {
            "origin": info.get("origin", "0x00000000"),
            "length": length,
            "bytes": info.get("bytes", parse_memory_size(length)),
            "attributes": info.get("attributes", ""),
            "type": info.get("type", "OTHER"),
        }

    raw_sections = data.get("sections") or {}
    sections = {}
    if isinstance(raw_sections, dict):
        for name, info in raw_sections.items():
            sections[name] = {
                "location": info.get("location", ""),
                "load_memory": info.get("load_memory", info.get("location", "")),
                "type": info.get("type", ""),
            }
    else:  # старый формат — список словарей
        for info in raw_sections:
            sections[info["name"]] = {
                "location": info.get("location", ""),
                "load_memory": info.get("load_memory", info.get("location", "")),
                "type": info.get("type", ""),
            }

    return {"memory_blocks": blocks, "sections": sections}


# ---------------------------------------------------------------------------
# Разбор ELF
# ---------------------------------------------------------------------------

NON_ALLOC_PREFIXES = (
    ".debug", ".comment", ".ARM.attributes", ".symtab", ".strtab",
    ".shstrtab", ".note", ".gnu", ".stab", ".line",
)


def is_alloc_section(name):
    return not any(name.startswith(prefix) for prefix in NON_ALLOC_PREFIXES)


def run_size(size_tool, elf_file, details=False):
    """Запускает <size-tool> -A и возвращает (сырой_вывод, {секция: размер})."""
    try:
        result = subprocess.run([size_tool, "-A", elf_file],
                                capture_output=True, text=True, check=True)
    except FileNotFoundError:
        raise RuntimeError(
            "Инструмент '%s' не найден в PATH. Укажите полный путь через "
            "--size-tool." % size_tool)
    except subprocess.CalledProcessError as exc:
        raise RuntimeError("Ошибка запуска '%s': %s" % (size_tool, exc.stderr.strip()))

    output = result.stdout
    if details:
        print(colorize("\nДетальная информация о памяти:", "bold", "cyan"))
        print(output)

    sections = {}
    for line in output.splitlines():
        line = line.strip()
        if not line or not line.startswith("."):
            continue
        match = re.match(r"^(\.[\w.]+)\s+(\d+)", line)
        if match:
            sections[match.group(1)] = int(match.group(2))

    return output, sections


def compute_usage(elf_sections, config):
    """Считает занятые байты по регионам памяти."""
    used = {name: 0 for name in config["memory_blocks"]}
    details = []
    unknown = []

    for name in sorted(elf_sections):
        size = elf_sections[name]
        if size == 0 or not is_alloc_section(name):
            continue

        info = config["sections"].get(name)
        if not info:
            unknown.append((name, size))
            continue

        location = info.get("location") or ""
        load_memory = info.get("load_memory") or location

        used[location] = used.get(location, 0) + size
        if load_memory and load_memory != location:
            used[load_memory] = used.get(load_memory, 0) + size

        details.append((name, size, location, load_memory))

    return used, details, unknown


# ---------------------------------------------------------------------------
# Отчёт
# ---------------------------------------------------------------------------

def print_report(used, details, config):
    print()
    print(colorize("Memory Regions:", "bold", "green"))
    header = ("%-10s %-15s %-15s %-10s %-10s %-10s %-10s"
              % ("Region", "Start address", "End address",
                 "Size", "Free", "Used", "Usage (%)"))
    print(colorize(header, "bold"))
    print(colorize(hrule(), "green"))

    for name in sorted(config["memory_blocks"]):
        info = config["memory_blocks"][name]
        total = info["bytes"]
        block_used = used.get(name, 0)
        block_free = total - block_used
        percent = round((block_used / total) * 100, 2) if total else 0.0

        if percent >= 90:
            tone = "red"
        elif percent >= 75:
            tone = "yellow"
        else:
            tone = "white"

        start_int = int(info["origin"], 16)
        end_addr = "0x%08X" % (start_int + total)

        row = ("%-10s %-15s %-15s %-10s %-10s %-10s %-10s"
               % (name + ":", info["origin"], end_addr,
                  "%.0f KB" % (total / 1024.0),
                  "%.2f KB" % (block_free / 1024.0),
                  "%.2f KB" % (block_used / 1024.0),
                  "%.2f%%" % percent))
        print(colorize(row, tone))

    # Детализация по секциям
    if details:
        print()
        print(colorize("Распределение по секциям:", "bold", "cyan"))
        print(colorize("%-24s %-10s %-10s %-8s" %
                        ("Section", "Size", "Region", "Load"), "dim"))
        for name, size, location, load_memory in details:
            load_text = load_memory if load_memory != location else "-"
            print("%-24s %-10s %-10s %-8s"
                  % (name, "%d B" % size, location, load_text))

        top = sorted(details, key=lambda item: item[1], reverse=True)[:5]
        if top:
            print()
            print(colorize("Крупнейшие секции:", "bold", "cyan"))
            for name, size, location, _ in top:
                print("  %-22s %8d B  -> %s" % (name, size, location))


def print_config_summary(config):
    print(colorize("Memory blocks:", "bold", "cyan"))
    for name in sorted(config["memory_blocks"]):
        info = config["memory_blocks"][name]
        print("  %-10s ORIGIN=%-12s LENGTH=%-6s (%d bytes, %s)"
              % (name, info["origin"], info.get("length", "?"),
                 info["bytes"], info["type"]))
    print(colorize("Секций в карте: %d" % len(config["sections"]), "dim"))


# ---------------------------------------------------------------------------
# main
# ---------------------------------------------------------------------------

def main():
    global COLOR_ON

    parser = argparse.ArgumentParser(
        description="Анализатор памяти для встраиваемых проектов")
    parser.add_argument("--ld", dest="ld_file",
                        default="core/stm32f103c8tx_flash.ld",
                        help="Путь к линкерному скрипту (.ld)")
    parser.add_argument("--elf", dest="elf_file", default="build/main.elf",
                        help="Путь к ELF-файлу (если не задан — только конфиг)")
    parser.add_argument("--map", dest="map_file", default="build/main.map",
                        help="Путь к .map файлу (для справки)")
    parser.add_argument("--config", dest="config_file",
                        default=".vscode/memory_config.json",
                        help="Путь к memory_config.json")
    parser.add_argument("--size-tool", dest="size_tool",
                        default="arm-none-eabi-size",
                        help="Путь к утилите size")
    parser.add_argument("--regen", action="store_true",
                        help="Принудительно пересоздать конфиг из .ld")
    parser.add_argument("--details", action="store_true",
                        help="Показать сырой вывод size -A")
    parser.add_argument("--no-color", action="store_true",
                        help="Отключить цветной вывод")
    parser.add_argument("--json", dest="json_out",
                        help="Дублировать отчёт в JSON-файл")
    args = parser.parse_args()

    if args.no_color:
        COLOR_ON = False
    else:
        enable_vt()

    print(colorize("=" * 85, "bold", "green"))
    print(colorize(" Build Analyzer v2.0", "bold", "green"))
    print(colorize("=" * 85, "bold", "green"))

    # --- 1. Конфиг памяти --------------------------------------------------
    config = None
    if os.path.exists(args.config_file) and not args.regen:
        try:
            config = load_config(args.config_file)
            print(colorize("[OK]  Конфиг памяти загружен: %s" % args.config_file,
                           "green"))
        except Exception as exc:
            print(colorize("[WARN] Не удалось прочитать %s: %s"
                           % (args.config_file, exc), "yellow"))
            config = None

    if config is None:
        if not os.path.exists(args.ld_file):
            print(colorize("[ERR] Линкерный скрипт не найден: %s" % args.ld_file,
                           "red", "bold"))
            return 2
        try:
            raw_config = parse_ld(args.ld_file)
        except Exception as exc:
            print(colorize("[ERR] Ошибка разбора %s: %s" % (args.ld_file, exc),
                           "red", "bold"))
            return 2

        if not raw_config["memory_blocks"]:
            print(colorize("[ERR] В %s не найден блок MEMORY" % args.ld_file,
                           "red", "bold"))
            return 2

        write_config(raw_config, args.config_file)
        config = load_config(args.config_file)
        print(colorize("[OK]  Конфиг создан из %s -> %s"
                       % (args.ld_file, args.config_file), "green"))

    print_config_summary(config)

    # --- 2. Без ELF — только конфиг ----------------------------------------
    if not args.elf_file:
        return 0

    if not os.path.exists(args.elf_file):
        print(colorize("[ERR] ELF-файл не найден: %s" % args.elf_file,
                       "red", "bold"))
        return 3

    if args.map_file and not os.path.exists(args.map_file):
        print(colorize("[WARN] .map файл не найден: %s" % args.map_file,
                       "yellow"))

    # --- 3. Разбор ELF -----------------------------------------------------
    try:
        _, elf_sections = run_size(args.size_tool, args.elf_file, args.details)
    except RuntimeError as exc:
        print(colorize("[ERR] %s" % exc, "red", "bold"))
        return 4

    if not elf_sections:
        print(colorize("[ERR] size не вернул ни одной секции", "red", "bold"))
        return 4

    used, details, unknown = compute_usage(elf_sections, config)

    print_report(used, details, config)

    if unknown:
        print()
        print(colorize("Секции вне карты памяти (не учтены):", "yellow"))
        for name, size in unknown:
            print("  %-24s %d B" % (name, size))

    # --- 4. JSON-отчёт -----------------------------------------------------
    if args.json_out:
        report = {
            "memory_blocks": config["memory_blocks"],
            "usage": {name: used.get(name, 0)
                      for name in config["memory_blocks"]},
            "sections": [
                {"name": n, "size": s, "location": loc, "load_memory": load}
                for n, s, loc, load in details
            ],
        }
        with open(args.json_out, "w", encoding="utf-8") as handle:
            json.dump(report, handle, ensure_ascii=False, indent=2)
        print()
        print(colorize("[OK]  JSON-отчёт: %s" % args.json_out, "green"))

    return 0


if __name__ == "__main__":
    sys.exit(main())