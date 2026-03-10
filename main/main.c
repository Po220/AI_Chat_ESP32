/* LCD GUI  example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include "esp_ping.h"
#include "lwip/ip_addr.h"
#include "main.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"
#include "driver/i2s_std.h"
#include "esp_lvgl_port.h"
#include "esp_spiffs.h"
#include "board.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "audio_idf_version.h"
#include "esp_wifi.h"
#include "esp_http_client.h"
#include "baidu_access_token.h"
#include "esp_peripherals.h"
#include "periph_button.h"
#include "periph_wifi.h"
#include "nvs_flash.h"
#include "baidu_tts.h"
#include "baidu_vtt.h"
#include "minimax_chat.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "lvgl.h"
#include "driver/ledc.h"
#include "lv_gui.h"
#include "esp_lcd_touch_ft5x06.h"
#include "freertos/event_groups.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "audio_idf_version.h"
#include "esp_wifi.h"
#include "esp_http_client.h"
#include "baidu_access_token.h"
#include "esp_peripherals.h"
#include "periph_button.h"
#include "periph_wifi.h"
#include "board.h"
#include "driver/i2s_std.h"
#include "esp_lvgl_port.h"
#include "esp_spiffs.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"
#include "secrets.h"
// 定义事件组和事件标志
static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT1

static const char *TAG = "AI_CHAT_EXAMPLE";

char *baidu_access_token = "25.e64aa12a1deb11cad01b2c1b9433d1f6.315360000.2059728961.282335-118460502";


#define CONFIG_BAIDU_ACCESS_KEY     "gw3T9zXYIcA7X2V50JsW5Uh1"            // 这里修改成自己的百度API Key
#define CONFIG_BAIDU_SECRET_KEY     "nSXbCjAULsJXCBa4hjMr3UyeOeQeBtjS"            // 这里修改成自己的百度Secret Key
// 把下面的xxx替换成自己的token_key
const char * minimax_key = "Bearer sk-api-rujsuM_eARVy-vVJqzDUh61Oztr4H5RTy57oM707wB9KGopz6DHVNnhPO9DXp1qWlwTs31tunmE9LNjz5AhlZ4AcdUePYwnMY4ikts9iIuNE9l8V6Dj1ptY";
 
#define EXAMPLE_LVGL_TICK_PERIOD_MS    2

int ask_flag = 0;
int answer_flag = 0;
int lcd_clear_flag = 0;

static EventGroupHandle_t my_event_group;
#define ALL_REDAY   BIT0

/**************************** LVGL相关处理函数 **********************************/
static bool example_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_disp_drv_t *disp_driver = (lv_disp_drv_t *)user_ctx;
    lv_disp_flush_ready(disp_driver);
    return false;
}

static void example_lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) drv->user_data;
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    // copy a buffer's content to a specific area of the display
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
}

static void example_increase_lvgl_tick(void *arg)
{
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

static void example_lvgl_touch_cb(lv_indev_drv_t * drv, lv_indev_data_t * data)
{
    uint16_t touchpad_x[1] = {0};
    uint16_t touchpad_y[1] = {0};
    uint8_t touchpad_cnt = 0;

    /* Read touch controller data */
    esp_lcd_touch_read_data(drv->user_data);

    /* Get coordinates */
    bool touchpad_pressed = esp_lcd_touch_get_coordinates(drv->user_data, touchpad_x, touchpad_y, NULL, &touchpad_cnt, 1);

    if (touchpad_pressed && touchpad_cnt > 0) {
        data->point.x = touchpad_x[0];
        data->point.y = touchpad_y[0];
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}


/**************************** LVGL主界面更新函数 **********************************/
extern lv_obj_t *label1;
extern lv_obj_t *label2;
extern char ask_text[256];
extern char minimax_content[2048];

void value_update_cb(lv_timer_t * timer)
{
    if (ask_flag == 1)
    {
        lv_label_set_text_fmt(label1, "我：%s", ask_text);
        ask_flag = 0;
    }
    if (answer_flag == 1)
    {
        lv_label_set_text_fmt(label2, "AI：%s", minimax_content);
        answer_flag = 0;
    }
    if(lcd_clear_flag == 1)
    {
        lcd_clear_flag = 0;
        lv_label_set_text(label1, "我：");
        lv_label_set_text(label2, "AI：");
    }
}

/**************************** 主界面任务函数 **********************************/
static void main_page_task(void *pvParameters)
{
    xEventGroupWaitBits(my_event_group, ALL_REDAY, pdFALSE, pdFALSE, portMAX_DELAY);
    lv_obj_clean(lv_scr_act());
    lv_main_page();
    lv_timer_create(value_update_cb, 100, NULL);  // 创建一个lv_timer

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));    
    }

    vTaskDelete(NULL);
}

