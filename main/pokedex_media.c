// pokedex_media.c —— 挂载 pokedexfs,读 80x80 精灵图,在工作任务里播 IMA 叫声。
// 按键回调只投递播放请求,不得在 ISR/LVGL 锁里阻塞读完整叫声。
#include "pokedex_media.h"

#include "pokedex.h"
#include "pokedex_ima.h"
#include "bsp_audio.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static const char *TAG = "pokedex_media";

#define CRY_PACKED_MAX  24576
#define CRY_CHUNK       256
#define SPRITE_PACK1    "/dex/sprites1.bin"
#define SPRITE_PACK2    "/dex/sprites2.bin"
#define TRAINERS_PATH   "/dex/trainers.bin"
#define CRIES_PATH      "/dex/cries.bin"

static bool s_fs;
static TaskHandle_t s_task;
static volatile int s_play_id;
static volatile uint32_t s_gen;
static volatile int s_bgm_on;
static uint32_t s_bgm_t;
static uint8_t s_packed[CRY_PACKED_MAX];

#define BGM_RATE          8000
#define BGM_TEMPO         81
#define BGM_LOOP_BEATS    8
#define BGM_VOL           40
#define CRY_VOL           60
#define BGM_LEAD_AMP      1800
#define BGM_BASS_AMP      900

typedef struct {
    float beat;
    float duration;
    float hz;
    float gain;
} bgm_note_t;

/* 我是谁 default track「宝可梦中心」，square lead + triangle bass. */
static const bgm_note_t s_bgm_lead[] = {
    { 0.0f, 0.45f, 523.25f, 1.0f },
    { 0.5f, 0.45f, 659.25f, 1.0f },
    { 1.0f, 0.45f, 783.99f, 1.0f },
    { 1.5f, 0.45f, 659.25f, 1.0f },
    { 2.0f, 0.45f, 587.33f, 1.0f },
    { 2.5f, 0.45f, 698.46f, 1.0f },
    { 3.0f, 0.70f, 880.00f, 1.0f },
    { 4.0f, 0.45f, 783.99f, 1.0f },
    { 4.5f, 0.45f, 698.46f, 1.0f },
    { 5.0f, 0.45f, 659.25f, 1.0f },
    { 5.5f, 0.45f, 587.33f, 1.0f },
    { 6.0f, 0.90f, 523.25f, 1.0f },
};

static const bgm_note_t s_bgm_bass[] = {
    { 0.0f, 1.8f, 130.81f, 0.6f },
    { 2.0f, 1.8f, 146.83f, 0.6f },
    { 4.0f, 1.8f, 164.81f, 0.6f },
    { 6.0f, 1.8f, 196.00f, 0.6f },
};

static const uint32_t s_bgm_spb = (BGM_RATE * 60u) / BGM_TEMPO;
static const uint32_t s_bgm_loop = ((BGM_RATE * 60u) / BGM_TEMPO) * BGM_LOOP_BEATS;

static void pokedex_media_task(void *arg);

static bool read_at(const char *path, uint32_t offset, void *dst, size_t len)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        return false;
    }
    bool ok = (fseek(fp, (long)offset, SEEK_SET) == 0) &&
              (fread(dst, 1, len, fp) == len);
    fclose(fp);
    return ok;
}

bool pokedex_media_init(void)
{
    if (!s_fs) {
        esp_vfs_spiffs_conf_t conf = {
            .base_path = "/dex",
            .partition_label = "pokedexfs",
            .max_files = 6,
            .format_if_mount_failed = false,
        };
        esp_err_t err = esp_vfs_spiffs_register(&conf);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "SPIFFS mount failed: %s", esp_err_to_name(err));
        } else {
            s_fs = true;
            size_t total = 0, used = 0;
            if (esp_spiffs_info("pokedexfs", &total, &used) == ESP_OK) {
                ESP_LOGI(TAG, "pokedexfs %u / %u", (unsigned)used, (unsigned)total);
            }
        }
    }
    if (!s_task) {
        xTaskCreate(pokedex_media_task, "dex_cry", 8192, NULL, 4, &s_task);
    }
    return s_fs;
}

