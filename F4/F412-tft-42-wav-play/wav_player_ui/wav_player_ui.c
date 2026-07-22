#include "wav_player_ui.h"
#include "dir_manager.h"
#include "stm32f4xx.h"
#include "common.h"
#include "audio_i2s.h"
#include "wav_player.h"
#include "sdcard.h"
#include "diskio.h"
#include "ff.h"
#include "sdcard_dma.h"
#include "lcd_io.h"
#include "lcd_draw_u8g2.h"
#include <string.h>

extern const uint8_t     u8g2_font_terminus_24b_cyr[];
extern volatile uint32_t ttms;
volatile uint32_t        last_fill_w = 0;

// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ
uint32_t          g_current_seconds = 0;
uint32_t          g_total_seconds   = 0;
volatile uint32_t ram_noise_seed;

// СТРУКТУРА ДАННЫХ ИНТЕРФЕЙСА
typedef struct {
  __attribute__((aligned(4))) char text[64];
  __attribute__((aligned(4))) char path[256];
  __attribute__((aligned(4))) char name[256];
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
  char     file_list[MAX_TRACKS][256];  // Список файлов текущей папки
  uint16_t playlist[MAX_TRACKS];        // Порядок воспроизведения
  uint16_t dir_current_idx;             // Каталоги
  uint16_t dir_total_count;
  char     current_dir_path[256];
  uint8_t  seeking_forward;  // Перемотка
  uint8_t  seeking_backward;
} Player_t;

static UI_t     ui;
static Player_t p;

// Геометрия интерфейса
#define UI_LEFT  0
#define UI_RIGHT 479
#define UI_WIDTH (UI_RIGHT - UI_LEFT)
// Верхняя панель
#define TOP_PANEL_Y 8
#define TOP_PANEL_H 30
// Разделитель
#define SEP_Y (TOP_PANEL_Y + TOP_PANEL_H)
#define SEP_H 1
// Панель каталога
#define DIR_PANEL_Y (SEP_Y + SEP_H)
#define DIR_PANEL_H 28
// Имя файла
#define NAME_PANEL_Y (DIR_PANEL_Y + DIR_PANEL_H + 26)
#define NAME_PANEL_H 72  // три строки

// Прогресс
#define BAR_X      UI_LEFT
#define BAR_Y      165
#define BAR_W      UI_WIDTH
#define BAR_H      16
#define BAR_BOTTOM (BAR_Y + BAR_H)

// Строка времени
#define TIME_ROW_Y (BAR_BOTTOM + 28)

// Общее время
#define UPTIME_ROW_Y (TIME_ROW_Y + 30)

#define NAVY 0x000F

#define TOP_PANEL_BG  GRAY
#define TOP_PANEL_TXT BLACK

#define PROGRESS_BAR_COLOR GRAY

#define PLAYCLR WHITE
#define PAUSCLR YELLOW

// Вспомогательные функции
// Безопасная вставка двух цифр
static void put_num2(char* dst, uint32_t val) {
  dst[0] = '0' + (val / 10) % 10;
  dst[1] = '0' + (val % 10);
}
// Быстрый перевод uint32 в строку
static void itoa_fast(uint32_t num, char* buf) {
  char* s = buf + 11;
  *s      = 0;
  do {
    *(--s) = '0' + (num % 10);
    num /= 10;
  } while (num);
  memmove(buf, s, (buf + 12) - s);
}

// Перемешивание списка файлов
static void shuffle_playlist(void) {
  if (p.total == 0) return;
  for (uint16_t i = 0; i < p.total; i++)
    p.playlist[i] = i;
  for (int i = p.total - 1; i > 0; i--) {
    uint16_t j    = get_random(i + 1);
    uint16_t t    = p.playlist[i];
    p.playlist[i] = p.playlist[j];
    p.playlist[j] = t;
  }
  p.idx = 0;
}

// Очистка областей