void WIFI_init(void)
{
      // 初始化 TCP/IP 协议栈
      ESP_ERROR_CHECK(esp_netif_init());

      // 创建默认事件循环
      ESP_ERROR_CHECK(esp_event_loop_create_default());
  
      // 创建 Wi-Fi STA 接口
      esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
      assert(sta_netif);
  
      // 初始化 Wi-Fi 配置
      wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
      ESP_ERROR_CHECK(esp_wifi_init(&cfg));

          // 配置 Wi-Fi STA 模式
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    while(1)
    {
        esp_wifi_connect(); // 启动 Wi-Fi 连接
        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            ESP_LOGI(TAG, "Wi-Fi connected to SSID: %s", ap_info.ssid);
            ESP_LOGI(TAG, "Signal strength: %d dBm", ap_info.rssi);
            break;
        } else {
            ESP_LOGE(TAG, "Wi-Fi not connected");
            vTaskDelay(100);
        }
    }
}


void set_custom_dns() {
    esp_netif_dns_info_t dns_info;
    dns_info.ip.type = IPADDR_TYPE_V4;
    dns_info.ip.u_addr.ip4.addr = ipaddr_addr("8.8.8.8"); // Google 公共 DNS

    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif) {
        esp_netif_set_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns_info);
        ESP_LOGI(TAG, "Custom DNS set to 8.8.8.8");
    } else {
        ESP_LOGE(TAG, "Failed to get network interface");
    }
}

/**************************** 按键任务函数 **********************************/



/**************************** AI对话任务函数 **********************************/

