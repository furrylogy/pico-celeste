#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/i2c_master.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"

#include "esp_lcd_touch_gt911.h"
#include "esp_lcd_st6201.h"

#include "pico8sim.h"

extern const unsigned char bg_celeste_pixel_data[];

/* LCD size */
#define LCD_H_RES   (480)
#define LCD_V_RES   (272)

/* LCD settings */
#define LCD_SPI_NUM         (SPI2_HOST)
#define LCD_PIXEL_CLK_HZ    (80 * 1000 * 1000)
#define LCD_CMD_BITS        (8)
#define LCD_PARAM_BITS      (8)
#define LCD_BITS_PER_PIXEL  (16)
#define LCD_BL_ON_LEVEL     (1)

/* LCD pins */
#define LCD_GPIO_SCLK       (GPIO_NUM_23)
#define LCD_GPIO_MOSI       (GPIO_NUM_19)
#define LCD_GPIO_RST        (GPIO_NUM_12)
#define LCD_GPIO_DC         (GPIO_NUM_14)
#define LCD_GPIO_CS         (GPIO_NUM_22)
#define LCD_GPIO_BL         (GPIO_NUM_2)

/* Touch settings */
#define TOUCH_I2C_NUM       (0)
#define TOUCH_I2C_CLK_HZ    (400000)
/* LCD touch pins */
#define TOUCH_I2C_SCL       (GPIO_NUM_16)
#define TOUCH_I2C_SDA       (GPIO_NUM_18)
#define TOUCH_GPIO_INT      (GPIO_NUM_17)
#define TOUCH_GPIO_RST      (GPIO_NUM_4)

static const char *TAG = "MAIN";

static esp_lcd_panel_io_handle_t lcd_io = NULL;
static esp_lcd_panel_handle_t lcd_panel = NULL;
static esp_lcd_touch_handle_t touch_handle = NULL;

static esp_err_t app_lcd_init(void)
{
    esp_err_t ret = ESP_OK;

    /* LCD backlight */
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << LCD_GPIO_BL
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

    /* LCD initialization */
    ESP_LOGD(TAG, "Initialize SPI bus");
    const spi_bus_config_t buscfg = {
        .sclk_io_num = LCD_GPIO_SCLK,
        .mosi_io_num = LCD_GPIO_MOSI,
        .miso_io_num = GPIO_NUM_NC,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = 128 * 128 * sizeof(uint16_t),
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(LCD_SPI_NUM, &buscfg, SPI_DMA_CH_AUTO), TAG, "SPI init failed");

    ESP_LOGD(TAG, "Install panel IO");
    const esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = LCD_GPIO_DC,
        .cs_gpio_num = LCD_GPIO_CS,
        .pclk_hz = LCD_PIXEL_CLK_HZ,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_SPI_NUM, &io_config, &lcd_io), err, TAG, "New panel IO failed");

    ESP_LOGD(TAG, "Install LCD driver");
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_GPIO_RST,
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(6, 0, 0)
        .rgb_endian = LCD_RGB_ENDIAN_BGR,
#else
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
#endif
        .bits_per_pixel = LCD_BITS_PER_PIXEL,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_st6201(lcd_io, &panel_config, &lcd_panel), err, TAG, "New panel failed");

    esp_lcd_panel_reset(lcd_panel);
    esp_lcd_panel_init(lcd_panel);
    esp_lcd_panel_mirror(lcd_panel, true, true);
    esp_lcd_panel_disp_on_off(lcd_panel, true);

    /* LCD backlight on */
    ESP_ERROR_CHECK(gpio_set_level(LCD_GPIO_BL, LCD_BL_ON_LEVEL));

    return ret;

err:
    if (lcd_panel) {
        esp_lcd_panel_del(lcd_panel);
    }
    if (lcd_io) {
        esp_lcd_panel_io_del(lcd_io);
    }
    spi_bus_free(LCD_SPI_NUM);
    return ret;
}
static esp_err_t app_touch_init(void)
{
    /* Initilize I2C */
    esp_log_level_set("*", ESP_LOG_DEBUG);
    i2c_master_bus_handle_t i2c_handle = NULL;
    const i2c_master_bus_config_t i2c_config = {
        .i2c_port = TOUCH_I2C_NUM,
        .sda_io_num = TOUCH_I2C_SDA,
        .scl_io_num = TOUCH_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags = {
            .enable_internal_pullup = true,
        },
    };
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&i2c_config, &i2c_handle), TAG, "New TouchPad IO failed");
    
    /* Initialize touch HW */
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();

    esp_lcd_touch_io_gt911_config_t tp_gt911_config = {
        .dev_addr = tp_io_config.dev_addr,
    };

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = LCD_H_RES,
        .y_max = LCD_V_RES,
        .rst_gpio_num = TOUCH_GPIO_RST,
        .int_gpio_num = TOUCH_GPIO_INT,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
        .driver_data = &tp_gt911_config,
    };
    
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    tp_io_config.scl_speed_hz = TOUCH_I2C_CLK_HZ;
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c(i2c_handle, &tp_io_config, &tp_io_handle), TAG, "New TouchPad failed");
    return esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &touch_handle);
}

