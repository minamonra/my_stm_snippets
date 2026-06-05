@echo off
chcp 65001 > nul
echo ===================================================
echo   ОЧИСТКА CMSIS PACK ПОД СТАНДАРТ SEGGER (Cortex-M4)  
echo ===================================================

:: Создаем временную структуру для сохранения критических файлов
mkdir _temp_inc
mkdir _temp_inc\m-profile
mkdir _temp_src

echo [1/3] Копирование нужных файлов (включая m-profile ядра M4)...

:: Сохраняем файлы ядра Cortex-M4 и базовой спецификации ARM
copy "inc\core_cm4.h" "_temp_inc\" > nul
copy "inc\cmsis_compiler.h" "_temp_inc\" > nul
copy "inc\cmsis_gcc.h" "_temp_inc\" > nul
copy "inc\cmsis_version.h" "_temp_inc\" > nul

:: ЖЕСТКАЯ ФИКСАЦИЯ: Сохраняем зависимости ядра M4 (ARMv7-M) и компилятора GCC
if exist "inc\m-profile\cmsis_gcc_m.h" copy "inc\m-profile\cmsis_gcc_m.h" "_temp_inc\m-profile\" > nul
if exist "inc\m-profile\armv7m_mpu.h"  copy "inc\m-profile\armv7m_mpu.h"  "_temp_inc\m-profile\" > nul
if exist "inc\m-profile\armv7m_cachel1.h" copy "inc\m-profile\armv7m_cachel1.h" "_temp_inc\m-profile\" > nul

:: Сохраняем заголовочники под вашу серию STM32F4
copy "inc\stm32f4xx.h" "_temp_inc\" > nul
copy "inc\system_stm32f4xx.h" "_temp_inc\" > nul

:: Сохраняем целевые МК (и F411, и F412)
if exist "inc\stm32f411xe.h" copy "inc\stm32f411xe.h" "_temp_inc\" > nul
if exist "inc\stm32f412cx.h" copy "inc\stm32f412cx.h" "_temp_inc\" > nul

:: Сохраняем исходники из папки src (стартап и системные часы)
if exist "src\system_stm32f4xx.c" copy "src\system_stm32f4xx.c" "_temp_src\" > nul
if exist "src\startup_stm32f411xe.s" copy "src\startup_stm32f411xe.s" "_temp_src\" > nul
if exist "src\startup_stm32f412cx.s" copy "src\startup_stm32f412cx.s" "_temp_src\" > nul

echo [2/3] Удаление мусора (остальные ядра, папки IAR, Keil, TI)...

:: Сносим старые папки целиком
rmdir /s /q inc
rmdir /s /q src

:: Возвращаем чистые папки обратно
mkdir inc
mkdir src
mkdir inc\m-profile

echo [3/3] Восстановление чистой структуры проекта...
xcopy _temp_inc inc /E /Y > nul
xcopy _temp_src src /E /Y > nul

:: Удаляем временные папки
rmdir /s /q _temp_inc
rmdir /s /q _temp_src

echo ===================================================
echo   ГОТОВО! Оставлен только чистый Cortex-M4 GCC Набор.
echo ===================================================
pause