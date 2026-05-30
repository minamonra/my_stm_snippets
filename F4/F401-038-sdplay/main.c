#include "main.h"            // main.h теперь в одном каталоге, оставляем как есть


// Фиксированные безопасные размеры массивов в ОЗУ
static FATFS fs;
static DIR s_dir;
static FILINFO s_fno;
static char current_wav_file[64];
static char full_wav_path[128];
static char time_str[16];
static char rem_str[16];
static char out_line[128];
static char s_buf[16];

// Глобальные статические переменные состояний
static PlayerState_t state = STATE_INIT;
static uint16_t current_track_display_idx = 1;

__attribute__((section(".noinit"))) static uint32_t s_rand_seed;
static uint16_t total_wav_files_count = 0;
static uint32_t start_track_time = 0;

const char *WAV_Next_Track_Callback(void)
{
    if (total_wav_files_count == 0)
        return NULL;

    if (Find_Random_Wav_File(current_wav_file) == 0)
    {
        current_track_display_idx++;
        Draw_Static_UI(current_track_display_idx);
        Display_Process(0, current_wav_file, 1);

        strcpy(full_wav_path, AUDIO_DIR);
        strcat(full_wav_path, "/");
        strcat(full_wav_path, current_wav_file);

        start_track_time = ttms;
        return full_wav_path;
    }
    return NULL;
}

int main(void)
{
    uint8_t track_started_flag = 0;

    SystemClock_Config_84MHz();
    Periph_Init();
    delay_ms(100);

    ST7789_DMA_Init();
    ST7789_Init();
    ST7789_ClearScreen_DMA(ST7789_BLACK);

    if (s_rand_seed == 0 || s_rand_seed == 0xFFFFFFFF)
    {
        s_rand_seed = 123456789UL;
    }

    state = STATE_INIT;

    while (1)
    {
        WAV_Process_Buffer(); // Постоянно подпитываем DMA звуком

        static uint32_t last_led = 0;
        if (ttms - last_led >= 500)
        {
            last_led = ttms;
            LED2TOGGLE;
        }

        // Подфункция обработки кнопок
        Process_Button_Events();

        // Идеально читаемый конечный автомат
        switch (state)
        {
        case STATE_INIT:
            if (Playlist_Init() != 0)
            {
                ST7789_FillRectangle(10, 80, 300, 60, ST7789_RED);
                ST7789_DrawString_16x32(45, 95, "Error SD", ST7789_WHITE, ST7789_RED);
                state = STATE_ERROR;
            }
            else
            {
                state = STATE_PLAY_NEXT;
            }
            break;

        case STATE_PLAY_NEXT:
            if (Playlist_Load_Track(&start_track_time) != 0)
            {
                state = STATE_ERROR;
            }
            else
            {
                track_started_flag = 0;
                state = STATE_PLAYING;
            }
            break;

        case STATE_PLAYING:
            if (total_audio_bytes > 0 && wav_audio_bytes_left < total_audio_bytes)
            {
                track_started_flag = 1;
            }

            static uint32_t last_display = 0;
            if (ttms - last_display >= 200)
            {
                last_display = ttms;
                uint32_t seconds_ticks = (ttms - start_track_time) / 1000;
                Display_Process(seconds_ticks, current_wav_file, 0);
            }

            if (track_started_flag && wav_audio_bytes_left == 0)
            {
                current_track_display_idx++;
                end_of_file_reached = 0;
                state = STATE_PLAY_NEXT;
            }
            break;

        case STATE_ERROR:
            LED2TOGGLE;
            delay_ms(100);
            break;
        }
    }
}

// ============================================================================
// ВЫДЕЛЕННЫЕ ФУНКЦИИ ДЛЯ РАБОТЫ С ФАЙЛАМИ И ПЛЕЙЛИСТОМ
// ============================================================================

// 1. Инициализация SD карты, FatFS и подсчет треков в AU
uint8_t Playlist_Init(void)
{
    if (SD_Init() != 0 || f_mount(&fs, "", 1) != FR_OK)
    {
        return 1; // Ошибка железа или монтирования
    }

    s_rand_seed ^= ttms;
    total_wav_files_count = 0;

    if (f_opendir(&s_dir, AUDIO_DIR) == FR_OK)
    {
        while (f_readdir(&s_dir, &s_fno) == FR_OK && s_fno.fname[0] != '\0')
        {
            if (!(s_fno.fattrib & AM_DIR))
            {
                char *ext = strrchr(s_fno.fname, '.');
                if (ext && (ext[1] == 'W' || ext[1] == 'w') && (ext[2] == 'A' || ext[2] == 'a') && (ext[3] == 'V' || ext[3] == 'v'))
                {
                    total_wav_files_count++;
                }
            }
        }
        f_closedir(&s_dir);
    }

    current_track_display_idx = 1;
    return 0; // Успешно настроено
}

