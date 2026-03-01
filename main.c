#include "ll_config.h"
#ifdef USE_LCD_UC1701
#include "lcd_uc1701.h"
#endif
#ifdef USE_WS2812B
#include "ws2812b.h"
#endif
#include <stdint.h>
#include <string.h>

#define UART_RX_BUFFER_SIZE 128

uint32_t SysTick_Counter = 0;

// 串口接收全局缓冲区
volatile uint8_t g_uart_rx_buffer[UART_RX_BUFFER_SIZE];
volatile uint16_t g_uart_rx_index = 0;
volatile uint8_t g_uart_rx_complete = 0;

// 时间偏移（秒，从2000-01-01 00:00:00开始）
static uint64_t time_offset_sec = 0;

// 命令函数类型定义
typedef void (*CommandFunc)(void);

// 命令表结构
typedef struct {
    const char* name;
    CommandFunc func;
} CommandEntry;

// 命令函数声明
void Cmd_Date(void);
void Cmd_DateD(void);
void Cmd_DateS(void);
void Cmd_Clear(void);
void Cmd_Ls(void);
void Cmd_Default(void);
#ifdef USE_WS2812B
void Cmd_Led(void);
void Cmd_LedSet(void);
void Cmd_LedFill(void);
void Cmd_LedRainbow(void);
void Cmd_LedClear(void);
void Cmd_LedNum(void);
void Cmd_LedTest(void);
#endif

// 命令表（不包含子命令）
CommandEntry cmd_table[] = {
    {"date", Cmd_Date},
    {"clear", Cmd_Clear},
    {"ls", Cmd_Ls},
#ifdef USE_WS2812B
    {"led", Cmd_Led},
#endif
};
#define CMD_TABLE_SIZE (sizeof(cmd_table) / sizeof(cmd_table[0]))

// 系统时钟配置函数 - 使用外部8MHz HSE晶振
void SystemClock_Config(void)
{
  /* Set FLASH latency */
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_2);

  /* Enable HSE oscillator */
  LL_RCC_HSE_Enable();
  while(LL_RCC_HSE_IsReady() != 1)
  {
  };

  /* Main PLL configuration and activation */
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE_DIV_1, LL_RCC_PLL_MUL_9);

  LL_RCC_PLL_Enable();
  while(LL_RCC_PLL_IsReady() != 1)
  {
  };

  /* Sysclk activation on the main PLL */
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
  {
  };

  /* Set APB1 & APB2 prescaler*/
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_2);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);

  /* Set systick to 1ms in using frequency set to 72MHz */
  LL_Init1msTick(72000000);

  /* Update CMSIS variable (which can be updated also through SystemCoreClockUpdate function) */
  LL_SetSystemCoreClock(72000000);
  LL_SYSTICK_EnableIT();
  NVIC_SetPriority(SysTick_IRQn, 0);
}

// LED初始化函数（PB12）- 使用真正LL库
void LED_Init(void) {
    // 使能GPIOB时钟
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOB);

    // 配置PB12为推挽输出（LED）
    LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_12, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinOutputType(GPIOB, LL_GPIO_PIN_12, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinSpeed(GPIOB, LL_GPIO_PIN_12, LL_GPIO_SPEED_FREQ_HIGH);

    // 初始状态：LED关闭
    LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_12);
}

// LED控制函数 - 使用真正LL库
void LED_Toggle(void) {
    LL_GPIO_TogglePin(GPIOB, LL_GPIO_PIN_12);
}