void ai_chat_task(void *pv)
{
    /******************* 1. 外设初始化 *******************/
    // 初始化外设管理配置（使用默认配置）
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    // 可以在此调整任务堆栈大小
    //periph_cfg.task_stack = 8*1024; 
    
    // 初始化外设集合
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);
    
    // 配置按钮外设IO8
    periph_button_cfg_t btn_cfg = {
        .gpio_mask = (1ULL << 8),
    };
    esp_periph_handle_t button_handle = periph_button_init(&btn_cfg);

    // 启动按钮外设
    esp_periph_start(set, button_handle);

    /******************* 2. 百度语音服务初始化 *******************/
    // 获取百度语音服务的访问令牌（如果尚未获取）
    if (baidu_access_token == NULL) {
        // 注意：使用后需要释放 baidu_access_token
        baidu_access_token = baidu_get_access_token(CONFIG_BAIDU_ACCESS_KEY, CONFIG_BAIDU_SECRET_KEY);
    }

    // 初始化百度语音转文字（VTT）服务
    baidu_vtt_config_t vtt_config = {
        .record_sample_rates = 16000,  // 采样率16kHz
        .encoding = ENCODING_LINEAR16, // 音频编码格式
    };
    baidu_vtt_handle_t vtt = baidu_vtt_init(&vtt_config);

    // 初始化百度文字转语音（TTS）服务
    baidu_tts_config_t tts_config = {
        .playback_sample_rate = 16000, // 播放采样率16kHz
    };
    baidu_tts_handle_t tts = baidu_tts_init(&tts_config);

    /******************* 3. 事件监听设置 *******************/
    ESP_LOGI(TAG, "[ 5 ] Set up event listener");
    
    // 初始化音频事件接口（默认配置）
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

    // 设置语音服务的监听器
    ESP_LOGI(TAG, "[5.1] Listening event from the pipeline");
    baidu_vtt_set_listener(vtt, evt);
    baidu_tts_set_listener(tts, evt);

    // 设置外设事件的监听器
    ESP_LOGI(TAG, "[5.2] Listening event from peripherals");
    audio_event_iface_set_listener(esp_periph_set_get_event_iface(set), evt);

    ESP_LOGI(TAG, "[ 6 ] Listen for all pipeline events");

    // 发送系统准备就绪信号（事件标志组）
    xEventGroupSetBits(my_event_group, ALL_REDAY);

    /******************* 4. 主事件循环 *******************/
    while (1) {
        // 等待事件到来（无限等待）
        audio_event_iface_msg_t msg;
        if (audio_event_iface_listen(evt, &msg, portMAX_DELAY) != ESP_OK) {
            ESP_LOGW(TAG, "[ * ] Event process failed: src_type:%d, source:%p cmd:%d, data:%p, data_len:%d",
                     msg.source_type, msg.source, msg.cmd, msg.data, msg.data_len);
            continue;
        }

        ESP_LOGI(TAG, "[ * ] Event received: src_type:%d, source:%p cmd:%d, data:%p, data_len:%d",
                 msg.source_type, msg.source, msg.cmd, msg.data, msg.data_len);

        // 检查TTS是否完成播放
        if (baidu_tts_check_event_finish(tts, &msg)) {
            ESP_LOGI(TAG, "[ * ] TTS Finish");
            continue;
        }

        /******************* 按钮事件处理 *******************/
        // 按钮按下事件处理
        if (msg.cmd == PERIPH_BUTTON_PRESSED) {
            // 停止当前可能正在播放的TTS
            baidu_tts_stop(tts);
            ESP_LOGI(TAG, "[ * ] Resuming pipeline");
            
            // 清除LCD显示标志
            lcd_clear_flag = 1;
            
            // 开始语音识别（录音+转文字）
            baidu_vtt_start(vtt);
        } 
        // 按钮释放事件处理（包括短按释放和长按释放）
        else if (msg.cmd == PERIPH_BUTTON_RELEASE || msg.cmd == PERIPH_BUTTON_LONG_RELEASE) {
            ESP_LOGI(TAG, "[ * ] Stop pipeline");

            // 停止语音识别并获取识别结果
            char *original_text = baidu_vtt_stop(vtt);
            if (original_text == NULL) {
                minimax_content[0] = 0; // 清空对话内容
                continue;
            }
            
            ESP_LOGI(TAG, "Original text = %s", original_text);
            ask_flag = 1;  // 设置提问标志

            // 调用MiniMax聊天API获取回答
            char *answer = minimax_chat(original_text);
            if (answer == NULL) {
                continue;
            }
            
            ESP_LOGI(TAG, "minimax answer = %s", answer);
            answer_flag = 1;  // 设置回答标志
            // 使用TTS播放回答
            baidu_tts_start(tts, answer);
        }
    }

    /******************* 5. 资源清理 *******************/
    ESP_LOGI(TAG, "[ 7 ] Stop audio_pipeline");
    
    // 销毁语音服务实例
    baidu_vtt_destroy(vtt);
    baidu_tts_destroy(tts);
    
    /* 在移除监听器前停止所有外设 */
    esp_periph_set_stop_all(set);
    audio_event_iface_remove_listener(esp_periph_set_get_event_iface(set), evt);

    /* 确保在销毁event_iface前移除所有监听器 */
    audio_event_iface_destroy(evt);
    esp_periph_set_destroy(set);
    
    // 删除当前任务
    vTaskDelete(NULL);
}

/******************************************************************************/
/***************************  I2C ↓ *******************************************/
esp_err_t bsp_i2c_init(void)
{
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = BSP_I2C_SDA,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = BSP_I2C_SCL,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = BSP_I2C_FREQ_HZ
    };
    i2c_param_config(BSP_I2C_NUM, &i2c_conf);

    return i2c_driver_install(BSP_I2C_NUM, i2c_conf.mode, 0, 0, 0);
}
/***************************  I2C ↑  *******************************************/
/*******************************************************************************/

/***********************************************************/
/*********************    音频 ↓   *************************/
static esp_codec_dev_handle_t play_dev_handle;    // speaker句柄
static esp_codec_dev_handle_t record_dev_handle;  // microphone句柄

static i2s_chan_handle_t i2s_tx_chan = NULL; // 发送通道
static i2s_chan_handle_t i2s_rx_chan = NULL; // 接收通道
static const audio_codec_data_if_t *i2s_data_if = NULL;  /* Codec data interface */


