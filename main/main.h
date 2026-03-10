#include <stdint.h>

/******************************************************************************/
/***************************  I2C ↓ *******************************************/
#define BSP_I2C_SDA           (GPIO_NUM_42)   // SDA引脚
#define BSP_I2C_SCL           (GPIO_NUM_41)   // SCL引脚

#define BSP_I2C_NUM           (0)            // I2C外设
#define BSP_I2C_FREQ_HZ       100000         // 100kHz

/***************************  I2C ↑  *******************************************/
/*******************************************************************************/

/*******************************************************************************/

/******************************************************************************/
/***************************   I2S  ↓    **************************************/

/* Example configurations */
#define EXAMPLE_RECV_BUF_SIZE   (2400)
#define EXAMPLE_SAMPLE_RATE     (16000)
#define EXAMPLE_MCLK_MULTIPLE   (384) // If not using 24-bit data width, 256 should be enough
#define EXAMPLE_MCLK_FREQ_HZ    (EXAMPLE_SAMPLE_RATE * EXAMPLE_MCLK_MULTIPLE)
#define EXAMPLE_VOICE_VOLUME    (70)

/* I2S port and GPIOs */
#define I2S_NUM         (0)
#define I2S_MCK_IO      (GPIO_NUM_40)
#define I2S_BCK_IO      (GPIO_NUM_0)
#define I2S_WS_IO       (GPIO_NUM_2)
#define I2S_DO_IO       (GPIO_NUM_1)
#define I2S_DI_IO       (GPIO_NUM_45)


/***************    I2S ↑   ********************************/
/***********************************************************/


/***********************************************************/
/****************    LCD显示屏 ↓   *************************/
#define BSP_LCD_PIXEL_CLOCK_HZ     (20 * 1000 * 1000)
#define BSP_LCD_SPI_NUM            (SPI3_HOST)
#define LCD_CMD_BITS               (8)
#define LCD_PARAM_BITS             (8)
#define BSP_LCD_BITS_PER_PIXEL     (16)
#define LCD_LEDC_CH          LEDC_CHANNEL_0

#define BSP_LCD_H_RES              (320)
#define BSP_LCD_V_RES              (240)

#define BSP_LCD_SPI_MOSI      (GPIO_NUM_13)
#define BSP_LCD_SPI_CLK       (GPIO_NUM_12)
#define BSP_LCD_SPI_CS        (GPIO_NUM_46)
#define BSP_LCD_DC            (GPIO_NUM_10)
#define BSP_LCD_RST           (GPIO_NUM_NC)
#define BSP_LCD_BACKLIGHT     (GPIO_NUM_NC)     

#define BSP_LCD_DRAW_BUF_HEIGHT    (20)

/***************    LCD显示屏 ↑   *************************/
/***********************************************************/

/***********************************************************/
/****************    摄像头 ↓   ****************************/
#define CAMERA_EN    0
#if CAMERA_EN

#define CAMERA_PIN_PWDN     GPIO_NUM_NC
#define CAMERA_PIN_RESET    GPIO_NUM_NC
#define CAMERA_PIN_XCLK     GPIO_NUM_9
#define CAMERA_PIN_SIOD     GPIO_NUM_39
#define CAMERA_PIN_SIOC     GPIO_NUM_38

#define CAMERA_PIN_D7       GPIO_NUM_18
#define CAMERA_PIN_D6       GPIO_NUM_17
#define CAMERA_PIN_D5       GPIO_NUM_16
#define CAMERA_PIN_D4       GPIO_NUM_15
#define CAMERA_PIN_D3       GPIO_NUM_7
#define CAMERA_PIN_D2       GPIO_NUM_6
#define CAMERA_PIN_D1       GPIO_NUM_5
#define CAMERA_PIN_D0       GPIO_NUM_4
#define CAMERA_PIN_VSYNC    GPIO_NUM_47
#define CAMERA_PIN_HREF     GPIO_NUM_21
#define CAMERA_PIN_PCLK     GPIO_NUM_14

#define XCLK_FREQ_HZ 24000000

#endif
/********************    摄像头 ↑   *************************/
/***********************************************************/


/***********************************************************/
/***************    SPIFFS文件系统 ↓   *********************/
#define SPIFFS_BASE             "/spiffs"

/***************    SPIFFS文件系统 ↑  *********************/
/**********************************************************/


/***********************************************************/
/**********************    SD卡 ↓   *********************/
/* SD card GPIOs */
#define SD_CMD_IO      (13) 
#define SD_CLK_IO      (12)
#define SD_DAT0_IO     (11)

#define SD_MOUNT_POINT     "/sdcard"

/**********************    SD卡 ↑  *********************/
/**********************************************************/


/***********************************************************/
/*********************    音频 ↓   *************************/
#define ADC_I2S_CHANNEL 4

#define VOLUME_DEFAULT    100        // 默认声音大小 0~100

#define CODEC_DEFAULT_SAMPLE_RATE          (16000)
#define CODEC_DEFAULT_BIT_WIDTH            (16)
#define CODEC_DEFAULT_ADC_VOLUME           (24.0)
#define CODEC_DEFAULT_CHANNEL              (2)

#define BSP_I2S_NUM                  I2S_NUM_1

#define GPIO_I2S_LRCK       (GPIO_NUM_2)
#define GPIO_I2S_MCLK       (GPIO_NUM_40)
#define GPIO_I2S_SCLK       (GPIO_NUM_0)
#define GPIO_I2S_SDIN       (GPIO_NUM_45)
#define GPIO_I2S_DOUT       (GPIO_NUM_1)
#define GPIO_PWR_CTRL       (GPIO_NUM_NC)

#define START_MUSIC_COMPLETED            BIT0
#define WIFI_SET_START                   BIT1

/*********************    音频 ↑   *************************/
/***********************************************************/