void pokedex_media_deinit(void)
{
    pokedex_media_stop_bgm();
    pokedex_media_stop_cry();
}

static void fill_rgb565_dsc(uint8_t *rgb565, lv_image_dsc_t *dsc)
{
    memset(dsc, 0, sizeof(*dsc));
    dsc->header.magic = LV_IMAGE_HEADER_MAGIC;
    dsc->header.cf = LV_COLOR_FORMAT_RGB565;
    dsc->header.w = POKEDEX_SPRITE_W;
    dsc->header.h = POKEDEX_SPRITE_H;
    dsc->header.stride = POKEDEX_SPRITE_W * 2;
    dsc->data_size = POKEDEX_SPRITE_BYTES;
    dsc->data = rgb565;
}

bool pokedex_media_load_sprite(int id, uint8_t *rgb565, lv_image_dsc_t *dsc)
{
    if (!s_fs || !rgb565 || !dsc || id < 1 || id > POKEDEX_COUNT) {
        return false;
    }
    const char *path = (id <= POKEDEX_SPRITE_PACK1) ? SPRITE_PACK1 : SPRITE_PACK2;
    int index = (id <= POKEDEX_SPRITE_PACK1) ? (id - 1) : (id - POKEDEX_SPRITE_PACK1 - 1);
    uint32_t offset = (uint32_t)index * POKEDEX_SPRITE_BYTES;
    if (!read_at(path, offset, rgb565, POKEDEX_SPRITE_BYTES)) {
        ESP_LOGW(TAG, "sprite #%d read failed", id);
        return false;
    }
    fill_rgb565_dsc(rgb565, dsc);
    return true;
}

bool pokedex_media_load_trainer(int speaker, uint8_t *rgb565, lv_image_dsc_t *dsc)
{
    if (!s_fs || !rgb565 || !dsc || speaker < 0 || speaker >= POKEDEX_SPEAKER_COUNT) {
        return false;
    }
    uint32_t offset = (uint32_t)speaker * POKEDEX_SPRITE_BYTES;
    if (!read_at(TRAINERS_PATH, offset, rgb565, POKEDEX_SPRITE_BYTES)) {
        ESP_LOGW(TAG, "trainer #%d read failed", speaker);
        return false;
    }
    fill_rgb565_dsc(rgb565, dsc);
    return true;
}

void pokedex_media_play_cry(int id)
{
    if (id < 1 || id > POKEDEX_COUNT) {
        return;
    }
    s_play_id = id;
    s_gen++;
    if (s_task) {
        xTaskNotifyGive(s_task);
    }
}

void pokedex_media_stop_cry(void)
{
    s_play_id = 0;
    s_gen++;
}

void pokedex_media_start_bgm(void)
{
    s_bgm_on = 1;
    s_bgm_t = 0;
    if (s_task) {
        xTaskNotifyGive(s_task);
    }
}

void pokedex_media_stop_bgm(void)
{
    s_bgm_on = 0;
    if (s_task) {
        xTaskNotifyGive(s_task);
    }
}

static int load_cry(int id, uint8_t *dst, size_t dst_len)
{
    uint8_t head[8];
    if (!read_at(CRIES_PATH, 0, head, sizeof(head)) || memcmp(head, "CRY1", 4) != 0) {
        return -1;
    }
    uint16_t count = (uint16_t)(head[4] | (head[5] << 8));
    if (id < 1 || id > count) {
        return -1;
    }
    uint8_t rec[8];
    uint32_t rec_off = 8 + (uint32_t)(id - 1) * 8;
    if (!read_at(CRIES_PATH, rec_off, rec, sizeof(rec))) {
        return -1;
    }
    uint32_t offset = rec[0] | ((uint32_t)rec[1] << 8) |
                      ((uint32_t)rec[2] << 16) | ((uint32_t)rec[3] << 24);
    uint32_t len = rec[4] | ((uint32_t)rec[5] << 8) |
                   ((uint32_t)rec[6] << 16) | ((uint32_t)rec[7] << 24);
    if (len == 0 || len > dst_len) {
        return -1;
    }
    if (!read_at(CRIES_PATH, offset, dst, len)) {
        return -1;
    }
    return (int)len;
}

