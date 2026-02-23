#include "lcd_uc1701.h"
#include "ll_config.h"
#include <stdint.h>

// 显示缓冲区
static uint8_t displayBuffer[LCD_PAGES][LCD_WIDTH];

// 光标相关变量
static uint8_t cursor_enabled = 0;
static uint8_t cursor_page = 0;
static uint8_t cursor_column = 0;
static uint8_t cursor_state = 0;       // 0=隐藏, 1=显示
static uint32_t cursor_last_tick = 0;
static uint8_t cursor_saved_char = ' '; // 保存光标位置原来的字符

// SPI模拟时序函数
static void LCD_SPI_Write(uint8_t data) {
    // 片选使能
    LL_GPIO_ResetOutputPin(GPIOB, LCD_CS_PIN);
    // 发送8位数据
    for(uint8_t i = 0; i < 8; i++) {
        // 设置时钟低
        LL_GPIO_ResetOutputPin(GPIOB, LCD_SCK_PIN);

        // 设置数据位
        if(data & 0x80) {
            LL_GPIO_SetOutputPin(GPIOB, LCD_SDA_PIN);
        } else {
            LL_GPIO_ResetOutputPin(GPIOB, LCD_SDA_PIN);
        }

        // 时钟上升沿
        LL_GPIO_SetOutputPin(GPIOB, LCD_SCK_PIN);

        data <<= 1;
    }

    // 片选禁用
    LL_GPIO_SetOutputPin(GPIOB, LCD_CS_PIN);
}

// 写命令函数
void LCD_WriteCommand(uint8_t cmd) {
    // 设置RS为命令模式
    LL_GPIO_ResetOutputPin(GPIOB, LCD_RS_PIN);

    // 发送命令
    LCD_SPI_Write(cmd);
}

// 写数据函数
void LCD_WriteData(uint8_t data) {
    // 设置RS为数据模式
    LL_GPIO_SetOutputPin(GPIOB, LCD_RS_PIN);

    // 发送数据
    LCD_SPI_Write(data);
}

