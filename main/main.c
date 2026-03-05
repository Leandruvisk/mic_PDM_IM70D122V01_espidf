#include <stdio.h>
#include "driver/i2s_pdm.h"
#include "esp_log.h"
#include "esp_log.h"
#include "driver/i2s_std.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define DATA_PIN 35
#define CLK_PIN  36
#define SAMPLE_SIZE (512)
#define SAMPLE_RATE 48000

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_NONE);

    i2s_chan_handle_t rx_handle = NULL;
    int16_t buf[SAMPLE_SIZE];
    size_t bytes_read;

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

    while (1)
    {
       if (i2s_channel_read(rx_handle, buf, sizeof(buf),
                         &bytes_read, portMAX_DELAY) == ESP_OK)
        {
            if (bytes_read > 0) {
                fwrite(buf, 1, bytes_read, stdout);
                fflush(stdout);
            }
        }
    }

}