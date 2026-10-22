#!/usr/bin/env python3
"""
Генерация .vscode/compile_commands.json из Makefile.
Только стандартная библиотека (compiledb/bear не нужны).
"""
import json
import os
import shlex
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, os.pardir, os.pardir))
OUT_DIR = os.path.join(ROOT, ".vscode")
OUT_FILE = os.path.join(OUT_DIR, "compile_commands.json")

MAKE = os.environ.get("MAKE", "make")


def norm(p):
    p = os.path.normpath(p)
    if os.path.isabs(p):
        try:
            p = os.path.relpath(p, ROOT)
        except ValueError:
            pass
    return p.replace("\\", "/")


def main():
    # -n = dry run (печатает команды), -B = считать всё устаревшим
    proc = subprocess.run(
        f"{MAKE} -n -B all",
        cwd=ROOT, shell=True, capture_output=True, text=True
    )
    if proc.returncode != 0:
        sys.stdout.write(proc.stdout)
        sys.stderr.write(proc.stderr)
        return proc.returncode

    entries = []
    seen = set()

    for raw in proc.stdout.splitlines():
        line = raw.strip()
        if not line or " -c " not in line:
            continue
        try:
            toks = shlex.split(line, posix=True)
        except ValueError:
            toks = line.split()
        if "-c" not in toks:
            continue

        src = toks[toks.index("-c") + 1]
        out = ""
        if "-o" in toks:
            out = toks[toks.index("-o") + 1]

        # По спеке JSON Compilation Database: из команды убираются -c/-o,
        # путь к .d задаётся через -MD -MF
        flags = []
        k = 0
        while k < len(toks):
            t = toks[k]
            if t in ("-c", "-o"):
                k += 2
                continue
            if t == "-MF":
                k += 2
                continue
            if t in ("-MMD", "-MD", "-MP"):
                k += 1
                continue
            flags.append(t)
            k += 1

        f = norm(src)
        if f in seen:
            continue
        seen.add(f)

        entry = {
            "directory": ROOT.replace("\\", "/"),
            "file": f,
            "command": " ".join(flags),
        }
        if out:
            entry["output"] = norm(out)
            dep = norm(out)[:-2] + ".d"
            entry["command"] += " -MD -MF " + dep
        entries.append(entry)

    os.makedirs(OUT_DIR, exist_ok=True)
    with open(OUT_FILE, "w", encoding="utf-8") as fh:
        json.dump(entries, fh, indent=2)

    print(f"[compile_commands] {len(entries)} entries -> {norm(OUT_FILE)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())