// 串口初始化函数 - 使用真正LL库
void USART1_Init(void) {
    // 使能USART1和GPIOA时钟
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOA);

    // 配置PA9为推挽输出（TX）
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_9, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetPinOutputType(GPIOA, LL_GPIO_PIN_9, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_9, LL_GPIO_SPEED_FREQ_HIGH);

    // 配置PA10为上拉输入（RX）
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_10, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_10, LL_GPIO_PULL_UP);

    // 配置USART参数
    LL_RCC_ClocksTypeDef RCC_Clocks;
    LL_RCC_GetSystemClocksFreq(&RCC_Clocks);
    LL_USART_SetBaudRate(USART1, RCC_Clocks.PCLK2_Frequency, 115200);
    LL_USART_SetDataWidth(USART1, LL_USART_DATAWIDTH_8B);
    LL_USART_SetStopBitsLength(USART1, LL_USART_STOPBITS_1);
    LL_USART_SetParity(USART1, LL_USART_PARITY_NONE);
    LL_USART_SetTransferDirection(USART1, LL_USART_DIRECTION_TX_RX);
    LL_USART_SetHWFlowCtrl(USART1, LL_USART_HWCONTROL_NONE);

    // 使能接收中断
    LL_USART_EnableIT_RXNE(USART1);

    // 配置NVIC
    NVIC_SetPriority(USART1_IRQn, 1);
    NVIC_EnableIRQ(USART1_IRQn);

    // 使能USART
    LL_USART_Enable(USART1);
}

// 串口发送单个字符 - 使用真正LL库
void USART1_SendChar(char ch) {
    // 等待发送缓冲区为空
    while(!LL_USART_IsActiveFlag_TXE(USART1));

    // 发送字符
    LL_USART_TransmitData8(USART1, ch);
}

// 串口发送字符串
void USART1_SendString(const char *str) {
    while(*str) {
        USART1_SendChar(*str++);
    }
}

extern uint8_t g_column;
extern uint8_t g_page;

// 简单的uint32转字符串
void uint32_to_str(uint32_t num, char* buf) {
    char temp[12];
    int i = 0, j = 0;
    if (num == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    while (num > 0) {
        temp[i++] = '0' + (num % 10);
        num = num / 10;
    }
    while (i > 0) {
        buf[j++] = temp[--i];
    }
    buf[j] = '\0';
}

// 判断是否是闰年
static uint8_t is_leap_year(uint16_t year) {
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
        return 1;
    }
    return 0;
}

// 获取某年某月的天数
static uint8_t days_in_month(uint16_t year, uint8_t month) {
    static const uint8_t days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (month == 2 && is_leap_year(year)) {
        return 29;
    }
    return days[month - 1];
}

// 星期几名称
static const char* week_days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

// 月份名称
static const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                               "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

// 数字转2位字符串，补零
static void uint8_to_2str(uint8_t num, char* buf) {
    buf[0] = '0' + (num / 10);
    buf[1] = '0' + (num % 10);
    buf[2] = '\0';
}

// 数字转4位字符串
static void uint16_to_4str(uint16_t num, char* buf) {
    buf[0] = '0' + (num / 1000);
    buf[1] = '0' + ((num / 100) % 10);
    buf[2] = '0' + ((num / 10) % 10);
    buf[3] = '0' + (num % 10);
    buf[4] = '\0';
}

// 时间结构
typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t day_of_week;
} DateTime;

// 将DateTime转换为从2000-01-01开始的秒数
static uint64_t DateTimeToSeconds(DateTime* dt) {
    uint64_t seconds = 0;

    // 计算年
    for (uint16_t y = 2000; y < dt->year; y++) {
        seconds += is_leap_year(y) ? 366 : 365;
    }

    // 计算月
    for (uint8_t m = 1; m < dt->month; m++) {
        seconds += days_in_month(dt->year, m);
    }

    // 计算日
    seconds += (dt->day - 1);

    // 转换为秒
    seconds = seconds * 24 * 60 * 60;
    seconds += dt->hour * 60 * 60;
    seconds += dt->minute * 60;
    seconds += dt->second;

    return seconds;
}