// I2S总线初始化
esp_err_t bsp_audio_init(void)
{
    esp_err_t ret = ESP_FAIL;
    if (i2s_tx_chan && i2s_rx_chan) {
        /* Audio was initialized before */
        return ESP_OK;
    }

    /* Setup I2S peripheral */
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(BSP_I2S_NUM, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true; // Auto clear the legacy data in the DMA buffer
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &i2s_tx_chan, &i2s_rx_chan));

    /* Setup I2S channels */
    const i2s_std_config_t std_cfg_default = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(16000),   // 采样率16000
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(32, I2S_SLOT_MODE_STEREO),  // 32位 2通道
        .gpio_cfg = { 
            .mclk = GPIO_I2S_MCLK, 
            .bclk = GPIO_I2S_SCLK, 
            .ws   = GPIO_I2S_LRCK, 
            .dout = GPIO_I2S_DOUT, 
            .din  = GPIO_I2S_SDIN,
        },
    };

    if (i2s_tx_chan != NULL) {
        ESP_GOTO_ON_ERROR(i2s_channel_init_std_mode(i2s_tx_chan, &std_cfg_default), err, TAG, "I2S channel initialization failed");
        ESP_GOTO_ON_ERROR(i2s_channel_enable(i2s_tx_chan), err, TAG, "I2S enabling failed");
    }
    if (i2s_rx_chan != NULL) {
        ESP_GOTO_ON_ERROR(i2s_channel_init_std_mode(i2s_rx_chan, &std_cfg_default), err, TAG, "I2S channel initialization failed");
        ESP_GOTO_ON_ERROR(i2s_channel_enable(i2s_rx_chan), err, TAG, "I2S enabling failed");
    }

    audio_codec_i2s_cfg_t i2s_cfg = {
        .port = BSP_I2S_NUM,
        .rx_handle = i2s_rx_chan,
        .tx_handle = i2s_tx_chan,
    };
    i2s_data_if = audio_codec_new_i2s_data(&i2s_cfg);
    if (i2s_data_if == NULL) {   
        goto err;
    }   

    return ESP_OK;

err:
    if (i2s_tx_chan) {
        i2s_del_channel(i2s_tx_chan);
    }
    if (i2s_rx_chan) {
        i2s_del_channel(i2s_rx_chan);
    }

    return ret;
}

// 初始化音频输出芯片
esp_codec_dev_handle_t bsp_audio_codec_speaker_init(void)
{
    if (i2s_data_if == NULL) {
        /* Configure I2S peripheral and Power Amplifier */
        ESP_ERROR_CHECK(bsp_audio_init());
    }
    assert(i2s_data_if);

    const audio_codec_gpio_if_t *gpio_if = audio_codec_new_gpio();

    audio_codec_i2c_cfg_t i2c_cfg = {
        .port = BSP_I2C_NUM,
        .addr = ES8311_CODEC_DEFAULT_ADDR,
    };
    const audio_codec_ctrl_if_t *i2c_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
    assert(i2c_ctrl_if);

    esp_codec_dev_hw_gain_t gain = {
        .pa_voltage = 5.0,
        .codec_dac_voltage = 3.3,
    };

    es8311_codec_cfg_t es8311_cfg = {
        .ctrl_if = i2c_ctrl_if,
        .gpio_if = gpio_if,
        .codec_mode = 1<<1,
        .pa_pin = GPIO_PWR_CTRL,
        .pa_reverted = false,
        .master_mode = false,
        .use_mclk = true,
        .digital_mic = false,
        .invert_mclk = false,
        .invert_sclk = false,
        .hw_gain = gain,
    };
    const audio_codec_if_t *es8311_dev = es8311_codec_new(&es8311_cfg);
    assert(es8311_dev);

    esp_codec_dev_cfg_t codec_dev_cfg = {
        .dev_type = 1<<1,
        .codec_if = es8311_dev,
        .data_if = i2s_data_if,
    };
    return esp_codec_dev_new(&codec_dev_cfg);
}

