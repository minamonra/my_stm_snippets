#include "shuffle_player.h"
#include "dir_manager.h"
#include "stm32f4xx.h"
#include "common.h"
#include "audio_i2s.h"
#include "wav_player.h"
#include "sdcard.h"
#include "diskio.h"
#include "ff.h"
#include <string.h>
#include "sdcard_dma.h"
#include "lcd_io.h"
#include "lcd_draw_u8g2.h"

extern const uint8_t     u8g2_font_terminus_24b_cyr[];
extern volatile uint32_t ttms;

// ГЕОМЕТРИЯ UI
#define BAR_X 20
#define BAR_Y 135
#define BAR_W 440
#define BAR_H 16

// ЦВЕТА ИНТЕРФЕЙСА
#define TOP_PANEL_BG  GRAY   // Цвет заливки верхней плашки
#define TOP_PANEL_TXT BLACK  // Цвет текста на верхней плашке

// КООРДИНАТЫ СТРОК
#define ROW_TOP_Y    35   // Позиция текста внутри верхней плашки
#define ROW_NAME_Y   65   // Начало блока имени файла (до 3 строк)
#define ROW_TIME_Y   178  // Строка времени и статуса (под прогресс-баром)
#define ROW_UPTIME_Y 208  // Строка общего времени воспроизведения

// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ
uint32_t          g_current_seconds = 0;
uint32_t          g_total_seconds   = 0;
volatile uint32_t ram_noise_seed;

// СТРУКТУРА ДАННЫХ ИНТЕРФЕЙСА
typedef struct {
  __attribute__((aligned(4))) char text[64];
  __attribute__((aligned(4))) char path[256];
  __attribute__((aligned(4))) char name[128];
} UI_t;

// СТРУКТУРА СОСТОЯНИЯ И ЛОГИКИ ПЛЕЕРА
typedef struct {
  DIR      dir;
  FILINFO  fno;
  uint32_t pos;
  uint32_t size;
  uint32_t rate;
  uint32_t upt_sec;
  uint32_t idx;
  uint16_t total;
  uint16_t channels;
  uint8_t  paused;
  uint8_t  next;
  uint8_t  prev;
  uint8_t  playlist[MAX_TRACKS];

  // Переменные динамических каталогов
  uint16_t dir_current_idx;
  uint16_t dir_total_count;
  char     current_dir_path[64];
} Player_t;

static UI_t     ui;
static Player_t p;

// Безопасная побайтовая вставка двух цифр
static void put_num2(char* dst, uint32_t val) {
  dst[0] = '0' + (val / 10) % 10;
  dst[1] = '0' + (val % 10);
}

// Быстрый перевод числа в строку
static char* itoa_fast(uint32_t num) {
  static char buf[12];
  char*       s = buf + 11;
  *s            = '\0';
  if (num == 0) {
    *(--s) = '0';
    return s;
  }
  while (num > 0) {
    *(--s) = '0' + (num % 10);
    num /= 10;
  }
  return s;
}

// Вывод верхней информационной плашки (Прямоугольник + Трек: хх/хх + каталог)
static void draw_top_panel_ui(uint32_t current_idx, uint32_t total_count) {
  // Рисуем сплошной цветной прямоугольник для шапки (Высота 30 пикселей)
  lcd_fillrect(BAR_X, ROW_TOP_Y - 22, BAR_X + BAR_W, ROW_TOP_Y + 8, TOP_PANEL_BG);

  lcd_u8g2_set_font(u8g2_font_terminus_24b_cyr);

  // Сборка строки "Трек: X / Y  [Каталог]" в буфер ui.text
  memcpy(ui.text, "Трек: ", 10);

  char*    p1 = itoa_fast(current_idx);
  uint32_t l1 = strlen(p1);
  memcpy(ui.text + 10, p1, l1);

  memcpy(ui.text + 10 + l1, " / ", 3);

  char*    p2 = itoa_fast(total_count);
  uint32_t l2 = strlen(p2);
  memcpy(ui.text + 10 + l1 + 3, p2, l2);

  uint32_t offset = 10 + l1 + 3 + l2;
  memcpy(ui.text + offset, "  [", 3);
  offset += 3;

  uint32_t dir_len = strlen(p.current_dir_path);
  memcpy(ui.text + offset, p.current_dir_path, dir_len);
  offset += dir_len;

  ui.text[offset]     = ']';
  ui.text[offset + 1] = '\0';

  lcd_draw_u8g2_string(BAR_X + 6, ROW_TOP_Y, ui.text, TOP_PANEL_TXT, TOP_PANEL_BG);
}