// 计算当前时间
static void GetDateTime(DateTime* dt) {
    uint64_t total_seconds = (SysTick_Counter / 1000) + time_offset_sec;

    dt->year = 2000;
    dt->month = 1;
    dt->day = 1;
    dt->hour = 0;
    dt->minute = 0;
    dt->second = 0;

    // 计算时分秒
    dt->second = total_seconds % 60;
    total_seconds = total_seconds / 60;
    dt->minute = total_seconds % 60;
    total_seconds = total_seconds / 60;
    dt->hour = total_seconds % 24;
    total_seconds = total_seconds / 24;

    // 计算星期几（2000-01-01是周六）
    dt->day_of_week = (6 + total_seconds) % 7;

    // 计算年月日
    while (1) {
        uint16_t year_days = is_leap_year(dt->year) ? 366 : 365;
        if (total_seconds < year_days) {
            break;
        }
        total_seconds -= year_days;
        dt->year++;
    }

    while (1) {
        uint8_t month_days = days_in_month(dt->year, dt->month);
        if (total_seconds < month_days) {
            break;
        }
        total_seconds -= month_days;
        dt->month++;
    }

    dt->day = total_seconds + 1;
}

// date命令：显示当前时间（从2000-01-01 00:00:00开始）
void Cmd_Date(void) {
    char buf[64];
    char num_buf[8];
    DateTime dt;

    GetDateTime(&dt);

    // 格式: "Wed Feb 21 14:30:45 2024"
    strcpy(buf, week_days[dt.day_of_week]);
    strcat(buf, " ");
    strcat(buf, months[dt.month - 1]);
    strcat(buf, " ");
    uint8_to_2str(dt.day, num_buf);
    strcat(buf, num_buf);
    strcat(buf, " ");
    uint8_to_2str(dt.hour, num_buf);
    strcat(buf, num_buf);
    strcat(buf, ":");
    uint8_to_2str(dt.minute, num_buf);
    strcat(buf, num_buf);
    strcat(buf, ":");
    uint8_to_2str(dt.second, num_buf);
    strcat(buf, num_buf);
    strcat(buf, " ");
    uint16_to_4str(dt.year, num_buf);
    strcat(buf, num_buf);
    strcat(buf, "\r\n");

    USART1_SendString(buf);

#ifdef USE_LCD_UC1701
    // LCD只显示时分秒
    uint8_to_2str(dt.hour, num_buf);
    strcpy(buf, num_buf);
    strcat(buf, ":");
    uint8_to_2str(dt.minute, num_buf);
    strcat(buf, num_buf);
    strcat(buf, ":");
    uint8_to_2str(dt.second, num_buf);
    strcat(buf, num_buf);
    strcat(buf, "\n");
    LCD_WriteString(buf);
#endif
}

// dated命令：显示年月日
void Cmd_DateD(void) {
    char buf[64];
    char num_buf[8];
    DateTime dt;

    GetDateTime(&dt);

    // 串口完整显示
    strcpy(buf, week_days[dt.day_of_week]);
    strcat(buf, " ");
    strcat(buf, months[dt.month - 1]);
    strcat(buf, " ");
    uint8_to_2str(dt.day, num_buf);
    strcat(buf, num_buf);
    strcat(buf, " ");
    uint8_to_2str(dt.hour, num_buf);
    strcat(buf, num_buf);
    strcat(buf, ":");
    uint8_to_2str(dt.minute, num_buf);
    strcat(buf, num_buf);
    strcat(buf, ":");
    uint8_to_2str(dt.second, num_buf);
    strcat(buf, num_buf);
    strcat(buf, " ");
    uint16_to_4str(dt.year, num_buf);
    strcat(buf, num_buf);
    strcat(buf, "\r\n");

    USART1_SendString(buf);

#ifdef USE_LCD_UC1701
    // LCD只显示年月日
    strcpy(buf, months[dt.month - 1]);
    strcat(buf, " ");
    uint8_to_2str(dt.day, num_buf);
    strcat(buf, num_buf);
    strcat(buf, " ");
    uint16_to_4str(dt.year, num_buf);
    strcat(buf, num_buf);
    strcat(buf, "\n");
    LCD_WriteString(buf);
#endif
}

