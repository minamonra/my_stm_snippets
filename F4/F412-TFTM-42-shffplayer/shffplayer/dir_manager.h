#ifndef DIR_MANAGER_H
#define DIR_MANAGER_H

#include "ff.h"
#include <stdint.h>

#define ROOT_DIR "0:/"

// Функция инициализации менеджера папок (считает сколько всего WAV-папок в корне)
uint16_t dir_manager_scan_total(void);

// Функция, которая находит папку по её порядковому номеру (индексу) и копирует её полный путь в out_buf
uint8_t dir_manager_get_path_by_index(uint16_t target_idx, char* out_buf, uint32_t buf_size);

#endif // DIR_MANAGER_H
