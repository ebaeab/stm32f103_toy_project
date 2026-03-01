#ifndef WS2812B_H
#define WS2812B_H

#include "ll_config.h"
#include <stdint.h>

#ifdef USE_WS2812B

// WS2812B配置
#define WS2812B_PIN        LL_GPIO_PIN_0   // PA0
#define WS2812B_NUM_LEDS   64              // 8x8 = 64颗LED

// 颜色结构 - GRB格式
typedef struct {
    uint8_t g;
    uint8_t r;
    uint8_t b;
} RGB_t;

// 函数声明
void WS2812B_Init(void);
void WS2812B_SetPixel(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
void WS2812B_SetPixelRGB(uint8_t index, RGB_t color);
void WS2812B_Clear(void);
void WS2812B_Update(void);
void WS2812B_Fill(uint8_t r, uint8_t g, uint8_t b);
void WS2812B_Rainbow(uint8_t offset);
void WS2812B_TestPA0(void);  // PA0测试函数

// 8x8点阵显示函数
void WS2812B_DrawPixel(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b);
void WS2812B_ShowDigit(uint8_t digit, uint8_t r, uint8_t g, uint8_t b);
void WS2812B_ShowDigitAt(uint8_t digit, uint8_t x_offset, uint8_t y_offset, uint8_t r, uint8_t g, uint8_t b);

#else // USE_WS2812B not defined

// WS2812B功能禁用时提供空宏定义
#define WS2812B_Init()
#define WS2812B_SetPixel(i, r, g, b)
#define WS2812B_SetPixelRGB(i, c)
#define WS2812B_Clear()
#define WS2812B_Update()
#define WS2812B_Fill(r, g, b)
#define WS2812B_Rainbow(o)
#define WS2812B_TestPA0()
#define WS2812B_DrawPixel(x, y, r, g, b)
#define WS2812B_ShowDigit(d, r, g, b)
#define WS2812B_ShowDigitAt(d, xo, yo, r, g, b)

#endif // USE_WS2812B

#endif // WS2812B_H