// 简单的字符串转数字
static uint32_t str_to_uint(const char* str, uint8_t len) {
    uint32_t num = 0;
    for (uint8_t i = 0; i < len && str[i] >= '0' && str[i] <= '9'; i++) {
        num = num * 10 + (str[i] - '0');
    }
    return num;
}

// date -s命令：设置时间，格式 "YYYY-MM-DD HH:MM:SS"
void Cmd_DateS(void) {
    char* cmd = (char*)g_uart_rx_buffer;
    // 格式: "date -s YYYY-MM-DD HH:MM:SS"
    // 找到 "-s " 后面的部分
    char* time_str = strstr(cmd, "-s ");
    if (!time_str) {
        USART1_SendString("Usage: date -s YYYY-MM-DD HH:MM:SS\r\n");
#ifdef USE_LCD_UC1701
        LCD_WriteString("Err: format\n");
#endif
        return;
    }
    time_str += 3;  // 跳过 "-s "

    DateTime dt;
    // 解析: YYYY-MM-DD HH:MM:SS
    if (strlen(time_str) < 19) {
        USART1_SendString("Usage: date -s YYYY-MM-DD HH:MM:SS\r\n");
#ifdef USE_LCD_UC1701
        LCD_WriteString("Err: format\n");
#endif
        return;
    }

    dt.year = str_to_uint(time_str, 4);
    dt.month = str_to_uint(time_str + 5, 2);
    dt.day = str_to_uint(time_str + 8, 2);
    dt.hour = str_to_uint(time_str + 11, 2);
    dt.minute = str_to_uint(time_str + 14, 2);
    dt.second = str_to_uint(time_str + 17, 2);

    // 简单验证
    if (dt.year < 2000 || dt.year > 2100 ||
        dt.month < 1 || dt.month > 12 ||
        dt.day < 1 || dt.day > 31 ||
        dt.hour > 23 || dt.minute > 59 || dt.second > 59) {
        USART1_SendString("Invalid date/time\r\n");
#ifdef USE_LCD_UC1701
        LCD_WriteString("Err: invalid\n");
#endif
        return;
    }

    // 计算目标时间与2000-01-01的差值
    uint64_t target_sec = DateTimeToSeconds(&dt);
    // 设置时间偏移 = 目标时间 - 当前运行时间
    time_offset_sec = target_sec - (SysTick_Counter / 1000);

    USART1_SendString("Time set\r\n");
#ifdef USE_LCD_UC1701
    LCD_WriteString("Time set\n");
#endif
}

// clear命令：清除整个屏幕
void Cmd_Clear(void) {
#ifdef USE_LCD_UC1701
    extern uint8_t g_column;
    extern uint8_t g_page;
    LCD_Clear();
    g_column = 0;
    g_page = 0;
#endif
    USART1_SendString("Screen cleared\r\n");
}

// ls命令：列出所有支持的命令
void Cmd_Ls(void) {
    USART1_SendString("Available commands:\r\n");
    for (uint8_t i = 0; i < CMD_TABLE_SIZE; i++) {
        USART1_SendString("  ");
        USART1_SendString(cmd_table[i].name);
#ifdef USE_LCD_UC1701
        LCD_WriteString(cmd_table[i].name);
        LCD_WriteString(" ");
#endif
    }
    USART1_SendString("\r\n");
#ifdef USE_WS2812B
    USART1_SendString("  Use 'led' for LED subcommands\r\n");
#endif
#ifdef USE_LCD_UC1701
    LCD_WriteString("\n");
#endif
}

// 默认命令
void Cmd_Default(void) {
    USART1_SendString("Unknown cmd: ");
    USART1_SendString((const char*)g_uart_rx_buffer);
    USART1_SendString("\r\n");
#ifdef USE_LCD_UC1701
    LCD_WriteString("Err:");
    LCD_WriteString((const char*)g_uart_rx_buffer);
    LCD_WriteString("\n");
#endif
}

