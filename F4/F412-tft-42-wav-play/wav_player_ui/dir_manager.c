#include "dir_manager.h"
#include <string.h>

static DIR root_dj;
static FILINFO root_fno;
static DIR sub_dj;
static FILINFO sub_fno;

// Вспомогательная функция: проверяет, есть ли внутри папки WAV-файлы
static uint8_t has_wav_files(const char* dir_path) {
    if (f_opendir(&sub_dj, dir_path) != FR_OK) return 0;
    uint8_t found = 0;
    while (f_readdir(&sub_dj, &sub_fno) == FR_OK && sub_fno.fname[0] != '\0') {
        if (!(sub_fno.fattrib & AM_DIR)) {
            char* ext = strrchr(sub_fno.fname, '.');
            if (ext && strcasecmp(ext, ".WAV") == 0) {
                found = 1;
                break;
            }
        }
    }
    f_closedir(&sub_dj);
    return found;
}

// Считает общее количество папок с музыкой в корне
uint16_t dir_manager_scan_total(void) {
    uint16_t dir_count = 0;
    if (f_opendir(&root_dj, ROOT_DIR) != FR_OK) return 0;

    char tmp_path[256];
    while (f_readdir(&root_dj, &root_fno) == FR_OK && root_fno.fname[0] != '\0') {
        if (root_fno.fattrib & AM_DIR) {
            // Формируем путь "0:/ИмяПапки"
            strcpy(tmp_path, ROOT_DIR);
            strcat(tmp_path, root_fno.fname);
            
            if (has_wav_files(tmp_path)) {
                dir_count++;
            }
        }
    }
    f_closedir(&root_dj);
    return dir_count;
}

// Ищет папку по индексу и возвращает её путь (например, "0:/ROCK")
uint8_t dir_manager_get_path_by_index(uint16_t target_idx, char* out_buf, uint32_t buf_size) {
    uint16_t current_idx = 0;
    if (f_opendir(&root_dj, ROOT_DIR) != FR_OK) return 0;

    uint8_t success = 0;
    while (f_readdir(&root_dj, &root_fno) == FR_OK && root_fno.fname[0] != '\0') {
        if (root_fno.fattrib & AM_DIR) {
            // Временно собираем путь в выходной буфер для проверки
            strncpy(out_buf, ROOT_DIR, buf_size);
            strncat(out_buf, root_fno.fname, buf_size - strlen(out_buf) - 1);
            
            if (has_wav_files(out_buf)) {
                if (current_idx == target_idx) {
                    success = 1; // Нашли нужную папку! Путь уже лежит в out_buf
                    break;
                }
                current_idx++;
            }
        }
    }
    f_closedir(&root_dj);
    return success;
}
