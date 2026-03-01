#ifndef LCD_UC1701_H
#define LCD_UC1701_H

#include "ll_config.h"
#include <stdint.h>

#ifdef USE_LCD_UC1701

// LCD引脚定义 - 根据实际硬件连接调整
#define LCD_CS_PIN      LL_GPIO_PIN_0   // PB0 - 片选
#define LCD_RST_PIN     LL_GPIO_PIN_1   // PB1 - 复位
#define LCD_RS_PIN      LL_GPIO_PIN_10  // PB10 - 数据/命令选择
#define LCD_SCK_PIN     LL_GPIO_PIN_13  // PB13 - SPI时钟
#define LCD_SDA_PIN     LL_GPIO_PIN_15  // PB15 - SPI数据

// LCD参数定义
#define LCD_WIDTH       128
#define LCD_HEIGHT      64
#define LCD_PAGES       8               // 64/8 = 8页

// UC1701命令定义
typedef enum {
    LCD_CMD_DISPLAY_ON          = 0xAF,
    LCD_CMD_DISPLAY_OFF         = 0xAE,
    LCD_CMD_SET_START_LINE      = 0x40,
    LCD_CMD_SET_PAGE_ADDR       = 0xB0,
    LCD_CMD_SET_COL_ADDR_HIGH   = 0x10,
    LCD_CMD_SET_COL_ADDR_LOW    = 0x00,
    LCD_CMD_SET_ADC_NORMAL      = 0xA0,
    LCD_CMD_SET_ADC_REVERSE     = 0xA1,
    LCD_CMD_SET_DISPLAY_NORMAL  = 0xA6,
    LCD_CMD_SET_DISPLAY_REVERSE = 0xA7,
    LCD_CMD_SET_ALL_PIXELS_ON   = 0xA5,
    LCD_CMD_SET_ALL_PIXELS_OFF  = 0xA4,
    LCD_CMD_SET_BIAS_9          = 0xA2,
    LCD_CMD_SET_BIAS_7          = 0xA3,
    LCD_CMD_RMW                 = 0xE0,
    LCD_CMD_RMW_END             = 0xEE,
    LCD_CMD_RESET               = 0xE2,
    LCD_CMD_SET_COM_NORMAL      = 0xC0,
    LCD_CMD_SET_COM_REVERSE     = 0xC8,
    LCD_CMD_SET_POWER_CONTROL   = 0x28,
    LCD_CMD_SET_RESISTOR_RATIO  = 0x20,
    LCD_CMD_SET_VOLUME_FIRST    = 0x81,
    LCD_CMD_SET_STATIC_INDICATOR = 0xAD,
    LCD_CMD_SET_BOOSTER_RATIO   = 0xF8
} LCD_Command_t;

// 函数声明
void LCD_Init(void);
void LCD_WriteCommand(uint8_t cmd);
void LCD_WriteData(uint8_t data);
void LCD_Clear(void);
void LCD_SetPosition(uint8_t page, uint8_t column);
void LCD_WriteChar(char ch);
void LCD_WriteString(const char *str);
void LCD_DrawPixel(uint8_t x, uint8_t y, uint8_t color);
void LCD_Update(void);
void Display_Graphic_5x7(uint8_t page,uint8_t column,const uint8_t *dp);

// 光标相关函数
void LCD_Cursor_Enable(void);
void LCD_Cursor_Disable(void);
void LCD_Cursor_Blink(void);  // 在主循环中调用，实现闪烁

#else // USE_LCD_UC1701 not defined

// LCD功能禁用时提供空宏定义，避免编译错误
#define LCD_Init()
#define LCD_Clear()
#define LCD_WriteString(s)
#define LCD_Cursor_Enable()
#define LCD_Cursor_Disable()
#define LCD_Cursor_Blink()

#endif // USE_LCD_UC1701

#endif // LCD_UC1701_H