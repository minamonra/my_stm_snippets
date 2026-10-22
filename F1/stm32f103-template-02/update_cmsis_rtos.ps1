<#
    Обновление CMSIS-Core и ядра FreeRTOS в шаблоне STM32F103C8T6.

    Копирует только файлы, нужные для Cortex-M3 + GCC.
    Для clangd генерирует обёртку core\cmsis_clang.h (Arm Compiler 6 не нужен).

        pwsh -ExecutionPolicy Bypass -File .\update_cmsis_rtos.ps1 -DryRun
        pwsh -ExecutionPolicy Bypass -File .\update_cmsis_rtos.ps1
        pwsh -ExecutionPolicy Bypass -File .\update_cmsis_rtos.ps1 -Backup
#>

[CmdletBinding()]
param(
    [string] $CmsisInclude   = "C:\_0GitHub\CMSIS_6\CMSIS\Core\Include",
    [string] $FreeRtosKernel = "C:\_0GitHub\FreeRTOS-Kernel",
    [string] $ProjectRoot    = $PSScriptRoot,
    [switch] $SkipCmsis,
    [switch] $SkipRtos,
    [switch] $Backup,
    [switch] $DryRun
)

$ErrorActionPreference = "Stop"

$TargetCore = Join-Path $ProjectRoot "core"
$TargetRtos = Join-Path $ProjectRoot "rtos"
$BackupDir  = Join-Path $ProjectRoot ".cmsis_backup"

# Проектные файлы, которые нельзя удалять
$Protected = @("stm32f103c8tx_flash.ld", "stm32f103.svd")

# --- вывод -----------------------------------------------------------------
function Info ($m) { Write-Host $m -ForegroundColor Cyan }
function Ok   ($m) { Write-Host $m -ForegroundColor Green }
function Warn ($m) { Write-Host $m -ForegroundColor Yellow }
function Err  ($m) { Write-Host $m -ForegroundColor Red }
function Title ($m) {
    Write-Host ""
    Write-Host ("=" * 72) -ForegroundColor DarkGray
    Write-Host "  $m" -ForegroundColor White
    Write-Host ("=" * 72) -ForegroundColor DarkGray
}