static void clear_filename_area(void) {
  lcd_fillrect(UI_LEFT, NAME_PANEL_Y, UI_RIGHT, NAME_PANEL_Y + NAME_PANEL_H, BLACK);
}

static void clear_progress_area(void) {
  lcd_fillrect(BAR_X + 1, BAR_Y + 1, BAR_X + BAR_W - 1, BAR_BOTTOM - 1, BLACK);
}

static void clear_bottom_area(void) {
  lcd_fillrect(UI_LEFT, TIME_ROW_Y - 22, UI_RIGHT, UPTIME_ROW_Y + 22, BLACK);
}

static void clear_track_screen(void) {
  clear_filename_area();
  clear_progress_area();
  clear_bottom_area();
  last_fill_w = 0;
}

static void draw_top_panel_ui(uint32_t current_idx, uint32_t total_count) {
  lcd_fillrect(UI_LEFT, TOP_PANEL_Y, UI_RIGHT, TOP_PANEL_Y + TOP_PANEL_H, TOP_PANEL_BG);
  lcd_u8g2_set_font(u8g2_font_terminus_24b_cyr);
  char n1[12];
  char n2[12];
  itoa_fast(current_idx, n1);
  itoa_fast(total_count, n2);
  uint32_t    pos = 0;
  const char* s   = "Трек: ";
  while (*s) ui.text[pos++] = *s++;
  strcpy(&ui.text[pos], n1);
  pos += strlen(n1);
  ui.text[pos++] = ' ';
  ui.text[pos++] = '/';
  ui.text[pos++] = ' ';
  strcpy(&ui.text[pos], n2);
  pos += strlen(n2);
  ui.text[pos] = 0;
  lcd_draw_u8g2_string(UI_LEFT + 6, TOP_PANEL_Y + 22, ui.text, TOP_PANEL_TXT, TOP_PANEL_BG);
  // черная полоска между панелями
  lcd_fillrect(UI_LEFT, SEP_Y, UI_RIGHT, SEP_Y + SEP_H, BLACK);
}

static void draw_dir_panel_ui(void) {
  char        dir_text[256];
  const char* dir = p.current_dir_path;
  if (strncmp(dir, "0:/", 3) == 0) dir += 3;
  uint32_t pos    = 0;
  dir_text[pos++] = '[';
  while (*dir && pos < sizeof(dir_text) - 2) dir_text[pos++] = *dir++;
  dir_text[pos++] = ']';
  dir_text[pos]   = 0;
  lcd_fillrect(UI_LEFT, DIR_PANEL_Y, UI_RIGHT, DIR_PANEL_Y + DIR_PANEL_H, TOP_PANEL_BG);
  lcd_u8g2_set_font(u8g2_font_terminus_24b_cyr);
  lcd_draw_u8g2_string(UI_LEFT + 6, DIR_PANEL_Y + 21, dir_text, TOP_PANEL_TXT, TOP_PANEL_BG);
}

static void draw_time_row(uint32_t cur, uint32_t tot, const char* status, uint16_t color) {
  lcd_u8g2_set_font(u8g2_font_terminus_24b_cyr);
  lcd_fillrect(UI_LEFT, TIME_ROW_Y - 22, UI_RIGHT, TIME_ROW_Y + 2, BLACK);
  // текущее время
  put_num2(&ui.text[0], cur / 60);
  ui.text[2] = ':';
  put_num2(&ui.text[3], cur % 60);
  ui.text[5] = 0;
  lcd_draw_u8g2_string(UI_LEFT, TIME_ROW_Y, ui.text, GRAY, BLACK);
  // оставшееся
  uint32_t rem = (tot > cur) ? (tot - cur) : 0;
  put_num2(&ui.text[0], rem / 60);
  ui.text[2] = ':';
  put_num2(&ui.text[3], rem % 60);
  ui.text[5] = 0;
  lcd_draw_u8g2_string(UI_RIGHT - 60, TIME_ROW_Y, ui.text, GRAY, BLACK);
  if (status) {
    uint16_t cx = (UI_LEFT + UI_RIGHT) / 2 - 45;
    lcd_draw_u8g2_string(cx, TIME_ROW_Y, status, color, BLACK);
  }
}