// 初始化音频输入芯片
esp_codec_dev_handle_t bsp_audio_codec_microphone_init(void)
{
    if (i2s_data_if == NULL) {
        /* Configure I2S peripheral and Power Amplifier */
        ESP_ERROR_CHECK(bsp_audio_init());
    }
    assert(i2s_data_if);

    audio_codec_i2c_cfg_t i2c_cfg = {
        .port = BSP_I2C_NUM,
        .addr = 0x82,
    };
    const audio_codec_ctrl_if_t *i2c_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
    assert(i2c_ctrl_if);

    es7210_codec_cfg_t es7210_cfg = {
        .ctrl_if = i2c_ctrl_if,
        .mic_selected = ES7210_SEL_MIC1 | ES7210_SEL_MIC2 | ES7210_SEL_MIC3 | ES7210_SEL_MIC4,
    };
    const audio_codec_if_t *es7210_dev = es7210_codec_new(&es7210_cfg);
    assert(es7210_dev);

    esp_codec_dev_cfg_t codec_es7210_dev_cfg = {
        .dev_type = 1<<0,
        .codec_if = es7210_dev,
        .data_if = i2s_data_if,
    };
    return esp_codec_dev_new(&codec_es7210_dev_cfg);
}

// 设置采样率
esp_err_t bsp_codec_set_fs(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch)
{
    esp_err_t ret = ESP_OK;

    esp_codec_dev_sample_info_t fs = {
        .sample_rate = rate,
        .channel = ch,
        .bits_per_sample = bits_cfg,
    };
    
    if (play_dev_handle) {
        ret = esp_codec_dev_close(play_dev_handle);
    }
    if (record_dev_handle) {
        ret |= esp_codec_dev_close(record_dev_handle);
        ret |= esp_codec_dev_set_in_gain(record_dev_handle, CODEC_DEFAULT_ADC_VOLUME);
    }

    if (play_dev_handle) {
        ret |= esp_codec_dev_open(play_dev_handle, &fs);
    }
    if (record_dev_handle) {
        ret |= esp_codec_dev_open(record_dev_handle, &fs);
    }
    return ret;
}

// 音频芯片初始化
esp_err_t bsp_codec_init(void)
{
    //play_dev_handle = bsp_audio_codec_speaker_init();
    //assert((play_dev_handle) && "play_dev_handle not initialized");

    record_dev_handle = bsp_audio_codec_microphone_init();
    assert((record_dev_handle) && "record_dev_handle not initialized");

    bsp_codec_set_fs(CODEC_DEFAULT_SAMPLE_RATE, CODEC_DEFAULT_BIT_WIDTH, CODEC_DEFAULT_CHANNEL);
    //esp_codec_dev_set_out_vol(play_dev_handle, VOLUME_DEFAULT);

    return ESP_OK;
}

// 播放音乐
esp_err_t bsp_i2s_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms)
{
    esp_err_t ret = ESP_OK;
    ret = esp_codec_dev_write(play_dev_handle, audio_buffer, len);
    *bytes_written = len;
    return ret;
}

// 设置静音与否
esp_err_t bsp_codec_mute_set(bool enable)
{
    esp_err_t ret = ESP_OK;
    ret = esp_codec_dev_set_out_mute(play_dev_handle, enable);
    return ret;
}

// 设置喇叭音量
esp_err_t bsp_codec_volume_set(int volume, int *volume_set)
{
    esp_err_t ret = ESP_OK;
    float v = volume;
    ret = esp_codec_dev_set_out_vol(play_dev_handle, (int)v);
    return ret;
}

// 获取MIC通道数
int bsp_get_feed_channel(void)
{
    return ADC_I2S_CHANNEL;
}

// 把I2S数据重新排布成模型需要的顺序
esp_err_t bsp_get_feed_data(bool is_get_raw_channel, int16_t *buffer, int buffer_len)
{
    esp_err_t ret = ESP_OK;
    
    int audio_chunksize = buffer_len / (sizeof(int16_t) * ADC_I2S_CHANNEL);

    ret = esp_codec_dev_read(record_dev_handle, (void *)buffer, buffer_len);
    
    if (!is_get_raw_channel) {
        for (int i = 0; i < audio_chunksize; i++) {
            int16_t ref = buffer[4 * i + 0];
            buffer[3 * i + 0] = buffer[4 * i + 1];
            buffer[3 * i + 1] = buffer[4 * i + 3];
            buffer[3 * i + 2] = ref;
        }
    }
    return ret;
}

// 开机音乐 任务函数
extern EventGroupHandle_t my_event_group;

extern const uint8_t music_pcm_start[] asm("_binary_sword_pcm_start");
extern const uint8_t music_pcm_end[]   asm("_binary_sword_pcm_end");