// 命令解析执行函数
void ProcessCommand(void) {
#ifdef USE_LCD_UC1701
    // LCD回显命令
    LCD_WriteString("> ");
    LCD_WriteString((const char*)g_uart_rx_buffer);
    LCD_WriteString("\n");
#endif

    uint8_t found = 0;
    char* cmd = (char*)g_uart_rx_buffer;

    // 检查是否是date子命令
    if (strncmp(cmd, "date ", 5) == 0) {
        char* subcmd = cmd + 5;
        if (strcmp(subcmd, "-d") == 0) {
            Cmd_DateD();
            found = 1;
        } else if (strncmp(subcmd, "-s ", 3) == 0) {
            Cmd_DateS();
            found = 1;
        }
    }

#ifdef USE_WS2812B
    // 检查是否是led子命令
    if (strncmp(cmd, "led ", 4) == 0) {
        char* subcmd = cmd + 4;
        if (strncmp(subcmd, "set ", 4) == 0) {
            Cmd_LedSet();
            found = 1;
        } else if (strncmp(subcmd, "fill ", 5) == 0) {
            Cmd_LedFill();
            found = 1;
        } else if (strcmp(subcmd, "rainbow") == 0) {
            Cmd_LedRainbow();
            found = 1;
        } else if (strcmp(subcmd, "clear") == 0) {
            Cmd_LedClear();
            found = 1;
        } else if (strncmp(subcmd, "num ", 4) == 0) {
            Cmd_LedNum();
            found = 1;
        } else if (strcmp(subcmd, "test") == 0) {
            Cmd_LedTest();
            found = 1;
        }
    }
#endif

    // 普通命令匹配
    if (!found) {
        for (uint8_t i = 0; i < CMD_TABLE_SIZE; i++) {
            if (strcmp(cmd, cmd_table[i].name) == 0) {
                cmd_table[i].func();
                found = 1;
                break;
            }
        }
    }

    if (!found) {
        Cmd_Default();
    }
}

int main(void) {
    // 配置系统时钟 - 使用外部8MHz HSE晶振
    SystemClock_Config();

    // LED初始化
    LED_Init();

    // 串口初始化
    USART1_Init();

#ifdef USE_LCD_UC1701
    // LCD初始化
    LCD_Init();
#endif

#ifdef USE_WS2812B
    // WS2812B初始化
    WS2812B_Init();
#endif

    // 发送启动消息
    USART1_SendString("STM32F103C8T6 UC1701 LCD12864 Demo Started!\r\n");
    USART1_SendString("Clock: External 8MHz HSE (Real LL Library)\r\n");
    USART1_SendString("Baudrate: 115200\r\n");
#ifdef USE_LCD_UC1701
    USART1_SendString("LCD on PB0,1,10,13,15\r\n");
#endif
#ifdef USE_WS2812B
    USART1_SendString("WS2812B on PA0 (64 LEDs)\r\n");
#endif
    USART1_SendString("LED on PB12 will blink every 500ms\r\n");
    USART1_SendString("UART Rx enabled (interrupt mode, buffer size: 128)\r\n");
#ifdef USE_WS2812B
    USART1_SendString("Available commands: date, clear, ls, led\r\n");
#else
    USART1_SendString("Available commands: date, clear, ls\r\n");
#endif

#ifdef USE_LCD_UC1701
    LCD_WriteString(":) Welcome!\n");

    // 使能光标闪烁
    LCD_Cursor_Enable();
#endif

#ifdef USE_WS2812B
    // 先简单清屏
    WS2812B_Clear();
    WS2812B_Update();
#endif

    // 主循环
    while(1) {
#ifdef USE_LCD_UC1701
        // 光标闪烁
        LCD_Cursor_Blink();
#endif

        // 检查是否有新的串口数据接收完成
        if (g_uart_rx_complete) {
            USART1_SendString("> ");
            USART1_SendString((const char*)g_uart_rx_buffer);
            USART1_SendString("\r\n");

            // 处理命令
            ProcessCommand();

            // 清空缓冲区标志，准备下一次接收
            g_uart_rx_complete = 0;
            g_uart_rx_index = 0;
        }

        // 翻转LED状态
        LED_Toggle();

        // 延时约100ms - 加快更新速度
        LL_mDelay(100);
    }
}