static void draw_uptime_ui(void) {
  lcd_fillrect(UI_LEFT, UPTIME_ROW_Y - 22, UI_RIGHT, UPTIME_ROW_Y + 2, BLACK);
  const char* title = "Общее время: ";
  uint8_t     pos   = 0;
  while (*title) ui.text[pos++] = *title++;
  put_num2(&ui.text[pos], p.upt_sec / 3600);
  pos += 2;
  ui.text[pos++] = ':';
  put_num2(&ui.text[pos], (p.upt_sec % 3600) / 60);
  pos += 2;
  ui.text[pos++] = ':';
  put_num2(&ui.text[pos], p.upt_sec % 60);
  pos += 2;
  ui.text[pos] = 0;
  lcd_u8g2_set_font(u8g2_font_terminus_24b_cyr);
  lcd_draw_u8g2_string(UI_LEFT, UPTIME_ROW_Y, ui.text, CYAN, BLACK);
}

static uint8_t start_next_track(uint32_t* last_uptime_ms) {
  clear_track_screen();
  for (uint16_t tries = 0; tries < p.total; tries++) {
    if (p.idx >= p.total)
      shuffle_playlist();
    uint16_t file_idx = p.playlist[p.idx];
    uint32_t len      = strlen(p.current_dir_path);
    memcpy(ui.path, p.current_dir_path, len);
    ui.path[len] = '/';
    strcpy(ui.path + len + 1, p.file_list[file_idx]);
    if (wav_play(ui.path) == 0) {
      wav_get_file_info(&p.pos, &p.size, &p.rate, &p.channels);
      p.pos           = wav_get_position();
      g_total_seconds = (p.rate && p.channels) ? p.size / (p.rate * p.channels * 2) : 0;
      strncpy(ui.name, p.file_list[file_idx], sizeof(ui.name) - 1);
      ui.name[sizeof(ui.name) - 1] = 0;
      draw_top_panel_ui(p.idx + 1, p.total);
      draw_dir_panel_ui();
      lcd_draw_u8g2_string_wrapped(UI_LEFT, NAME_PANEL_Y, ui.name, UI_WIDTH, YELLOW, BLACK);
      POINT_COLOR = GRAY;
      lcd_drawrectangle(BAR_X, BAR_Y, BAR_X + BAR_W, BAR_Y + BAR_H);
      draw_time_row(0, g_total_seconds, "ВОСПР", PLAYCLR);
      draw_uptime_ui();
      p.idx++;
      *last_uptime_ms = ttms;
      return 1;
    }
    p.idx++;
  }

  return 0;
}

static void update_playback(uint32_t* last_sec, uint32_t* last_uptime_ms) {
  while ((ttms - *last_uptime_ms) >= 1000) {
    *last_uptime_ms += 1000;
    p.upt_sec++;
  }
  p.pos = wav_get_position();
  if (p.rate && p.channels) {
    g_current_seconds = p.pos / (p.rate * p.channels * 2);
    if (g_current_seconds != *last_sec) {
      *last_sec = g_current_seconds;
      draw_time_row(g_current_seconds, g_total_seconds, NULL, 0);
      draw_uptime_ui();
    }
  }
  if (p.size) {
    uint32_t max_w = BAR_W - 2;
    uint32_t fill  = (uint32_t)(((uint64_t)p.pos * max_w) / p.size);
    if (fill > max_w) fill = max_w;
    if (fill > last_fill_w) {
      lcd_fillrect(BAR_X + 1 + last_fill_w, BAR_Y + 1, BAR_X + 1 + fill, BAR_BOTTOM - 1, PROGRESS_BAR_COLOR);
      last_fill_w = fill;
    }
  }
}