static uint8_t btn_state[6] = {0};
static inline uint8_t check_touch_in_area(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,uint16_t x, uint16_t y)
{
    return x1 <= x && x <= x2 && y1 <= y && y <= y2;
}
void update_btn_state(esp_lcd_touch_point_data_t* touch_points, uint8_t touch_cnt)
{
    memset(btn_state,0,6);
    for(uint8_t i=0;i<touch_cnt;i++)
    {
        uint16_t x = touch_points[i].x;
        uint16_t y = touch_points[i].y;
        btn_state[0] |= check_touch_in_area(350,180,389,219,x,y)||check_touch_in_area(360,150,389,249,x,y);
        btn_state[1] |= check_touch_in_area(430,180,469,219,x,y)||check_touch_in_area(430,150,459,249,x,y);
        btn_state[2] |= check_touch_in_area(390,140,429,179,x,y)||check_touch_in_area(360,150,459,179,x,y);
        btn_state[3] |= check_touch_in_area(390,220,429,259,x,y)||check_touch_in_area(360,220,459,249,x,y);
        btn_state[4] |= check_touch_in_area(70,210,109,249,x,y);
        btn_state[5] |= check_touch_in_area(40,160,79,199,x,y);
    }
}
//获取按钮 B 状态
//B: 0..5：左 右 上 下 按钮 O 按钮 X
uint8_t btn(uint8_t b)
{
    if(unlikely(b>5)) return 0;
    return btn_state[b];
}

__weak void music(int16_t n, int16_t fade_len, uint8_t channel_mask)
{
    (void)n;
    (void)fade_len;
    (void)channel_mask;
}

__weak void sfx(uint8_t n)
{
    (void)n;
}

fix16_t rnd(fix16_t n)
{
    if (n <= 0)
        return 0;

    return (fix16_t)((uint32_t)rand() % (uint32_t)n);
}

void read_button_state()
{
    esp_lcd_touch_point_data_t touch_points[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
    uint8_t touch_cnt = 0;

    esp_lcd_touch_read_data(touch_handle);
    esp_lcd_touch_get_data(touch_handle, touch_points, &touch_cnt, CONFIG_ESP_LCD_TOUCH_MAX_POINTS);
    update_btn_state(touch_points, touch_cnt);
}

static void push_bg(uint16_t* buf)
{
    size_t row_size = LCD_H_RES * 2;
    size_t rows_per_chunk = (128 * 128 * 2) / row_size;

    for (int y = 0; y < LCD_V_RES; y += rows_per_chunk) {
        int rows = (y + rows_per_chunk <= LCD_V_RES) ? (int)rows_per_chunk : (LCD_V_RES - y);
        memcpy(buf, &bg_celeste_pixel_data[y * row_size], rows * row_size);
        esp_lcd_panel_draw_bitmap(lcd_panel, 0, y, LCD_H_RES, y + rows, buf);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void p8simulator_task(void *pvParameters)
{
    uint16_t* screenbuf=NULL;
    const TickType_t frame_ticks = pdMS_TO_TICKS(33);

    screenbuf = heap_caps_aligned_alloc(
        16, 
        128*128*2,
        MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL
    );
    push_bg(screenbuf);

    _init();

    TickType_t last_wake_time = xTaskGetTickCount();
    while (1) {
        read_button_state();
        _update();
        _draw();

        p8blit_to_rgb565(screenbuf);

        esp_lcd_panel_draw_bitmap(lcd_panel, 176,73,176+128,73+128, screenbuf);
        vTaskDelayUntil(&last_wake_time, frame_ticks);
    }
}

void app_main(void)
{
    /* LCD HW initialization */
    ESP_ERROR_CHECK(app_lcd_init());
    /* Touch initialization */
    ESP_ERROR_CHECK(app_touch_init());
    
    xTaskCreate(p8simulator_task, "pico-8 sim", 8192, NULL, 5, NULL);
}
