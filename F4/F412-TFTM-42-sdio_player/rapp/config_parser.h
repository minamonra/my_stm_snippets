#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <stdint.h>

typedef enum {
    ANNOUNCE_TRANSIT   = 0,
    ANNOUNCE_DEPARTURE = 1,
    ANNOUNCE_ARRIVAL   = 2
} AnnounceType_t;

// Бинарная структура заголовка файла (Ровно 208 байт)
typedef struct {
    uint16_t route_id;         // Идентификатор маршрута (например: 1, 2, 3, 4 вместо старых "0001"..."0004")
    char train_type[8];        // Тип поезда в UTF-8 (например: "EL\0" для электропоездов, "RA\0" для рельсовых автобусов)
    char train_number[32];     // Номер поезда/рейса в UTF-8 (например: "Поезд 6306\0")
    char station_start[64];    // Начальная станция маршрута в UTF-8 (например: "Южная\0")
    char station_end[64];      // Конечная станция маршрута в UTF-8 (например: "Тракторная пас\0")
    char head_panel_text[32];  // Текст для внешнего лобового табло поезда в UTF-8 (скрипт заполняет нулями b'\x00')
    uint16_t total_records;    // Общее количество записей объявлений в данном файле (например: 47)
    uint8_t reserved[2];       // Технический паддинг для кратности размера структуры 4 байтам (всегда заполнен 0x00)
} __attribute__((packed)) RouteInfo_t;

// Бинарная структура одной записи объявления (Ровно 264 байта)
typedef struct {
    uint16_t sub_idx;           // Порядковый сквозной индекс объявления (например: 1, 2, 3...47)
    char direction_label;       // Символ направления движения (принимает строго один байт-символ: 'F', 'B' или 'S') 
                                // тут можно ещё добавить на другие символы
    uint8_t announce_type;      // Тип объявления (0 - Транзит/Информация, 1 - Отправление, 2 - Прибытие)
    char wav_path[64];          // Относительный путь к аудиофайлу в UTF-8 (например: "forward/snd_1780393378509.wav\0")
    char station_text[128];     // Текст для ЖК-экрана в UTF-8 с вырезанным типом (например: "Нефтезаводская 06:31-06:32\0")
    char car_panel_text[64];    // Текст бегущей строки внутри вагонов по RS-485 в UTF-8 (скрипт заполняет нулями b'\x00')
    uint8_t reserved[4];        // Технический паддинг для выравнивания структуры по границе 4 байт (всегда заполнен 0x00)
} __attribute__((packed)) BinAnnounce_t;

uint8_t Config_ReadRouteInfo(uint16_t route_num, RouteInfo_t* out_info);
uint8_t Config_LoadStation(uint16_t route_num, char label_char, uint16_t sub_idx, uint8_t announce_type, char* out_wav, char* out_text, char* out_panel_text, uint16_t* out_global_id);

#endif // CONFIG_PARSER_H