function Rel {
    param([string]$Base, [string]$Full)
    $sep = [IO.Path]::DirectorySeparatorChar
    $u = [Uri]::new($Base.TrimEnd('\','/') + $sep)
    return [Uri]::UnescapeDataString($u.MakeRelativeUri([Uri]::new($Full)).ToString()).Replace('/', $sep)
}

# ============================================================================
# БЕЛЫЕ СПИСКИ  (только Cortex-M3 + GCC)
# ============================================================================

function Get-CmsisFiles {
    param([string]$Src)
    $list = @()

    # Ядро CMSIS для Cortex-M3 + GCC
    foreach ($f in @("core_cm3.h", "cmsis_compiler.h", "cmsis_gcc.h", "cmsis_version.h")) {
        if (Test-Path (Join-Path $Src $f)) { $list += $f }
    }

    # M-профиль: только GCC-реализация
    if (Test-Path (Join-Path $Src "m-profile\cmsis_gcc_m.h")) {
        $list += "m-profile\cmsis_gcc_m.h"
    }

    # НЕ копируем:
    #   cmsis_clang.h, m-profile\cmsis_clang_m.h - рассчитаны на Arm Compiler 6,
    #     ломают clangd (41 ошибка). clangd работает в режиме GCC (-U__clang__).
    #   m-profile\armv7m_mpu.h - STM32F103C8T6 = Cortex-M3 БЕЗ MPU,
    #     файл не включается в сборку, но ломает разбор clangd (MPU undeclared).

    return $list
}

function Get-RtosFiles {
    param([string]$Src)
    $list = @()

    Get-ChildItem $Src -File -Filter "*.c" -ErrorAction SilentlyContinue |
        ForEach-Object { $list += $_.Name }

    $inc = Join-Path $Src "include"
    if (Test-Path $inc) {
        Get-ChildItem $inc -Recurse -File -ErrorAction SilentlyContinue | ForEach-Object {
            $rel = Rel $inc $_.FullName
            if ($rel -like "*.readme" -or $rel -like "CMakeLists.txt") { return }
            $list += ("include\" + $rel)
        }
    }

    $cm3 = Join-Path $Src "portable\GCC\ARM_CM3"
    if (Test-Path $cm3) {
        Get-ChildItem $cm3 -Recurse -File | ForEach-Object {
            $list += ("portable\GCC\ARM_CM3\" + (Rel $cm3 $_.FullName))
        }
    }

    $mm = Join-Path $Src "portable\MemMang"
    if (Test-Path $mm) {
        Get-ChildItem $mm -File -Filter "heap_4.c" -ErrorAction SilentlyContinue |
            ForEach-Object { $list += ("portable\MemMang\" + $_.Name) }
    }

    return $list
}


# ============================================================================
# СИНХРОНИЗАЦИЯ ПО БЕЛОМУ СПИСКУ
# ============================================================================

function Sync-List {
    param(
        [string]   $Source,
        [string]   $Target,
        [string[]] $Files,
        [string[]] $Keep    = @(),
        [switch]   $Backup,
        [switch]   $DryRun
    )

    $added = 0; $updated = 0; $same = 0; $removed = 0
    $set = @{}

    foreach ($rel in $Files) {
        $src = Join-Path $Source $rel
        $dst = Join-Path $Target $rel
        if (-not (Test-Path $src)) { continue }
        $set[$rel.ToLowerInvariant()] = $true

        if (Test-Path $dst) {
            $si = Get-Item $src; $di = Get-Item $dst
            if ($si.Length -eq $di.Length -and $si.LastWriteTimeUtc -le $di.LastWriteTimeUtc) {
                $same++
                continue
            }
            if (-not $DryRun) { Copy-Item $src $dst -Force }
            $updated++; Ok "  ~ обновлён : $rel"
        } else {
            if (-not $DryRun) {
                $d = Split-Path $dst -Parent
                if (-not (Test-Path $d)) { New-Item -ItemType Directory -Path $d -Force | Out-Null }
                Copy-Item $src $dst -Force
            }
            $added++; Ok "  + добавлен : $rel"
        }
    }

    Get-ChildItem $Target -Recurse -File -ErrorAction SilentlyContinue | ForEach-Object {
        $rel = Rel $Target $_.FullName
        if ($set.ContainsKey($rel.ToLowerInvariant())) { return }
        if ($Keep -contains $rel) { Info "  = оставлен : $rel (защищён)"; return }

        if ($Backup) {
            $bak = Join-Path $BackupDir $rel
            if (-not $DryRun) {
                $bd = Split-Path $bak -Parent
                if (-not (Test-Path $bd)) { New-Item -ItemType Directory -Path $bd -Force | Out-Null }
                Move-Item $_.FullName $bak -Force
            }
            Warn "  - в backup : $rel"
        } else {
            if (-not $DryRun) { Remove-Item $_.FullName -Force }
            Warn "  - удалён  : $rel"
        }
        $script:__rm++
    }

    if (-not $DryRun) {
        Get-ChildItem $Target -Recurse -Directory -ErrorAction SilentlyContinue |
            Sort-Object { $_.FullName.Length } -Descending | ForEach-Object {
                if (@(Get-ChildItem $_.FullName -Force -ErrorAction SilentlyContinue).Count -eq 0) {
                    Remove-Item $_.FullName -Force
                    Warn "  - удалён каталог : $(Rel $Target $_.FullName)"
                }
            }
    }

    Write-Host ""
    Info "  Итог: +$added  ~$updated  =$same  -$removed"
}

# ============================================================================
# ОСНОВНОЙ СЦЕНАРИЙ
# ============================================================================

Title "Обновление CMSIS / FreeRTOS (STM32F103C8T6, Cortex-M3 + GCC)"

Info "CMSIS-Core : $CmsisInclude"
Info "FreeRTOS   : $FreeRtosKernel"
Info "Проект     : $ProjectRoot"
if ($DryRun) { Warn "РЕЖИМ ПРОСМОТРА — файлы не изменяются" }
Write-Host ""

if (-not $SkipCmsis) {
    if (-not (Test-Path $CmsisInclude)) {
        Err "НЕ НАЙДЕН: $CmsisInclude"
        Get-ChildItem (Split-Path $CmsisInclude -Parent) -ErrorAction SilentlyContinue |
            ForEach-Object { Write-Host "    $($_.FullName)" }
    } else {
        Title "CMSIS-Core  ->  core\"
        $f = Get-CmsisFiles $CmsisInclude
        Info "  Файлов в белом списке: $($f.Count)"
        Sync-List -Source $CmsisInclude -Target $TargetCore -Files $f `
                   -Keep $Protected -Backup:$Backup -DryRun:$DryRun
        Write-Host ""
    }
} else { Warn "CMSIS пропущен" }

if (-not $SkipRtos) {
    Write-Host ""
    Title "FreeRTOS  ->  rtos\"
    if (-not (Test-Path $FreeRtosKernel)) {
        Err "НЕ НАЙДЕН: $FreeRtosKernel"
        Get-ChildItem (Split-Path $FreeRtosKernel -Parent) -ErrorAction SilentlyContinue |
            ForEach-Object { Write-Host "    $($_.FullName)" }
    } else {
        $f = Get-RtosFiles $FreeRtosKernel
        Info "  Файлов в белом списке: $($f.Count)"
        Sync-List -Source $FreeRtosKernel -Target $TargetRtos -Files $f `
                   -Backup:$Backup -DryRun:$DryRun
    }
} else { Warn "FreeRTOS пропущен" }

Title "Проверка"
foreach ($p in @("core\cmsis_version.h", "rtos\include\task.h")) {
    $f = Join-Path $ProjectRoot $p
    if (Test-Path $f) {
        $hit = Select-String -Path $f -Pattern "(__CM_CMSIS_VERSION_MAIN|tskKERNEL_VERSION_NUMBER)" |
               Select-Object -First 1
        if ($hit) { Info ("  " + $hit.Line.Trim()) }
    }
}
Write-Host ""
Info "Дальше: make clean -> make all -> make ld_analyzer -> make memory_report"
Write-Host ""