// USART1中断处理函数
void USART1_IRQHandler(void) {
    // 检查是否是接收中断
    if (LL_USART_IsActiveFlag_RXNE(USART1) && LL_USART_IsEnabledIT_RXNE(USART1)) {
        uint8_t ch = LL_USART_ReceiveData8(USART1);  // 读取数据自动清除RXNE标志

        // 如果缓冲区未满，存储字符
        if (g_uart_rx_index < (UART_RX_BUFFER_SIZE - 1)) {
            // 接收到换行或回车表示一帧结束
            if (ch == '\n' || ch == '\r') {
                g_uart_rx_buffer[g_uart_rx_index] = '\0';
                g_uart_rx_complete = 1;
            } else {
                g_uart_rx_buffer[g_uart_rx_index++] = ch;
            }
        } else {
            // 缓冲区满，自动结束
            g_uart_rx_buffer[UART_RX_BUFFER_SIZE - 1] = '\0';
            g_uart_rx_complete = 1;
        }
    }

    // 检查并清除溢出错误标志（如果不清除会导致中断一直触发）
    if (LL_USART_IsActiveFlag_ORE(USART1)) {
        LL_USART_ClearFlag_ORE(USART1);
    }
}

// 中断服务函数（空实现）
void NMI_Handler(void) {}
void HardFault_Handler(void) { while(1); }
void MemManage_Handler(void) { while(1); }
void BusFault_Handler(void) { while(1); }
void UsageFault_Handler(void) { while(1); }
void SVC_Handler(void) {}
void DebugMon_Handler(void) {}
void PendSV_Handler(void) {}
void SysTick_Handler(void) {
    SysTick_Counter++;
}

#ifdef USE_WS2812B

// led命令：显示帮助
void Cmd_Led(void) {
    USART1_SendString("LED commands:\r\n");
    USART1_SendString("  led set <index> <r> <g> <b>  - Set single LED (0-63)\r\n");
    USART1_SendString("  led fill <r> <g> <b>           - Fill all LEDs\r\n");
    USART1_SendString("  led rainbow                    - Rainbow effect\r\n");
    USART1_SendString("  led clear                      - Clear all LEDs\r\n");
    USART1_SendString("  led num <digit> [r] [g] [b]    - Show digit 0-9\r\n");
    USART1_SendString("  led test                       - Test digits 0-9 in loop\r\n");
}

// 简单的字符串转数字
static uint8_t str_to_uint8(const char* str) {
    uint32_t num = 0;
    while (*str >= '0' && *str <= '9') {
        num = num * 10 + (*str - '0');
        str++;
    }
    if (num > 255) num = 255;
    return (uint8_t)num;
}

// 跳过空格
static const char* skip_space(const char* str) {
    while (*str == ' ') str++;
    return str;
}

// 查找下一个空格
static const char* find_space(const char* str) {
    while (*str && *str != ' ') str++;
    return str;
}