// Вывод раздельного времени и статуса под прогресс-баром
static void draw_time_row(uint32_t cur, uint32_t tot, const char* status, uint16_t color) {
  lcd_u8g2_set_font(u8g2_font_terminus_24b_cyr);

  // 1. Слева: Прошедшее время трека (текущая позиция)
  ui.text[0] = '0';
  ui.text[1] = '0';
  ui.text[2] = ':';
  ui.text[3] = '0';
  ui.text[4] = '0';
  ui.text[5] = '\0';
  put_num2(&ui.text[0], cur / 60);
  put_num2(&ui.text[3], cur % 60);
  lcd_draw_u8g2_string(BAR_X, ROW_TIME_Y, ui.text, GRAY, BLACK);

  // 2. Справа: Оставшееся время трека (tot - cur)
  uint32_t rem = (tot >= cur) ? (tot - cur) : 0;
  ui.text[0]   = '0';
  ui.text[1]   = '0';
  ui.text[2]   = ':';
  ui.text[3]   = '0';
  ui.text[4]   = '0';
  ui.text[5]   = '\0';
  put_num2(&ui.text[0], rem / 60);
  put_num2(&ui.text[3], rem % 60);
  lcd_draw_u8g2_string(BAR_X + BAR_W - 60, ROW_TIME_Y, ui.text, GRAY, BLACK);

  // 3. Посередине: Статус ПАУЗА / ВОСПР
  if (status) {
    uint32_t center_x = BAR_X + (BAR_W / 2) - 50;
    // Прямоугольник закраски уменьшен по высоте, чтобы не затирать нижнюю строку
    lcd_fillrect(center_x, ROW_TIME_Y - 20, center_x + 100, ROW_TIME_Y + 2, BLACK);
    lcd_draw_u8g2_string(center_x + 15, ROW_TIME_Y, status, color, BLACK);
  }
}

// Вывод накопленного времени воспроизведения
static void draw_uptime_ui(void) {
  memcpy(ui.text, "Общее время: ", 23);
  ui.text[23] = '0';
  ui.text[24] = '0';
  ui.text[25] = ':';
  ui.text[26] = '0';
  ui.text[27] = '0';
  ui.text[28] = ':';
  ui.text[29] = '0';
  ui.text[30] = '0';
  ui.text[31] = '\0';

  put_num2(&ui.text[23], p.upt_sec / 3600);
  put_num2(&ui.text[26], (p.upt_sec % 3600) / 60);
  put_num2(&ui.text[29], p.upt_sec % 60);

  lcd_u8g2_set_font(u8g2_font_terminus_24b_cyr);
  lcd_draw_u8g2_string(BAR_X, ROW_UPTIME_Y, ui.text, CYAN, BLACK);
}

// Поиск трека по индексу
static uint8_t find_track(uint32_t target_idx) {
  uint32_t idx = 0;
  f_closedir(&p.dir);
  if (f_opendir(&p.dir, p.current_dir_path) != FR_OK) return 0;

  while (f_readdir(&p.dir, &p.fno) == FR_OK && p.fno.fname[0] != '\0') {
    if (!(p.fno.fattrib & AM_DIR)) {
      char* ext = strrchr(p.fno.fname, '.');
      if (ext && strcasecmp(ext, ".WAV") == 0) {
        if (idx++ == target_idx) return 1;
      }
    }
  }
  return 0;
}

