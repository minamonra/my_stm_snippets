#include <string.h>
#include "route_mgr.h"
#include "ff.h"

// Глобальные переменные номеров поездов
static volatile uint16_t s_current_route_num = 1;
// ИСПРАВЛЕНО: Честный строковый массив из 4 байт под короткое имя папки (например, "01\0")
static char s_route_dir_str[4] = "01";

// Первичный запуск менеджера
void Route_Init(void) {
    s_current_route_num = 1;
    s_route_dir_str[0] = '0';
    s_route_dir_str[1] = '1';
    s_route_dir_str[2] = '\0'; // Жестко закрываем строку
}

// Выдача имени папки наружу для графического экрана
const char* Route_GetDirString(void) {
    return (const char*)s_route_dir_str;
}

// Выдача числового номера папки наружу для FatFS
uint16_t Route_GetNum(void) {
    return s_current_route_num;
}

// АВТОМАТИЧЕСКИЙ ПЕРЕБОР СЛЕДУЮЩЕГО ДОСТУПНОГО ПОЕЗДА НА ФЛЕШКЕ (По Кнопке 5)
void Route_Next(void) {
    uint16_t start_search = s_current_route_num;
    uint8_t found = 0;
    FIL test_file;
    char test_path[32];

    while (!found) {
        s_current_route_num++;
        // Жесткий лимит Си-перебора в пределах двузначного стандарта ТЗ (99 папок)
        if (s_current_route_num > 99) {
            s_current_route_num = 1;
        }

        // Собираем двузначное имя каталога с ведущим нулем (например, папка "02")
        s_route_dir_str[0] = '0' + (s_current_route_num / 10) % 10;
        s_route_dir_str[1] = '0' + (s_current_route_num % 10);
        s_route_dir_str[2] = '\0';

        // Собираем проверочный абсолютный путь для проверки физического существования папки
        test_path[0] = s_route_dir_str[0];
        test_path[1] = s_route_dir_str[1];
        test_path[2] = '/';
        test_path[3] = 'c';  test_path[4] = 'o';  test_path[5] = 'n';  test_path[6] = 'f';
        test_path[7] = 'i';  test_path[8] = 'g';  test_path[9] = '.';  test_path[10] = 'b';
        test_path[11] = 'i'; test_path[12] = 'n'; test_path[13] = '\0';

        // Пытаемся быстро приоткрыть config.bin в этой короткой папке
        if (f_open(&test_file, test_path, FA_READ) == FR_OK) {
            f_close(&test_file);
            found = 1; // Нашли! Папка существует, останавливаем перебор и переключаемся на неё!
        }
        else if (s_current_route_num == start_search) {
            // Защита: Если обошли по кругу все 99 слотов и больше ничего нет — падаем на дефолтную папку "01"
            s_current_route_num = 1;
            s_route_dir_str[0] = '0';
            s_route_dir_str[1] = '1';
            s_route_dir_str[2] = '\0';
            break;
        }
    }
}