// led set命令：设置单个LED
void Cmd_LedSet(void) {
    char* cmd = (char*)g_uart_rx_buffer;
    // 格式: "led set <index> <r> <g> <b>"
    char* p = strstr(cmd, "set ");
    if (!p) {
        USART1_SendString("Usage: led set <index> <r> <g> <b>\r\n");
        return;
    }
    p += 4;

    // 解析参数
    const char* s = skip_space(p);
    const char* end = find_space(s);
    if (s == end) {
        USART1_SendString("Usage: led set <index> <r> <g> <b>\r\n");
        return;
    }
    uint8_t index = str_to_uint8(s);
    if (index >= 64) {
        USART1_SendString("Index must be 0-63\r\n");
        return;
    }

    s = skip_space(end);
    end = find_space(s);
    if (s == end) {
        USART1_SendString("Usage: led set <index> <r> <g> <b>\r\n");
        return;
    }
    uint8_t r = str_to_uint8(s);

    s = skip_space(end);
    end = find_space(s);
    if (s == end) {
        USART1_SendString("Usage: led set <index> <r> <g> <b>\r\n");
        return;
    }
    uint8_t g = str_to_uint8(s);

    s = skip_space(end);
    uint8_t b = str_to_uint8(s);

    WS2812B_SetPixel(index, r, g, b);
    WS2812B_Update();

    USART1_SendString("LED set\r\n");
}

// led fill命令：填充所有LED
void Cmd_LedFill(void) {
    char* cmd = (char*)g_uart_rx_buffer;
    char* p = strstr(cmd, "fill ");
    if (!p) {
        USART1_SendString("Usage: led fill <r> <g> <b>\r\n");
        return;
    }
    p += 5;

    // 解析参数
    const char* s = skip_space(p);
    const char* end = find_space(s);
    if (s == end) {
        USART1_SendString("Usage: led fill <r> <g> <b>\r\n");
        return;
    }
    uint8_t r = str_to_uint8(s);

    s = skip_space(end);
    end = find_space(s);
    if (s == end) {
        USART1_SendString("Usage: led fill <r> <g> <b>\r\n");
        return;
    }
    uint8_t g = str_to_uint8(s);

    s = skip_space(end);
    uint8_t b = str_to_uint8(s);

    WS2812B_Fill(r, g, b);
    WS2812B_Update();

    USART1_SendString("LED fill\r\n");
}

// led rainbow命令：彩虹效果
void Cmd_LedRainbow(void) {
    static uint8_t rainbow_offset = 0;
    WS2812B_Rainbow(rainbow_offset);
    WS2812B_Update();
    rainbow_offset += 10;
    USART1_SendString("Rainbow\r\n");
}

// led clear命令：清除所有LED
void Cmd_LedClear(void) {
    WS2812B_Clear();
    WS2812B_Update();
    USART1_SendString("LED clear\r\n");
}

// led num命令：显示单个数字
void Cmd_LedNum(void) {
    char* cmd = (char*)g_uart_rx_buffer;
    char* p = strstr(cmd, "num ");
    if (!p) {
        USART1_SendString("Usage: led num <digit> [r] [g] [b]\r\n");
        return;
    }
    p += 4;

    const char* s = skip_space(p);
    if (*s < '0' || *s > '9') {
        USART1_SendString("Usage: led num <digit> [r] [g] [b]\r\n");
        return;
    }
    uint8_t digit = *s - '0';

    // 默认红色
    uint8_t r = 64, g = 0, b = 0;

    // 解析可选的颜色参数
    s = find_space(s);
    if (*s) {
        s = skip_space(s);
        const char* end = find_space(s);
        if (s != end) {
            r = str_to_uint8(s);
            s = skip_space(end);
            end = find_space(s);
            if (s != end) {
                g = str_to_uint8(s);
                s = skip_space(end);
                b = str_to_uint8(s);
            }
        }
    }

    WS2812B_ShowDigit(digit, r, g, b);

    char buf[16];
    buf[0] = '0' + digit;
    buf[1] = '\r';
    buf[2] = '\n';
    buf[3] = '\0';
    USART1_SendString("Show digit: ");
    USART1_SendString(buf);
}

// led test命令：循环显示0-9
void Cmd_LedTest(void) {
    USART1_SendString("Testing digits 0-9...\r\n");

    for (uint8_t d = 0; d <= 9; d++) {
        WS2812B_ShowDigit(d, 0, 64, 0);  // 绿色
        LL_mDelay(500);
    }

    WS2812B_Clear();
    WS2812B_Update();
    USART1_SendString("Test done\r\n");
}

#endif // USE_WS2812B
