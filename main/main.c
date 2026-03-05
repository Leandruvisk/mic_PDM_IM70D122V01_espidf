#include <stdio.h>
#include <math.h>
#include "driver/i2s_pdm.h"
#include "esp_log.h"
#include "driver/i2s_std.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define DATA_PIN 35
#define CLK_PIN  36

/********** PARAMETRIZÁVEIS **********/
#define SAMPLE_SIZE     2048       // tamanho do buffer
#define SAMPLE_RATE     48000      // Hz
#define FREQ_MIN        200        // Hz
#define FREQ_MAX        400        // Hz
#define DETECT_PERIOD_MS 200       // ms
#define DETECT_THRESHOLD 50        // % mínimo de frames ativos
/************************************/

static const char *TAG = "PDM_DETECT";

// Buffer global
static int16_t buf[SAMPLE_SIZE];

// Função de detecção de frequência (simplificada)
float detect_frequency(int16_t *buf, size_t len) {
    float max_amp = 0;
    int peak_bin = 0;
    for (int bin = 0; bin < len / 2; bin++) {
        float re = 0, im = 0;
        for (int n = 0; n < len; n++) {
            float angle = 2.0f * M_PI * bin * n / len;
            re += buf[n] * cosf(angle);
            im -= buf[n] * sinf(angle);
        }
        float mag = sqrtf(re*re + im*im);
        if (mag > max_amp) {
            max_amp = mag;
            peak_bin = bin;
        }
    }
    return (float)peak_bin * SAMPLE_RATE / len;
}

// Task de detecção
void freq_detect_task(void *arg) {
    i2s_chan_handle_t rx_handle = (i2s_chan_handle_t)arg;
    size_t bytes_read;
    int active_frames = 0;
    int total_frames = 0;

    int frames_needed = (SAMPLE_RATE * DETECT_PERIOD_MS / 1000) / SAMPLE_SIZE;

    while (1) {
        if (i2s_channel_read(rx_handle, buf, sizeof(buf), &bytes_read, portMAX_DELAY) == ESP_OK) {
            if (bytes_read > 0) {
                float freq = detect_frequency(buf, SAMPLE_SIZE);

                if (freq >= FREQ_MIN && freq <= FREQ_MAX) {
                    active_frames++;
                }
                total_frames++;

                if (total_frames >= frames_needed) {
                    int percent = (active_frames * 100) / total_frames;
                    if (percent >= DETECT_THRESHOLD) {
                        ESP_LOGI(TAG, "Freq detectada: %.1f Hz, %d%% ativo", freq, percent);
                    }
                    active_frames = 0;
                    total_frames = 0;
                }
            }
        }
    }
}

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);

    i2s_chan_handle_t rx_handle = NULL;
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_handle));

    i2s_pdm_rx_config_t pdm_rx_cfg = {
        .clk_cfg = {
            .sample_rate_hz = SAMPLE_RATE,
            .clk_src = I2S_CLK_SRC_PLL_160M,
            .dn_sample_mode = I2S_PDM_DSR_8S,
            .mclk_multiple = I2S_MCLK_MULTIPLE_128,
            .bclk_div = 2,
        },
        .slot_cfg = I2S_PDM_RX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .clk = CLK_PIN,
            .din = DATA_PIN,
            .invert_flags = {
                .clk_inv = false,
            },
        },
    };

    ESP_ERROR_CHECK(i2s_channel_init_pdm_rx_mode(rx_handle, &pdm_rx_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));

    // Cria task de detecção
    xTaskCreate(freq_detect_task, "freq_detect", 4096, (void*)rx_handle, 5, NULL);
}