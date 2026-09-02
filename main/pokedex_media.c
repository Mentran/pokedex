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

#include <stdio.h>
#include <string.h>

static const char *TAG = "pokedex_media";

#define CRY_PACKED_MAX  24576
#define CRY_CHUNK       256
#define SPRITE_PACK1    "/dex/sprites1.bin"
#define SPRITE_PACK2    "/dex/sprites2.bin"
#define CRIES_PATH      "/dex/cries.bin"

static bool s_fs;
static TaskHandle_t s_task;
static volatile int s_play_id;
static volatile uint32_t s_gen;
static uint8_t s_packed[CRY_PACKED_MAX];

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
            .max_files = 4,
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
        xTaskCreate(pokedex_media_task, "dex_cry", 6144, NULL, 4, &s_task);
    }
    return s_fs;
}

void pokedex_media_deinit(void)
{
    pokedex_media_stop_cry();
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
    memset(dsc, 0, sizeof(*dsc));
    dsc->header.magic = LV_IMAGE_HEADER_MAGIC;
    dsc->header.cf = LV_COLOR_FORMAT_RGB565;
    dsc->header.w = POKEDEX_SPRITE_W;
    dsc->header.h = POKEDEX_SPRITE_H;
    dsc->header.stride = POKEDEX_SPRITE_W * 2;
    dsc->data_size = POKEDEX_SPRITE_BYTES;
    dsc->data = rgb565;
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
    bsp_audio_set_volume(80);
    int16_t pcm[CRY_CHUNK];
    while (gen == s_gen) {
        int n = pokedex_ima_next(&st, s_packed, (size_t)packed_len, pcm, CRY_CHUNK);
        if (n <= 0) {
            break;
        }
        bsp_audio_write(pcm, (size_t)n * sizeof(int16_t));
    }
}

static void pokedex_media_task(void *arg)
{
    (void)arg;
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        int id = s_play_id;
        uint32_t gen = s_gen;
        if (id > 0 && gen == s_gen) {
            play_one(id, gen);
        }
    }
}
