#include "ws2812b.h"
#include "ll_config.h"
#include <stdint.h>

#ifdef USE_WS2812B

// LED缓冲区 - GRB格式
static RGB_t led_buffer[WS2812B_NUM_LEDS];

// 测试PA0输出 - 用于硬件检测
void WS2812B_TestPA0(void)
{
    // PA0快速翻转，用示波器或逻辑分析仪查看
    for (uint32_t i = 0; i < 1000000; i++) {
        LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_0);
        LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_0);
    }
}

// 发送一个bit
static void send_bit(uint8_t val)
{
    if (val) {
        // 1: 高电平时间长
        GPIOA->BSRR = LL_GPIO_PIN_0;
        __asm__ volatile(
            "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
            "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
            "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
            "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
            "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
        );
        GPIOA->BRR = LL_GPIO_PIN_0;
        __asm__ volatile(
            "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
            "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
            "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
        );
    } else {
        // 0: 高电平时间短
        GPIOA->BSRR = LL_GPIO_PIN_0;
        __asm__ volatile(
            "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
            "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
            "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
        );
        GPIOA->BRR = LL_GPIO_PIN_0;
        __asm__ volatile(
            "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
            "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
            "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
            "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
            "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
        );
    }
}

// 发送一个字节
static void send_byte(uint8_t byte)
{
    uint8_t i;
    for (i = 0; i < 8; i++) {
        send_bit(byte & 0x80);
        byte <<= 1;
    }
}

// 发送复位
static void send_reset(void)
{
    GPIOA->BRR = LL_GPIO_PIN_0;
    LL_mDelay(1);  // 1ms复位足够
}