void wav_playerui_process(void) {
  static uint8_t  play_next      = 1;
  static uint32_t last_sec       = 0xFFFFFFFF;
  static uint32_t last_uptime_ms = 0;
  if (p.next || p.prev) {
    if (p.prev && p.idx > 0) p.idx = (p.idx >= 2) ? p.idx - 2 : p.total - 1;
    p.next      = 0;
    p.prev      = 0;
    play_next   = 1;
    last_fill_w = 0;
    last_sec    = 0xFFFFFFFF;
  }
  wav_process();
  uint8_t playing = wav_is_playing();
  if (!playing && !p.paused) play_next = 1;
  if (play_next && !p.paused) {
    play_next   = 0;
    last_fill_w = 0;
    last_sec    = 0xFFFFFFFF;
    if (start_next_track(&last_uptime_ms)) playing = 1;
  }
  if (playing && !p.paused) {
    update_playback(&last_sec, &last_uptime_ms);
  } else {
    last_uptime_ms = ttms;
  }
}

// Сканирование WAV файлов текущего каталога
static void scan_current_directory(void) {
  p.total = 0;
  if (f_opendir(&p.dir, p.current_dir_path) != FR_OK) return;
  while (f_readdir(&p.dir, &p.fno) == FR_OK) {
    if (p.fno.fname[0] == '\0') break;
    if (p.fno.fattrib & AM_DIR) continue;
    char* ext = strrchr(p.fno.fname, '.');
    if (ext && strcasecmp(ext, ".WAV") == 0) {
      if (p.total < MAX_TRACKS) {
        strncpy(p.file_list[p.total], p.fno.fname, sizeof(p.file_list[0]) - 1);
        p.file_list[p.total][sizeof(p.file_list[0]) - 1] = 0;
        p.total++;
      }
    }
  }
  f_closedir(&p.dir);
}

// Инициализация
void wav_playerui_init(void) {
  wav_play("");
  p.dir_total_count = dir_manager_scan_total();  // Сканируем доступные каталоги
  p.dir_current_idx = 0;
  if (p.dir_total_count == 0 || !dir_manager_get_path_by_index(p.dir_current_idx, p.current_dir_path, sizeof(p.current_dir_path))) {
    while (1)
      blink_led(200);
  }
  scan_current_directory();  // Один раз читаем содержимое папки
  if (p.total == 0) {
    while (1) blink_led(200);
  }
  random_seed(*(volatile uint32_t*)(0x1FFF7A10) ^ ram_noise_seed ^ p.total);
  shuffle_playlist();
  p.paused           = 0;
  p.next             = 0;
  p.prev             = 0;
  p.seeking_forward  = 0;
  p.seeking_backward = 0;
}

// Управление Play/Pause
void wav_playerui_play_pause(void) {
  p.paused = !p.paused;
  if (p.paused) {
    wav_pause();
    draw_time_row(g_current_seconds, g_total_seconds, "ПАУЗА", PAUSCLR);
  } else {
    wav_resume();
    draw_time_row(g_current_seconds, g_total_seconds, "ВОСПР", PLAYCLR);
  }
}

void wav_playerui_next(void) {
  wav_stop();
  p.paused = 0;
  p.next   = 1;
}

void wav_playerui_prev(void) {
  wav_stop();
  p.paused = 0;
  p.prev   = 1;
}

// Прыжок по найденным каталогам (вызывать на Кнопку 5 из button.c)
void wav_playerui_switch_directory(void) {
  if (p.dir_total_count <= 1) return;
  wav_stop();
  p.paused = 0;
  p.dir_current_idx++;  // следующий каталог
  if (p.dir_current_idx >= p.dir_total_count) p.dir_current_idx = 0;
  // получить путь
  if (!dir_manager_get_path_by_index(p.dir_current_idx, p.current_dir_path, sizeof(p.current_dir_path))) {
    return;
  }
  scan_current_directory();  // заново заполнить список файлов
  if (p.total == 0) return;
  shuffle_playlist();
  p.next = 1;  // принудительно первый трек
}