// 2. Поиск и мгновенный аппаратный запуск нового трека
uint8_t Playlist_Load_Track(uint32_t *track_start_time)
{
    if (Find_Random_Wav_File(current_wav_file) != 0)
    {
        ST7789_FillRectangle(0, 26, 320, 138, ST7789_BLACK);
        ST7789_DrawString_16x32(40, 70, "НЕТ ТРЕКОВ В AU", ST7789_CYAN, ST7789_BLACK);
        return 1; // Файлов нет
    }

    Draw_Static_UI(current_track_display_idx);
    Display_Process(0, current_wav_file, 1);

    strcpy(full_wav_path, AUDIO_DIR);
    strcat(full_wav_path, "/");
    strcat(full_wav_path, current_wav_file);

    DMA1->HIFCR |= 0x00000F3D; // Аппаратный сброс аудио DMA

    uint8_t play_status = WAV_Play(full_wav_path);
    if (play_status != 0)
    {
        ST7789_FillRectangle(10, 80, 300, 60, ST7789_RED);
        strcpy(out_line, "Error WAV: ");
        itoa32(play_status, s_buf);
        strcat(out_line, s_buf);
        ST7789_DrawString_12x24(20, 98, out_line, ST7789_WHITE, ST7789_RED);
        return 2; // Ошибка декодирования WAV
    }

    *track_start_time = ttms; // Фиксируем время старта через указатель
    return 0;                 // Трек успешно запущен
}

// ============================================================================
// ОСТАЛЬНЫЕ ОПТИМИЗИРОВАННЫЕ ПОДФУНКЦИИ (КНОПКИ, МАТЕМАТИКА, ГРАФИКА)
// ============================================================================

void Process_Button_Events(void)
{
    static uint8_t btn_state = 0;
    static uint32_t btn_timer = 0;
    uint8_t btn_click_triggered = 0;
    uint8_t btn_hold_triggered = 0;

    if (!(GPIOA->IDR & (1 << 0)))
    {
        if (btn_state == 0)
        {
            btn_state = 1;
            btn_timer = ttms;
        }
        else if (btn_state == 1)
        {
            if (ttms - btn_timer >= 1500)
            {
                btn_state = 2;
                btn_hold_triggered = 1;
            }
        }
        s_rand_seed ^= ttms;
    }
    else
    {
        if (btn_state == 1)
        {
            if (ttms - btn_timer > 50)
            {
                btn_click_triggered = 1;
            }
        }
        btn_state = 0;
    }

    if (btn_hold_triggered)
    {
        Audio_Stop();
        memset(audio_buff1, 0, (AUDIO_BUF_SIZE * 2) * sizeof(uint16_t));
        memset(audio_buff2, 0, (AUDIO_BUF_SIZE * 2) * sizeof(uint16_t));

        s_rand_seed ^= ttms;
        current_track_display_idx = 1;
        Draw_Static_UI(current_track_display_idx);
        Display_Process(0, "Плеер остановлен", 1);

        state = STATE_PLAY_NEXT;
    }

    if (btn_click_triggered && state == STATE_PLAYING)
    {
        s_rand_seed ^= (ttms + btn_timer);

        Audio_Stop();
        memset(audio_buff1, 0, (AUDIO_BUF_SIZE * 2) * sizeof(uint16_t));
        memset(audio_buff2, 0, (AUDIO_BUF_SIZE * 2) * sizeof(uint16_t));

        WAV_Process_Buffer();
        delay_ms(20);

        current_track_display_idx++;
        state = STATE_PLAY_NEXT;
    }
}

uint32_t Get_Random_Number(uint32_t max_val)
{
    if (max_val == 0)
        return 0;
    s_rand_seed = (1103515245UL * s_rand_seed + 12345UL);
    return (uint32_t)(s_rand_seed / 65536UL) % max_val;
}