// WS2812B初始化
void WS2812B_Init(void)
{
    // 使能GPIOA时钟
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOA);

    // 配置PA0为推挽输出
    LL_GPIO_SetPinMode(GPIOA, WS2812B_PIN, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinOutputType(GPIOA, WS2812B_PIN, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinSpeed(GPIOA, WS2812B_PIN, LL_GPIO_SPEED_FREQ_HIGH);

    // 初始低电平
    LL_GPIO_ResetOutputPin(GPIOA, WS2812B_PIN);

    LL_mDelay(10);

    // 清空缓冲区
    WS2812B_Clear();

    // 更新
    WS2812B_Update();
}

void WS2812B_SetPixel(uint8_t index, uint8_t r, uint8_t g, uint8_t b)
{
    if (index < WS2812B_NUM_LEDS) {
        led_buffer[index].r = r;
        led_buffer[index].g = g;
        led_buffer[index].b = b;
    }
}

void WS2812B_SetPixelRGB(uint8_t index, RGB_t color)
{
    if (index < WS2812B_NUM_LEDS) {
        led_buffer[index] = color;
    }
}

void WS2812B_Clear(void)
{
    for (uint8_t i = 0; i < WS2812B_NUM_LEDS; i++) {
        led_buffer[i].r = 0;
        led_buffer[i].g = 0;
        led_buffer[i].b = 0;
    }
}

void WS2812B_Fill(uint8_t r, uint8_t g, uint8_t b)
{
    for (uint8_t i = 0; i < WS2812B_NUM_LEDS; i++) {
        led_buffer[i].r = r;
        led_buffer[i].g = g;
        led_buffer[i].b = b;
    }
}

void WS2812B_Update(void)
{
    uint8_t i;
    uint32_t was_masked;

    // 关中断
    was_masked = __get_PRIMASK();
    __disable_irq();

    // 复位
    send_reset();

    // 发送数据 - GRB顺序
    for (i = 0; i < WS2812B_NUM_LEDS; i++) {
        send_byte(led_buffer[i].g);
        send_byte(led_buffer[i].r);
        send_byte(led_buffer[i].b);
    }

    // 复位锁存
    send_reset();

    // 恢复中断
    if (!was_masked) {
        __enable_irq();
    }
}

void WS2812B_Rainbow(uint8_t offset)
{
    for (uint8_t i = 0; i < WS2812B_NUM_LEDS; i++) {
        uint8_t hue = (i * 3 + offset) % 255;
        RGB_t c;

        // 简化的彩虹
        if (hue < 85) {
            c.r = hue * 3;
            c.g = 255 - hue * 3;
            c.b = 0;
        } else if (hue < 170) {
            hue -= 85;
            c.r = 255 - hue * 3;
            c.g = 0;
            c.b = hue * 3;
        } else {
            hue -= 170;
            c.r = 0;
            c.g = hue * 3;
            c.b = 255 - hue * 3;
        }

        // 降低亮度
        c.r = c.r / 4;
        c.g = c.g / 4;
        c.b = c.b / 4;

        led_buffer[i] = c;
    }
}

// 8x8数字0-9的点阵数据 (8x8)
// 1 = 亮, 0 = 灭
static const uint8_t digit_font[10][8] = {
    // 0
    {0x3C, 0x66, 0x6E, 0x76, 0x66, 0x66, 0x66, 0x3C},
    // 1
    {0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7E},
    // 2
    {0x3C, 0x66, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x7E},
    // 3
    {0x3C, 0x66, 0x06, 0x1C, 0x06, 0x06, 0x66, 0x3C},
    // 4
    {0x0C, 0x1C, 0x3C, 0x6C, 0x7E, 0x0C, 0x0C, 0x0C},
    // 5
    {0x7E, 0x60, 0x60, 0x7C, 0x06, 0x06, 0x66, 0x3C},
    // 6
    {0x1C, 0x30, 0x60, 0x7C, 0x66, 0x66, 0x66, 0x3C},
    // 7
    {0x7E, 0x06, 0x0C, 0x18, 0x30, 0x30, 0x30, 0x30},
    // 8
    {0x3C, 0x66, 0x66, 0x3C, 0x66, 0x66, 0x66, 0x3C},
    // 9
    {0x3C, 0x66, 0x66, 0x66, 0x3E, 0x06, 0x0C, 0x38}
};

// 将(x,y)坐标转换为LED索引
// 假设8x8矩阵排列: 行优先，从左到右，从上到下
// 行0: 0-7, 行1: 8-15, ... 行7: 56-63
static inline uint8_t xy_to_index(uint8_t x, uint8_t y)
{
    if (x >= 8 || y >= 8) return 0;
    return y * 8 + x;
}

// 在指定(x,y)位置画点
void WS2812B_DrawPixel(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t index = xy_to_index(x, y);
    WS2812B_SetPixel(index, r, g, b);
}

// 显示一个数字(0-9)，居中显示
void WS2812B_ShowDigit(uint8_t digit, uint8_t r, uint8_t g, uint8_t b)
{
    if (digit > 9) return;

    WS2812B_Clear();

    for (uint8_t y = 0; y < 8; y++) {
        uint8_t line = digit_font[digit][y];
        for (uint8_t x = 0; x < 8; x++) {
            if (line & (0x80 >> x)) {
                WS2812B_DrawPixel(x, y, r, g, b);
            }
        }
    }

    WS2812B_Update();
}

// 在指定偏移位置显示数字
void WS2812B_ShowDigitAt(uint8_t digit, uint8_t x_offset, uint8_t y_offset, uint8_t r, uint8_t g, uint8_t b)
{
    if (digit > 9) return;

    for (uint8_t y = 0; y < 8; y++) {
        uint8_t line = digit_font[digit][y];
        for (uint8_t x = 0; x < 8; x++) {
            if (line & (0x80 >> x)) {
                uint8_t px = x + x_offset;
                uint8_t py = y + y_offset;
                if (px < 8 && py < 8) {
                    WS2812B_DrawPixel(px, py, r, g, b);
                }
            }
        }
    }
}

#endif // USE_WS2812B