static void play_one(int id, uint32_t gen)
{
    int packed_len = load_cry(id, s_packed, sizeof(s_packed));
    if (packed_len < 16 || gen != s_gen) {
        return;
    }
    pokedex_ima_st_t st;
    if (pokedex_ima_begin(&st, s_packed, (size_t)packed_len) < 0) {
        return;
    }
    if (bsp_audio_set_format(POKEDEX_CRY_RATE, 16, 1) != ESP_OK) {
        ESP_LOGW(TAG, "audio format failed");
        return;
    }
    bsp_audio_set_volume(CRY_VOL);
    int16_t pcm[CRY_CHUNK];
    while (gen == s_gen) {
        int n = pokedex_ima_next(&st, s_packed, (size_t)packed_len, pcm, CRY_CHUNK);
        if (n <= 0) {
            break;
        }
        bsp_audio_write(pcm, (size_t)n * sizeof(int16_t));
    }
}

static int note_on(const bgm_note_t *n, float beat)
{
    return beat >= n->beat && beat < (n->beat + n->duration);
}

static int16_t bgm_sample(uint32_t t)
{
    float beat = (float)(t % s_bgm_loop) / (float)s_bgm_spb;
    float sec = (float)t / (float)BGM_RATE;
    float mix = 0.0f;
    for (size_t i = 0; i < sizeof(s_bgm_lead) / sizeof(s_bgm_lead[0]); i++) {
        const bgm_note_t *n = &s_bgm_lead[i];
        if (!note_on(n, beat)) {
            continue;
        }
        float phase = fmodf(n->hz * sec, 1.0f);
        mix += (phase < 0.5f ? 1.0f : -1.0f) * n->gain * (float)BGM_LEAD_AMP;
    }
    for (size_t i = 0; i < sizeof(s_bgm_bass) / sizeof(s_bgm_bass[0]); i++) {
        const bgm_note_t *n = &s_bgm_bass[i];
        if (!note_on(n, beat)) {
            continue;
        }
        float phase = fmodf(n->hz * sec, 1.0f);
        float tri = (phase < 0.5f) ? (4.0f * phase - 1.0f) : (3.0f - 4.0f * phase);
        mix += tri * n->gain * (float)BGM_BASS_AMP;
    }
    if (mix > 32767.0f) {
        mix = 32767.0f;
    } else if (mix < -32768.0f) {
        mix = -32768.0f;
    }
    return (int16_t)mix;
}

static void play_bgm_chunk(void)
{
    int16_t pcm[CRY_CHUNK];
    for (int i = 0; i < CRY_CHUNK; i++) {
        pcm[i] = bgm_sample(s_bgm_t++);
    }
    bsp_audio_write(pcm, sizeof(pcm));
}

static void pokedex_media_task(void *arg)
{
    (void)arg;
    for (;;) {
        if (s_play_id > 0) {
            int id = s_play_id;
            uint32_t gen = s_gen;
            s_play_id = 0;
            play_one(id, gen);
            if (s_bgm_on) {
                bsp_audio_set_volume(BGM_VOL);
            }
            continue;
        }
        if (s_bgm_on) {
            if (bsp_audio_set_format(BGM_RATE, 16, 1) != ESP_OK) {
                vTaskDelay(pdMS_TO_TICKS(50));
                continue;
            }
            bsp_audio_set_volume(BGM_VOL);
            play_bgm_chunk();
            continue;
        }
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
}