static const char err_reason[][30] = {"input param is invalid",
                                      "operation timeout"
                                     };

void power_music_task(void *pvParameters)
{
    esp_err_t ret = ESP_OK;
    size_t bytes_write = 0;
    uint8_t *data_ptr = (uint8_t *)music_pcm_start;
    
    /* (Optional) Disable TX channel and preload the data before enabling the TX channel,
     * so that the valid data can be transmitted immediately */
    ESP_ERROR_CHECK(i2s_channel_disable(i2s_tx_chan));
    ESP_ERROR_CHECK(i2s_channel_preload_data(i2s_tx_chan, data_ptr, music_pcm_end - data_ptr, &bytes_write));
    data_ptr += bytes_write;  // Move forward the data pointer

    /* Enable the TX channel */
    ESP_ERROR_CHECK(i2s_channel_enable(i2s_tx_chan));
    
    /* Write music to earphone */
    ret = i2s_channel_write(i2s_tx_chan, data_ptr, music_pcm_end - data_ptr, &bytes_write, portMAX_DELAY);
    if (ret != ESP_OK) {
        /* Since we set timeout to 'portMAX_DELAY' in 'i2s_channel_write'
            so you won't reach here unless you set other timeout value,
            if timeout detected, it means write operation failed. */
        ESP_LOGE(TAG, "[music] i2s write failed, %s", err_reason[ret == ESP_ERR_TIMEOUT]);
        abort();
    }
    if (bytes_write > 0) {
        ESP_LOGI(TAG, "[music] i2s music played, %d bytes are written.", bytes_write);
    } else {
        ESP_LOGE(TAG, "[music] i2s music play failed.");
        abort();
    }

    xEventGroupSetBits(my_event_group, START_MUSIC_COMPLETED);

    vTaskDelete(NULL);
}

/*********************    音频 ↑   *************************/
/***********************************************************/

/***********************************************************/
/****************    LCD显示屏 ↓   *************************/

// 定义液晶屏句柄
static esp_lcd_panel_handle_t panel_handle = NULL;
esp_lcd_panel_io_handle_t io_handle = NULL; 
static esp_lcd_touch_handle_t tp;   // 触摸屏句柄
static lv_disp_t *disp;      // 指向液晶屏
static lv_indev_t *disp_indev = NULL; // 指向触摸屏

// 液晶屏初始化
esp_err_t bsp_display_new(void)
{
    esp_err_t ret = ESP_OK;
    static lv_disp_drv_t disp_drv;      

    // 初始化SPI总线
    ESP_LOGD(TAG, "Initialize SPI bus");
    const spi_bus_config_t buscfg = {
        .sclk_io_num = BSP_LCD_SPI_CLK,
        .mosi_io_num = BSP_LCD_SPI_MOSI,
        .miso_io_num = GPIO_NUM_NC,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = BSP_LCD_H_RES * BSP_LCD_V_RES * sizeof(uint16_t),
        //.max_transfer_sz = BSP_LCD_H_RES * 40 * sizeof(uint16_t),
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(BSP_LCD_SPI_NUM, &buscfg, SPI_DMA_CH_AUTO), TAG, "SPI init failed");
    // 液晶屏控制IO初始化
    ESP_LOGD(TAG, "Install panel IO");
    const esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = BSP_LCD_DC,
        .cs_gpio_num = BSP_LCD_SPI_CS,
        .pclk_hz = BSP_LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        //.on_color_trans_done = example_notify_lvgl_flush_ready,
        .user_ctx = &disp_drv,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)BSP_LCD_SPI_NUM, &io_config, &io_handle), err, TAG, "New panel IO failed");
    // 初始化液晶屏驱动芯片ST7789
    ESP_LOGD(TAG, "Install LCD driver");
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = BSP_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = BSP_LCD_BITS_PER_PIXEL,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle), err, TAG, "New panel failed");
    
    esp_lcd_panel_reset(panel_handle);  // 液晶屏复位
    esp_lcd_panel_init(panel_handle);  // 初始化配置寄存器
    esp_lcd_panel_invert_color(panel_handle, true); // 颜色反转
    esp_lcd_panel_swap_xy(panel_handle, true);  // 显示翻转 
    esp_lcd_panel_mirror(panel_handle, true, false); // 镜像

    return ret;

