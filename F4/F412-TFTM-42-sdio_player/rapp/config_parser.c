#include "config_parser.h"
#include "ff.h"
#include <string.h>

// АВТОМАТИЧЕСКАЯ СБОРКА ПУТИ ПОД ДВУЗНАЧНЫЙ ФОРМАТ ПАПОК (Например, "01/config.bin")
static void Form_Bin_Path(uint16_t route_num, char* path) {
    path[0] = '0' + (route_num / 10) % 10; // Десятки номера папки
    path[1] = '0' + (route_num % 10);      // Единицы номера папки
    path[2] = '/';
    path[3] = 'c';  path[4] = 'o';  path[5] = 'n';  path[6] = 'f';
    path[7] = 'i';  path[8] = 'g';  path[9] = '.';  path[10] = 'b';
    path[11] = 'i'; path[12] = 'n'; path[13] = '\0'; // Строго закрываем строку Си!
}

// 1. ЧТЕНИЕ ОБЩЕЙ ИНФОРМАЦИИ МАРШРУТА (Читает заголовок файла)
uint8_t Config_ReadRouteInfo(uint16_t route_num, RouteInfo_t* out_info) {
    FIL file;
    char path[32];
    UINT br;
    Form_Bin_Path(route_num, path); // Собрали путь "01/config.bin"

    if (f_open(&file, path, FA_READ) != FR_OK) return 1; // Ошибка: Папка или файл не найдены

    // КРИТИЧЕСКИЙ ФИКС: Читаем строго 208 байт шапки, как пишет ПК-редактор!
    // Защищает FatFS от бесконечного зависания и удержания шины SDIO
    if (f_read(&file, out_info, 208, &br) != FR_OK || br != 208) {
        f_close(&file);
        return 2; // Ошибка: Файл поврежден или обрезан
    }

    f_close(&file);
    return 0; // Успешно прочитано!
}

// 2. КАСКАДНЫЙ ПОИСК КОНКРЕТНОЙ СТАНЦИИ ПО ИНДЕКСУ И ТИПУ ОБЪЯВЛЕНИЯ
uint8_t Config_LoadStation(uint16_t route_num, char label_char, uint16_t sub_idx, uint8_t announce_type, char* out_wav, char* out_text, char* out_panel_text, uint16_t* out_global_id) {
    FIL file;
    char path[32];
    UINT br;
    RouteInfo_t header;
    Form_Bin_Path(route_num, path);

    if (f_open(&file, path, FA_READ) != FR_OK) return 1;

    // Сначала вычитываем 208 байт шапки, чтобы узнать total_records (сколько всего строк в таблице)
    if (f_read(&file, &header, 208, &br) != FR_OK || br != 208) {
        f_close(&file);
        return 2;
    }

    BinAnnounce_t current_announce;
    uint8_t found = 0;

    // Поблочно перебираем структуры объявлений — каждая строго по 264 байта на диске!
    for (uint16_t i = 0; i < header.total_records; i++) {
        if (f_read(&file, &current_announce, 264, &br) != FR_OK || br != 264) {
            break; // Досрочный выход, если файл неожиданно закончился
        }

        // ПРОВЕРКА СТРОГОГО СОВПАДЕНИЯ ТРЕХ КРИТЕРИЕВ: Порядковый номер + Направление (F/B/S) + Тип объявления
        if (current_announce.sub_idx == sub_idx &&
            current_announce.direction_label == label_char &&
            current_announce.announce_type == announce_type) {

            *out_global_id = current_announce.sub_idx;

            // Безопасное копирование строк в буферы ОЗУ с жесткой фиксацией нуля на конце
            strncpy(out_wav, current_announce.wav_path, 63);
            out_wav[63] = '\0';

            strncpy(out_text, current_announce.station_text, 127);
            out_text[127] = '\0';

            strncpy(out_panel_text, current_announce.car_panel_text, 63);
            out_panel_text[63] = '\0';

            found = 1; // Ура, станция найдена!
            break;
        }
    }

    f_close(&file);
    return found ? 0 : 2; // Возвращаем 0 если нашли, и 2 если такой записи в config.bin нет
}