uint8_t Find_Random_Wav_File(char *out_name)
{
    if (total_wav_files_count == 0)
        return 1;
    uint32_t target_rand_idx = Get_Random_Number(total_wav_files_count);
    uint32_t current_idx = 0;

    if (f_opendir(&s_dir, AUDIO_DIR) != FR_OK)
        return 1;

    while (f_readdir(&s_dir, &s_fno) == FR_OK && s_fno.fname[0] != '\0')
    {
        if (!(s_fno.fattrib & AM_DIR))
        {
            char *ext = strrchr(s_fno.fname, '.');
            if (ext && (ext[1] == 'W' || ext[1] == 'w') && (ext[2] == 'A' || ext[2] == 'a') && (ext[3] == 'V' || ext[3] == 'v'))
            {
                if (current_idx == target_rand_idx)
                {
                    strcpy(out_name, s_fno.fname);
                    f_closedir(&s_dir);
                    return 0;
                }
                current_idx++;
            }
        }
    }
    f_closedir(&s_dir);

    if (f_opendir(&s_dir, AUDIO_DIR) == FR_OK)
    {
        if (f_readdir(&s_dir, &s_fno) == FR_OK && s_fno.fname[0] != '\0')
        {
            strcpy(out_name, s_fno.fname);
            f_closedir(&s_dir);
            return 0;
        }
        f_closedir(&s_dir);
    }
    return 1;
}

void Draw_Static_UI(uint16_t track_num)
{
    ST7789_FillRectangle(0, 0, 320, 25, ST7789_DARKGRAY);
    ST7789_DrawString_12x24(10, 1, "СЛУЧАЙНЫЙ ВЫБОР", ST7789_CYAN, ST7789_DARKGRAY);

    char num_buf[8];
    num_buf[0] = '#';
    num_buf[1] = '0' + (track_num / 10);
    num_buf[2] = '0' + (track_num % 10);
    num_buf[3] = '\0';
    ST7789_DrawString_12x24(280, 1, num_buf, ST7789_WHITE, ST7789_DARKGRAY);

    ST7789_DrawHLine(0, 25, 320, ST7789_GRAY);
    ST7789_DrawRectangle(8, 165, 304, 34, ST7789_GRAY);
    ST7789_FillRectangle(12, 191, 296, 6, ST7789_DARKGRAY);

    ST7789_DrawHLine(0, 207, 320, ST7789_GRAY);
    ST7789_FillRectangle(0, 208, 320, 32, ST7789_DARKGRAY);
    ST7789_DrawString_12x24(70, 212, "РЭНДОМ ПЛЕЕР СМСИС", ST7789_YELLOW, ST7789_DARKGRAY);
}

void Display_Process(uint32_t sec_played, const char *file_name, uint8_t update_text)
{
    if (update_text)
    {
        ST7789_FillRectangle(0, 26, 320, 138, ST7789_BLACK);

        char tmp_chunk[64];
        strncpy(tmp_chunk, file_name, 18);
        tmp_chunk[18] = '\0';
        ST7789_DrawString_16x32(5, 45, tmp_chunk, ST7789_ORANGE, ST7789_BLACK);
        if (strlen(file_name) > 18)
        {
            strncpy(tmp_chunk, file_name + 18, 18);
            tmp_chunk[18] = '\0';
            ST7789_DrawString_16x32(5, 85, tmp_chunk, ST7789_ORANGE, ST7789_BLACK);
        }
    }
    uint32_t total_seconds = 0;
    if (total_audio_bytes > 0)
    {
        total_seconds = total_audio_bytes / 88200;
    }
    uint32_t rem_seconds = (total_seconds > sec_played) ? (total_seconds - sec_played) : 0;
    format_time(sec_played, time_str);
    ST7789_DrawString_12x24(12, 167, time_str, ST7789_GREEN, ST7789_BLACK);
    format_time(rem_seconds, rem_str);
    ST7789_DrawString_12x24(248, 167, rem_str, ST7789_YELLOW, ST7789_BLACK);
    if (total_seconds > 0)
    {
        uint32_t bar_width = (sec_played * 296) / total_seconds;
        if (bar_width > 296)
            bar_width = 296;
        if (bar_width > 0)
        {
            ST7789_FillRectangle(12, 191, bar_width, 6, ST7789_GREEN);
        }
    }
}

void __libc_init_array(void) {}

/*### Что ещё можно убрать из `main.c`, чтобы освободить его на 100%?
Если захотите пойти еще дальше и оставить в `main.c` только чистый бесконечный цикл `while(1)`, вы можете сделать следующее:
* **Создать `src/player_ui.c`**: и перенести туда функции отрисовки `Draw_Static_UI` и `Display_Process`.
* **Создать `src/playlist.c`**: и перенести туда математический генератор `Get_Random_Number` и тяжелую функцию сканирования FatFS папки `Find_Random_Wav_File`.

Хотите **вынести графику и поиск файлов в отдельные Си-файлы**, чтобы `main.c` стал размером всего в 50 строчек?*/