err:
    if (panel_handle) {
        esp_lcd_panel_del(panel_handle);
    }
    if (io_handle) {
        esp_lcd_panel_io_del(io_handle);
    }
    spi_bus_free(BSP_LCD_SPI_NUM);
    return ret;
}

// LCD显示初始化
esp_err_t bsp_lcd_init(void)
{
    esp_err_t ret = ESP_OK;

    ret = bsp_display_new(); // 液晶屏驱动初始化
    lcd_set_color(0x0000); // 设置整屏背景黑色
    ret = esp_lcd_panel_disp_on_off(panel_handle, true); // 打开液晶屏显示

    return  ret;
}

// 液晶屏初始化+添加LVGL接口
static lv_disp_t *bsp_display_lcd_init(void)
{
    /* 初始化液晶屏 */
    bsp_display_new(); // 液晶屏驱动初始化
    lcd_set_color(0xffff); // 设置整屏背景白色
    esp_lcd_panel_disp_on_off(panel_handle, true); // 打开液晶屏显示

    /* 液晶屏添加LVGL接口 */
    ESP_LOGD(TAG, "Add LCD screen");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = BSP_LCD_H_RES * BSP_LCD_DRAW_BUF_HEIGHT,   // LVGL缓存大小 
        .double_buffer = true, // 是否开启双缓存
        .hres = BSP_LCD_H_RES, // 液晶屏的宽
        .vres = BSP_LCD_V_RES, // 液晶屏的高
        .monochrome = false,  // 是否单色显示器
        /* Rotation的值必须和液晶屏初始化里面设置的 翻转 和 镜像 一样 */
        .rotation = {
            .swap_xy = true,  // 是否翻转
            .mirror_x = true, // x方向是否镜像
            .mirror_y = false, // y方向是否镜像
        },
        .flags = {
            .buff_dma = true,  // 是否使用DMA 注意：dma与spiram不能同时为true
            .buff_spiram = false, // 是否使用PSRAM 注意：dma与spiram不能同时为true
        }
    };

    return lvgl_port_add_disp(&disp_cfg);
}

// 触摸屏初始化
esp_err_t bsp_touch_new(esp_lcd_touch_handle_t *ret_touch)
{
    /* Initialize touch */
    esp_lcd_touch_config_t tp_cfg = {
        .x_max = BSP_LCD_V_RES,
        .y_max = BSP_LCD_H_RES,
        .rst_gpio_num = GPIO_NUM_NC, // Shared with LCD reset
        .int_gpio_num = GPIO_NUM_NC, 
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 1,
            .mirror_x = 1,
            .mirror_y = 0,
        },
    };
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();

    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)BSP_I2C_NUM, &tp_io_config, &tp_io_handle), TAG, "");
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_ft5x06(tp_io_handle, &tp_cfg, ret_touch));

    return ESP_OK;
}

// 触摸屏初始化+添加LVGL接口
static lv_indev_t *bsp_display_indev_init(lv_disp_t *disp)
{
    /* 初始化触摸屏 */
    ESP_ERROR_CHECK(bsp_touch_new(&tp));
    assert(tp);

    /* 添加LVGL接口 */
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = disp,
        .handle = tp,
    };

    return lvgl_port_add_touch(&touch_cfg);
}

// 开发板显示初始化
void bsp_lvgl_start(void)
{
    /* 初始化LVGL */
    lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&lvgl_cfg);

    /* 初始化液晶屏 并添加LVGL接口 */
    disp = bsp_display_lcd_init();

    /* 初始化触摸屏 并添加LVGL接口 */
    disp_indev = bsp_display_indev_init(disp);
}