// Перемешивание плейлиста
static void shuffle_playlist(void) {
  if (p.total == 0) return;
  for (uint16_t i = 0; i < p.total && i < MAX_TRACKS; i++) p.playlist[i] = i;
  for (int i = p.total - 1; i > 0; i--) {
    int     j     = get_random(i + 1);
    uint8_t temp  = p.playlist[i];
    p.playlist[i] = p.playlist[j];
    p.playlist[j] = temp;
  }
  p.idx = 0;
}

// Инициализация
void shuffle_player_init(void) {
  wav_play("");

  // 1. Сканируем накопитель на музыкальные папки
  p.dir_total_count = dir_manager_scan_total();
  p.dir_current_idx = 0;

  // 2. Извлекаем путь к первому каталогу
  if (p.dir_total_count == 0 || !dir_manager_get_path_by_index(p.dir_current_idx, p.current_dir_path, sizeof(p.current_dir_path))) {
    while (1) {
      blink_led(200);
    }  // Если WAV папок вообще нет
  }

  // 3. Считаем треки внутри этой конкретной папки
  if (f_opendir(&p.dir, p.current_dir_path) != FR_OK) {
    while (1) blink_led(200);
  }
  p.total = 0;
  while (f_readdir(&p.dir, &p.fno) == FR_OK && p.fno.fname[0] != '\0') {
    if (!(p.fno.fattrib & AM_DIR)) {
      char* ext = strrchr(p.fno.fname, '.');
      if (ext && strcasecmp(ext, ".WAV") == 0 && p.total < MAX_TRACKS) p.total++;
    }
  }

  random_seed(*(volatile uint32_t*)(0x1FFF7A10) ^ ram_noise_seed ^ p.total);
  shuffle_playlist();
}

// Управление Play/Pause
void shuffle_player_play_pause(void) {
  p.paused = !p.paused;
  if (p.paused) {
    wav_pause();
    draw_time_row(g_current_seconds, g_total_seconds, "ПАУЗА", YELLOW);
  } else {
    wav_resume();
    draw_time_row(g_current_seconds, g_total_seconds, "ВОСПР", GREEN);
  }
}

void shuffle_player_next(void) {
  wav_stop();
  p.paused = 0;
  p.next   = 1;
}
void shuffle_player_prev(void) {
  wav_stop();
  p.paused = 0;
  p.prev   = 1;
}

// Прыжок по найденным каталогам (вызывать на Кнопку 5 из button.c)
void shuffle_player_switch_directory(void) {
  if (p.dir_total_count <= 1) return;

  wav_stop();
  p.paused = 0;

  // Переходим к следующему каталогу по кольцу
  p.dir_current_idx++;
  if (p.dir_current_idx >= p.dir_total_count) {
    p.dir_current_idx = 0;
  }

  // Достаем новый путь
  dir_manager_get_path_by_index(p.dir_current_idx, p.current_dir_path, sizeof(p.current_dir_path));

  // Считаем WAV-файлы в новой папке
  p.total = 0;
  if (f_opendir(&p.dir, p.current_dir_path) == FR_OK) {
    while (f_readdir(&p.dir, &p.fno) == FR_OK && p.fno.fname[0] != '\0') {
      if (!(p.fno.fattrib & AM_DIR)) {
        char* ext = strrchr(p.fno.fname, '.');
        if (ext && strcasecmp(ext, ".WAV") == 0 && p.total < MAX_TRACKS) p.total++;
      }
    }
  }

  shuffle_playlist();
  p.next = 1;  // Форсируем автомат выдать первый трек из новой папки
}

