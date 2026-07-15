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

extern const uint8_t u8g2_font_terminus_24b_cyr[];

#define WAV_DIR "0:/WAV3"
#define BAR_X   40
#define BAR_Y   180
#define BAR_W   400
#define BAR_H   16

FATFS   fs;
DIR     wav_dir;
FILINFO file_info;

__attribute__((aligned(4))) char time_line_str[32];
__attribute__((aligned(4))) char uptime_str[32];
__attribute__((aligned(4))) char full_path_buf[256];

__attribute__((aligned(4))) char tmp_buf_track[16];
__attribute__((aligned(4))) char tmp_buf_total[16];
__attribute__((aligned(4))) char tmp_buf_uptime[16];

int main(void) {
  hw_init();
  lcd_fillrect(0, 0, 480, 320, BLACK);

  if (f_mount(&fs, "0:", 1) != FR_OK) {
    while (1) {
      blink_led(100);
    }
  }

  if (f_opendir(&wav_dir, WAV_DIR) != FR_OK) {
    while (1) {
      blink_led(200);
    }
  }

  uint32_t target_track_index = 0;
  uint8_t  play_next          = 1;

  uint32_t current_pos     = 0;
  uint32_t total_size      = 0;
  uint32_t sample_rate     = 0;
  uint16_t channels        = 0;
  uint32_t total_seconds   = 0;
  uint32_t current_seconds = 0;

  uint32_t last_fill_w  = 0;
  uint32_t last_seconds = 0xFFFFFFFF;

  uint32_t ui_refresh_counter   = 0;
  uint32_t total_uptime_sec     = 0;
  uint32_t last_uptime_tick_sec = 0;

  while (1) {
    blink_led(500);

    wav_process();

    if (!wav_is_playing()) {
      play_next = 1;
    }

    if (play_next) {
      play_next = 0;

      last_fill_w  = 0;
      last_seconds = 0xFFFFFFFF;

      uint8_t track_started = 0;

      while (!track_started) {
        f_closedir(&wav_dir);
        if (f_opendir(&wav_dir, WAV_DIR) != FR_OK) {
          continue;
        }

        uint32_t current_file_index  = 0;
        uint8_t  wav_found_in_folder = 0;

        while (f_readdir(&wav_dir, &file_info) == FR_OK && file_info.fname[0] != 0) {
          if (!(file_info.fattrib & AM_DIR)) {
            char* ext = strrchr(file_info.fname, '.');
            if (ext && strcasecmp(ext, ".WAV") == 0) {
              if (current_file_index == target_track_index) {
                wav_found_in_folder = 1;
                break;
              }
              current_file_index++;
            }
          }
        }

        if (!wav_found_in_folder) {
          target_track_index = 0;
          continue;
        }

        strcpy(full_path_buf, WAV_DIR);
        strcat(full_path_buf, "/");
        strcat(full_path_buf, file_info.fname);

        if (wav_play(full_path_buf) == 0) {
          wav_get_file_info(&current_pos, &total_size, &sample_rate, &channels);

          if (sample_rate > 0 && channels > 0) {
            total_seconds = total_size / (sample_rate * channels * 2);
          } else {
            total_seconds = 0;
          }

          lcd_fillrect(BAR_X, BAR_Y - 60, BAR_X + BAR_W, BAR_Y - 5, BLACK);
          lcd_fillrect(BAR_X + 1, BAR_Y + 1, BAR_X + BAR_W - 1, BAR_Y + BAR_H - 1, BLACK);

          lcd_u8g2_set_font(u8g2_font_terminus_24b_cyr);
          lcd_draw_u8g2_string(BAR_X, BAR_Y - 50, "Воспроизведение:", WHITE, BLACK);
          lcd_draw_u8g2_string(BAR_X, BAR_Y - 20, file_info.fname, YELLOW, BLACK);

          POINT_COLOR = GRAY;
          lcd_drawrectangle(BAR_X, BAR_Y, BAR_X + BAR_W, BAR_Y + BAR_H);

          target_track_index++;
          track_started = 1;
          break;
        } else {
          target_track_index++;
        }
      }
    }

    wav_process();

    if (wav_is_playing()) {
      ui_refresh_counter++;
      if (ui_refresh_counter >= 15000) {
        ui_refresh_counter = 0;

        current_pos = wav_get_position();
        if (sample_rate > 0 && channels > 0) {
          current_seconds = current_pos / (sample_rate * channels * 2);
        } else {
          current_seconds = 0;
        }

        if (current_seconds != last_uptime_tick_sec) {
          last_uptime_tick_sec = current_seconds;
          total_uptime_sec++;
        }

        if (current_seconds != last_seconds) {
          last_seconds = current_seconds;

          format_time(current_seconds, tmp_buf_track);
          format_time(total_seconds, tmp_buf_total);

          strcpy(time_line_str, tmp_buf_track);
          strcat(time_line_str, " / ");
          strcat(time_line_str, tmp_buf_total);

          lcd_u8g2_set_font(u8g2_font_terminus_24b_cyr);
          uint8_t font_h = lcd_u8g2_get_font_height();

          lcd_draw_u8g2_string(BAR_X, BAR_Y + BAR_H + font_h + 4, time_line_str, GRAY, BLACK);

          format_time(total_uptime_sec, tmp_buf_uptime);
          strcpy(uptime_str, "Общее время: ");
          strcat(uptime_str, tmp_buf_uptime);

          lcd_draw_u8g2_string(BAR_X, BAR_Y + BAR_H + (font_h * 2) + 12, uptime_str, CYAN, BLACK);
        }
      }
    }

    if (wav_is_playing()) {
      if (total_size > 0) {
        uint32_t cur_fill_w = (uint32_t)(((uint64_t)current_pos * (BAR_W - 2)) / total_size);
        if (cur_fill_w > (BAR_W - 2)) cur_fill_w = BAR_W - 2;

        if (cur_fill_w > last_fill_w) {
          lcd_fillrect(BAR_X + 1 + last_fill_w, BAR_Y + 1, BAR_X + 1 + cur_fill_w, BAR_Y + BAR_H - 1, GREEN);
          last_fill_w = cur_fill_w;
        }
      }
    }
  }
}