// 显示图片
void lcd_draw_pictrue(int x_start, int y_start, int x_end, int y_end, const unsigned char *gImage)
{
    // 分配内存 分配了需要的字节大小 且指定在外部SPIRAM中分配
    size_t pixels_byte_size = (x_end - x_start)*(y_end - y_start) * 2;
    uint16_t *pixels = (uint16_t *)heap_caps_malloc(pixels_byte_size, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
    if (NULL == pixels)
    {
        ESP_LOGE(TAG, "Memory for bitmap is not enough");
        return;
    }
    memcpy(pixels, gImage, pixels_byte_size);  // 把图片数据拷贝到内存
    esp_lcd_panel_draw_bitmap(panel_handle, x_start, y_start, x_end, y_end, (uint16_t *)pixels); // 显示整张图片数据
    heap_caps_free(pixels);  // 释放内存
}

// 设置液晶屏颜色
void lcd_set_color(uint16_t color)
{
    // 分配内存 这里分配了液晶屏一行数据需要的大小
    uint16_t *buffer = (uint16_t *)heap_caps_malloc(BSP_LCD_H_RES * sizeof(uint16_t), MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
    
    if (NULL == buffer)
    {
        ESP_LOGE(TAG, "Memory for bitmap is not enough");
    }
    else
    {
        for (size_t i = 0; i < BSP_LCD_H_RES; i++) // 给缓存中放入颜色数据
        {
            buffer[i] = color;
        }
        for (int y = 0; y < 240; y++) // 显示整屏颜色
        {
            esp_lcd_panel_draw_bitmap(panel_handle, 0, y, 320, y+1, buffer);
        }
        free(buffer); // 释放内存
    }
}
/***************    LCD显示屏 ↑   *************************/
/***********************************************************/
/***********************************************************/
/***************    SPIFFS文件系统 ↓   *********************/

esp_err_t bsp_spiffs_mount(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = SPIFFS_BASE,
        .partition_label = "storage",
        .max_files = 5,
        .format_if_mount_failed = false,
    };

    esp_err_t ret_val = esp_vfs_spiffs_register(&conf);

    ESP_ERROR_CHECK(ret_val);

    size_t total = 0, used = 0;
    ret_val = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret_val != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret_val));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    return ret_val;
}

/***************    SPIFFS文件系统 ↑  *********************/
/**********************************************************/
// 自定义 gai_strerror 实现
const char *gai_strerror(int errcode) {
    switch (errcode) {
        case EAI_FAIL:
            return "Non-recoverable failure in name resolution";
        case EAI_NONAME:
            return "Name or service not known";
        case EAI_AGAIN:
            return "Temporary failure in name resolution";
        case EAI_MEMORY:
            return "Memory allocation failure";
        default:
            return "Unknown error";
    }
}
void test_dns_resolution() {
    struct addrinfo hints = {0};
    struct addrinfo *res;

    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;

    int ret = getaddrinfo("openapi.baidu.com", NULL, &hints, &res);
    if (ret == 0) {
        ESP_LOGI(TAG, "DNS resolution succeeded");
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &((struct sockaddr_in *)res->ai_addr)->sin_addr, ip_str, sizeof(ip_str));
        ESP_LOGI(TAG, "Resolved IP: %s", ip_str);
        freeaddrinfo(res);
    } else {
        ESP_LOGE(TAG, "DNS resolution failed, error: %s", gai_strerror(ret));
    }
}

/**************************** 主函数 **********************************/
void app_main(void)
{
    ESP_LOGI(TAG, "[ 1 ] 初始化NVS");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    /*******  初始化化ES8311音频芯片 (注意：这里面会初始化I2C 后面初始化触摸屏就不用再初始化I2C)  ********/
    ESP_LOGI(TAG, "[ 2 ] 初始化I2C和音频");
    audio_board_handle_t board_handle = audio_board_init();
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_BOTH, AUDIO_HAL_CTRL_START);
    //es8311_codec_set_voice_volume(70); // 最大音量

    bsp_codec_init(); // 音频初始化
 
    ESP_LOGI(TAG, "[ 3 ] 初始化液晶屏");
    bsp_lvgl_start(); // 初始化液晶屏lvgl接口

    my_event_group = xEventGroupCreate();
    /****************** 创建任务 **********************/
    ESP_LOGI(TAG, "[ 4 ] WIFI连接");
    lv_gui_start();
    WIFI_init();
    
    xTaskCreate(main_page_task, "main_page_task", 8192, NULL, 5, NULL);        
    xTaskCreate(ai_chat_task, "ai_chat_task", 8192, NULL, 5, NULL);
    while (1) {
        // raise the task priority of LVGL and/or reduce the handler period can improve the performance
        vTaskDelay(pdMS_TO_TICKS(10));
        // The task running lv_timer_handler should have lower priority than that running `lv_tick_inc`
        // lv_timer_handler();
    }
}