// Главный процесс
void shuffle_player_process(void) {
  static uint8_t  play_next   = 1;
  static uint32_t last_fill_w = 0, last_sec = 0xFFFFFFFF, ui_refresh = 0;

  if (p.next || p.prev) {
    if (p.prev && p.idx > 0) p.idx = (p.idx >= 2) ? (p.idx - 2) : (p.total - 1);
    p.next = p.prev = 0;
    play_next       = 1;
    last_fill_w     = 0;
    last_sec        = 0xFFFFFFFF;
    lcd_fillrect(BAR_X + 1, BAR_Y + 1, BAR_X + BAR_W - 1, BAR_Y + BAR_H - 1, BLACK);
  }

  wav_process();
  if (!wav_is_playing() && !p.paused) play_next = 1;

  if (play_next && !p.paused) {
    play_next = last_fill_w = 0;
    last_sec                = 0xFFFFFFFF;

    // Черный прямоугольник очистки бьет строго от стыка верхней плашки до бара
    lcd_fillrect(BAR_X, ROW_TOP_Y + 9, BAR_X + BAR_W, BAR_Y, BLACK);
    lcd_fillrect(BAR_X, BAR_Y + BAR_H + 1, BAR_X + BAR_W, ROW_UPTIME_Y + 24, BLACK);

    while (1) {
      if (p.idx >= p.total) shuffle_playlist();
      if (!find_track(p.playlist[p.idx])) {
        p.idx++;
        continue;
      }

      uint32_t dir_len = strlen(p.current_dir_path);
      memcpy(ui.path, p.current_dir_path, dir_len);
      ui.path[dir_len] = '/';
      strcpy(ui.path + dir_len + 1, p.fno.fname);

      if (wav_play(ui.path) == 0) {
        wav_get_file_info(&p.pos, &p.size, &p.rate, &p.channels);
        g_total_seconds = (p.rate > 0 && p.channels > 0) ? (p.size / (p.rate * p.channels * 2)) : 0;
        strncpy(ui.name, p.fno.fname, 120);
        ui.name[120] = '\0';
        // Рисуем верхнюю цветную плашку
        draw_top_panel_ui(p.idx + 1, p.total);
        // Отрисовка имени файла в три строки желтым цветом
        lcd_draw_u8g2_string_wrapped(BAR_X, ROW_NAME_Y, ui.name, BAR_W, YELLOW, BLACK);
        // Рамка прогресс-бара
        POINT_COLOR = GRAY;
        lcd_drawrectangle(BAR_X, BAR_Y, BAR_X + BAR_W, BAR_Y + BAR_H);
        // Отрисовка раздельного времени и статуса
        draw_time_row(0, g_total_seconds, "ВОСПР", GREEN);
        draw_uptime_ui();
        p.idx++;
        break;
      }
      p.idx++;
    }
    p.paused = 0;
  }
  wav_process();
  if (wav_is_playing() && !p.paused && ++ui_refresh >= 15000) {
    ui_refresh        = 0;
    p.pos             = wav_get_position();
    g_current_seconds = (p.rate > 0 && p.channels > 0) ? (p.pos / (p.rate * p.channels * 2)) : 0;
    if (g_current_seconds != last_sec) {
      last_sec = g_current_seconds;
      p.upt_sec++;
      draw_time_row(g_current_seconds, g_total_seconds, NULL, 0);
      draw_uptime_ui();
    }
  }
  if (wav_is_playing() && !p.paused && p.size > 0) {
    uint32_t max_w      = BAR_W - 2;
    uint32_t cur_fill_w = (uint32_t)(((uint64_t)p.pos * max_w) / p.size);
    if (cur_fill_w > max_w) cur_fill_w = max_w;
    if (cur_fill_w > last_fill_w) {
      lcd_fillrect(BAR_X + 1 + last_fill_w, BAR_Y + 1, BAR_X + 1 + cur_fill_w, BAR_Y + BAR_H - 1, GREEN);
      last_fill_w = cur_fill_w;
    }
  }
}