// LCD初始化函数
void LCD_Init(void) {
    // 使能GPIOB时钟
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOB);

    // 配置LCD控制引脚为推挽输出
    LL_GPIO_SetPinMode(GPIOB, LCD_CS_PIN, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(GPIOB, LCD_RST_PIN, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(GPIOB, LCD_RS_PIN, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(GPIOB, LCD_SCK_PIN, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(GPIOB, LCD_SDA_PIN, LL_GPIO_MODE_OUTPUT);

    LL_GPIO_SetPinOutputType(GPIOB, LCD_CS_PIN, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinOutputType(GPIOB, LCD_RST_PIN, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinOutputType(GPIOB, LCD_RS_PIN, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinOutputType(GPIOB, LCD_SCK_PIN, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinOutputType(GPIOB, LCD_SDA_PIN, LL_GPIO_OUTPUT_PUSHPULL);

    LL_GPIO_SetPinSpeed(GPIOB, LCD_CS_PIN, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinSpeed(GPIOB, LCD_RST_PIN, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinSpeed(GPIOB, LCD_RS_PIN, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinSpeed(GPIOB, LCD_SCK_PIN, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinSpeed(GPIOB, LCD_SDA_PIN, LL_GPIO_SPEED_FREQ_HIGH);

    // 初始状态设置
    LL_GPIO_SetOutputPin(GPIOB, LCD_CS_PIN);    // 片选高
    LL_GPIO_SetOutputPin(GPIOB, LCD_RS_PIN);    // RS高（数据模式）
    LL_GPIO_SetOutputPin(GPIOB, LCD_SCK_PIN);   // 时钟高

    // LCD硬件复位
    LL_GPIO_ResetOutputPin(GPIOB, LCD_RST_PIN); // 复位低
    LL_mDelay(10);                              // 延时10ms
    LL_GPIO_SetOutputPin(GPIOB, LCD_RST_PIN);   // 复位高
    LL_mDelay(10);                              // 延时10ms

    LCD_WriteCommand(LCD_CMD_RESET);	 //软复位
	LL_mDelay(1);
	LCD_WriteCommand(LCD_CMD_SET_ADC_NORMAL);  //列扫描顺序：从左到右
	LCD_WriteCommand(LCD_CMD_SET_COM_REVERSE);  //行扫描顺序：从下到上，c0从上到下
	LCD_WriteCommand(LCD_CMD_SET_BIAS_9);	 //设置偏压比1/9
	LCD_WriteCommand(0x2f);	 //控制电源
	LCD_WriteCommand(0x25);	 //粗调对比度
	LCD_WriteCommand(LCD_CMD_SET_VOLUME_FIRST);  //微调对比度,进入微调对比度命令
	LCD_WriteCommand(0x19);  //设置电压的参数RR值(对比度请修改此值，调浓增大此值，反之调淡)
	LCD_WriteCommand(LCD_CMD_SET_START_LINE);  //起始行：第一行开始
	LCD_WriteCommand(LCD_CMD_DISPLAY_ON);  //开显示

    // 清屏
    LCD_Clear();
}

// 清屏函数
void LCD_Clear(void) {
    // 清空显示缓冲区
    for(uint8_t page = 0; page < LCD_PAGES; page++) {
        for(uint8_t col = 0; col < LCD_WIDTH; col++) {
            displayBuffer[page][col] = 0x00;
        }
    }

    // 更新到LCD
    LCD_Update();
}

// 设置显示位置
void LCD_SetPosition(uint8_t page, uint8_t column) {
    if(page >= LCD_PAGES) page = LCD_PAGES - 1;
    if(column >= LCD_WIDTH) column = LCD_WIDTH - 1;

    LCD_WriteCommand(LCD_CMD_SET_PAGE_ADDR | page);
    LCD_WriteCommand(LCD_CMD_SET_COL_ADDR_HIGH | (column >> 4));
    LCD_WriteCommand(LCD_CMD_SET_COL_ADDR_LOW | (column & 0x0F));
}

// 绘制像素点
void LCD_DrawPixel(uint8_t x, uint8_t y, uint8_t color) {
    if(x >= LCD_WIDTH || y >= LCD_HEIGHT) return;

    uint8_t page = y / 8;
    uint8_t bit = y % 8;

    if(color) {
        displayBuffer[page][x] |= (1 << bit);
    } else {
        displayBuffer[page][x] &= ~(1 << bit);
    }
}

// 更新显示（将缓冲区内容写入LCD）
void LCD_Update(void) {
    for(uint8_t page = 0; page < LCD_PAGES; page++) {
        LCD_SetPosition(page, 0);

        for(uint8_t col = 0; col < LCD_WIDTH; col++) {
            LCD_WriteData(displayBuffer[page][col]);
        }
    }
}

// 简单的5x7字体（ASCII 32-127）
const uint8_t font5x7[][5] = {
  {  0x00, 0x00, 0x00, 0x00, 0x00, }, // 空格 (32)
  {  0x00, 0x00, 0x5F, 0x00, 0x00, }, // !
  {  0x00, 0x07, 0x00, 0x07, 0x00, }, // "
  {  0x14, 0x7F, 0x14, 0x7F, 0x14, }, // #
  {  0x24, 0x2A, 0x7F, 0x2A, 0x12, }, // $
  {  0x23, 0x13, 0x08, 0x64, 0x62, }, // %
  {  0x36, 0x49, 0x55, 0x22, 0x50, }, // &
  {  0x00, 0x05, 0x03, 0x00, 0x00, }, // '
  {  0x00, 0x1C, 0x22, 0x41, 0x00, }, // (
  {  0x00, 0x41, 0x22, 0x1C, 0x00, }, // )
  {  0x14, 0x08, 0x3E, 0x08, 0x14, }, // *
  {  0x08, 0x08, 0x3E, 0x08, 0x08, }, // +
  {  0x00, 0x50, 0x30, 0x00, 0x00, }, // ,
  {  0x08, 0x08, 0x08, 0x08, 0x08, }, // -
  {  0x00, 0x60, 0x60, 0x00, 0x00, }, // .
  {  0x20, 0x10, 0x08, 0x04, 0x02, }, // /
  {  0x3E, 0x51, 0x49, 0x45, 0x3E, }, // 0
  {  0x00, 0x42, 0x7F, 0x40, 0x00, }, // 1
  {  0x42, 0x61, 0x51, 0x49, 0x46, }, // 2
  {  0x21, 0x41, 0x45, 0x4B, 0x31, }, // 3
  {  0x18, 0x14, 0x12, 0x7F, 0x10, }, // 4
  {  0x27, 0x45, 0x45, 0x45, 0x39, }, // 5
  {  0x3C, 0x4A, 0x49, 0x49, 0x30, }, // 6
  {  0x01, 0x71, 0x09, 0x05, 0x03, }, // 7
  {  0x36, 0x49, 0x49, 0x49, 0x36, }, // 8
  {  0x06, 0x49, 0x49, 0x29, 0x1E, }, // 9
  {  0x00, 0x36, 0x36, 0x00, 0x00, }, // :
  {  0x00, 0x56, 0x36, 0x00, 0x00, }, // ;
  {  0x08, 0x14, 0x22, 0x41, 0x00, }, // <
  {  0x14, 0x14, 0x14, 0x14, 0x14, }, // =
  {  0x00, 0x41, 0x22, 0x14, 0x08, }, // >
  {  0x02, 0x01, 0x51, 0x09, 0x06, }, // ?
  {  0x32, 0x49, 0x79, 0x41, 0x3E, }, // @
  {  0x7E, 0x11, 0x11, 0x11, 0x7E, }, // A
  {  0x7F, 0x49, 0x49, 0x49, 0x36, }, // B
  {  0x3E, 0x41, 0x41, 0x41, 0x22, }, // C
  {  0x7F, 0x41, 0x41, 0x22, 0x1C, }, // D
  {  0x7F, 0x49, 0x49, 0x49, 0x41, }, // E
  {  0x7F, 0x09, 0x09, 0x09, 0x01, }, // F
  {  0x3E, 0x41, 0x49, 0x49, 0x7A, }, // G
  {  0x7F, 0x08, 0x08, 0x08, 0x7F, }, // H
  {  0x00, 0x41, 0x7F, 0x41, 0x00, }, // I
  {  0x20, 0x40, 0x41, 0x3F, 0x01, }, // J
  {  0x7F, 0x08, 0x14, 0x22, 0x41, }, // K
  {  0x7F, 0x40, 0x40, 0x40, 0x40, }, // L
  {  0x7F, 0x02, 0x0C, 0x02, 0x7F, }, // M
  {  0x7F, 0x04, 0x08, 0x10, 0x7F, }, // N
  {  0x3E, 0x41, 0x41, 0x41, 0x3E, }, // O
  {  0x7F, 0x09, 0x09, 0x09, 0x06, }, // P
  {  0x3E, 0x41, 0x51, 0x21, 0x5E, }, // Q
  {  0x7F, 0x09, 0x19, 0x29, 0x46, }, // R
  {  0x46, 0x49, 0x49, 0x49, 0x31, }, // S
  {  0x01, 0x01, 0x7F, 0x01, 0x01, }, // T
  {  0x3F, 0x40, 0x40, 0x40, 0x3F, }, // U
  {  0x1F, 0x20, 0x40, 0x20, 0x1F, }, // V
  {  0x3F, 0x40, 0x38, 0x40, 0x3F, }, // W
  {  0x63, 0x14, 0x08, 0x14, 0x63, }, // X
  {  0x07, 0x08, 0x70, 0x08, 0x07, }, // Y
  {  0x61, 0x51, 0x49, 0x45, 0x43, }, // Z
  {  0x00, 0x7F, 0x41, 0x41, 0x00, }, // [
  {  0x02, 0x04, 0x08, 0x10, 0x20, }, // (\)
  {  0x00, 0x41, 0x41, 0x7F, 0x00, }, // ]
  {  0x04, 0x02, 0x01, 0x02, 0x04, }, // ^
  {  0x40, 0x40, 0x40, 0x40, 0x40, }, // _
  {  0x00, 0x01, 0x02, 0x04, 0x00, }, // `
  {  0x20, 0x54, 0x54, 0x54, 0x78, }, // a
  {  0x7F, 0x48, 0x44, 0x44, 0x38, }, // b
  {  0x38, 0x44, 0x44, 0x44, 0x20, }, // c
  {  0x38, 0x44, 0x44, 0x48, 0x7F, }, // d
  {  0x38, 0x54, 0x54, 0x54, 0x18, }, // e
  {  0x08, 0x7E, 0x09, 0x01, 0x02, }, // f
  {  0x0C, 0x52, 0x52, 0x52, 0x3E, }, // g
  {  0x7F, 0x08, 0x04, 0x04, 0x78, }, // h
  {  0x00, 0x44, 0x7D, 0x40, 0x00, }, // i
  {  0x20, 0x40, 0x44, 0x3D, 0x00, }, // j
  {  0x7F, 0x10, 0x28, 0x44, 0x00, }, // k
  {  0x00, 0x41, 0x7F, 0x40, 0x00, }, // l
  {  0x7C, 0x04, 0x18, 0x04, 0x78, }, // m
  {  0x7C, 0x08, 0x04, 0x04, 0x78, }, // n
  {  0x38, 0x44, 0x44, 0x44, 0x38, }, // o
  {  0x7C, 0x14, 0x14, 0x14, 0x08, }, // p
  {  0x08, 0x14, 0x14, 0x18, 0x7C, }, // q
  {  0x7C, 0x08, 0x04, 0x04, 0x08, }, // r
  {  0x48, 0x54, 0x54, 0x54, 0x20, }, // s
  {  0x04, 0x3F, 0x44, 0x40, 0x20, }, // t
  {  0x3C, 0x40, 0x40, 0x20, 0x7C, }, // u
  {  0x1C, 0x20, 0x40, 0x20, 0x1C, }, // v
  {  0x3C, 0x40, 0x30, 0x40, 0x3C, }, // w
  {  0x44, 0x28, 0x10, 0x28, 0x44, }, // x
  {  0x0C, 0x50, 0x50, 0x50, 0x3C, }, // y
  {  0x44, 0x64, 0x54, 0x4C, 0x44, }, // z
  {  0x00, 0x08, 0x36, 0x41, 0x00, }, // {
  {  0x00, 0x00, 0x7F, 0x00, 0x00, }, // |
  {  0x00, 0x41, 0x36, 0x08, 0x00, }, // }
  {  0x08, 0x04, 0x08, 0x10, 0x08, }, // ~
};

uint8_t g_column = 0;
uint8_t g_page = 0;

// 清除指定行
static void LCD_ClearLine(uint8_t page) {
    LCD_SetPosition(page, 0);
    for(uint8_t i = 0; i < LCD_WIDTH; i++) {
        LCD_WriteData(0x00);
    }
}

// 写单个字符
void LCD_WriteChar(char ch) {
    if(ch == 0x0A)
    {
        // 移动到下一行
        g_page++;
        g_column = 0;
        if (g_page == LCD_PAGES) {
            g_page = 0;
        }
        // 清除新的一行
        LCD_ClearLine(g_page);
        // 更新光标保存的字符为空格
        cursor_saved_char = ' ';
        return;
    }
    if(ch < 32 || ch > 126) ch = ' '; // 非可打印字符显示为空格
    const uint8_t *fontPtr = font5x7[(ch - 32)];
    Display_Graphic_5x7(g_page,(g_column++)*8+1,fontPtr);

    if (g_column == 16) {
        g_column = 0;
        g_page++;
        if (g_page == LCD_PAGES) {
            g_page = 0;
        }
    }
    // 更新光标保存的字符为空格（下一个位置）
    cursor_saved_char = ' ';
}

// 写字符串
void LCD_WriteString(const char *str) {
    while(*str) {
        LCD_WriteChar(*str++);
    }
}

void Display_Graphic_5x7(uint8_t page,uint8_t column,const uint8_t *dp)
{
	uint8_t col_cnt;
    LCD_SetPosition(page,column);
	for (col_cnt=0;col_cnt<5;col_cnt++)
	{
		LCD_WriteData(*dp);
		dp++;
	}
}

// 使能光标
void LCD_Cursor_Enable(void) {
    cursor_enabled = 1;
    cursor_page = g_page;
    cursor_column = g_column;
    cursor_state = 0;
    cursor_last_tick = 0;
}

// 禁用光标
void LCD_Cursor_Disable(void) {
    // 如果光标正在显示，先恢复原来的字符
    if (cursor_enabled && cursor_state) {
        const uint8_t *fontPtr = font5x7[(cursor_saved_char - 32)];
        Display_Graphic_5x7(cursor_page, cursor_column * 8 + 1, fontPtr);
    }
    cursor_enabled = 0;
}

// 光标闪烁（需要在主循环中定期调用）
void LCD_Cursor_Blink(void) {
    extern uint32_t SysTick_Counter;

    if (!cursor_enabled) return;

    // 更新光标位置到当前写入位置
    cursor_page = g_page;
    cursor_column = g_column;

    // 每500ms切换一次状态
    if (SysTick_Counter - cursor_last_tick >= 500) {
        cursor_last_tick = SysTick_Counter;
        cursor_state = !cursor_state;

        uint8_t x = cursor_column * 8 + 1;
        uint8_t y = cursor_page;

        if (cursor_state) {
            // 显示光标（一个实心方块）
            LCD_SetPosition(y, x);
            for (uint8_t i = 0; i < 5; i++) {
                LCD_WriteData(0xFF);  // 全部点亮
            }
        } else {
            // 恢复原来的字符
            const uint8_t *fontPtr = font5x7[(cursor_saved_char - 32)];
            Display_Graphic_5x7(y, x, fontPtr);
        }
    }
}
