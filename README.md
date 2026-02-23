# STM32F103C8T6 LL库串口HelloWorld工程

这是一个基于STM32F103C8T6最小系统的串口HelloWorld工程，使用STM32官方LL库（Low Layer Library）进行开发，集成了UC1701驱动的LCD12864显示屏控制功能。

## 项目特性

- ✅ **官方LL库** - 使用ST官方STM32CubeF1 LL库
- ✅ **外部晶振** - 配置为使用外部8MHz HSE晶振
- ✅ **高速串口** - USART1配置为115200波特率，支持中断接收
- ✅ **LED指示** - PB12引脚控制LED闪烁
- ✅ **LCD显示** - UC1701 LCD12864显示屏驱动，支持光标闪烁
- ✅ **命令系统** - 支持串口命令交互
- ✅ **标准构建** - 完整的Makefile构建系统

## 硬件要求

- STM32F103C8T6最小系统板
- 外部8MHz晶振
- USB转串口模块（连接PA9-TX, PA10-RX）
- 3.3V电源
- LED连接至PB12引脚（可选）
- UC1701驱动的LCD12864显示屏

## 引脚连接

### 基本引脚连接
| STM32引脚 | 功能 | 连接 |
|----------|------|------|
| PA9      | USART1_TX | USB转串口RX |
| PA10     | USART1_RX | USB转串口TX |
| PB12     | LED控制 | LED阳极（通过限流电阻） |
| 3.3V     | 电源 | 3.3V |
| GND      | 地   | GND |

### LCD12864引脚连接
| STM32引脚 | LCD引脚 | 功能 |
|----------|---------|------|
| PB0      | CS      | 片选信号 |
| PB1      | RST     | 复位信号 |
| PB10     | RS      | 数据/命令选择 |
| PB13     | SCK     | SPI时钟 |
| PB15     | SDA     | SPI数据 |
| 3.3V     | VCC     | LCD电源 |
| GND      | GND     | 地线 |

## 技术规格

### 时钟配置
- **晶振源**: 外部8MHz HSE
- **系统时钟**: 72MHz（PLL倍频）
- **AHB时钟**: 72MHz
- **APB1时钟**: 36MHz
- **APB2时钟**: 72MHz（USART1使用）

### 串口配置
- **外设**: USART1
- **波特率**: 115200
- **数据位**: 8位
- **停止位**: 1位
- **校验位**: 无
- **流控制**: 无
- **接收模式**: 中断方式

### LED配置
- **引脚**: PB12
- **模式**: 推挽输出
- **初始状态**: 关闭
- **功能**: 500ms间隔闪烁

## 工程结构

```
stm32f103_prj/
├── main.c                 # 主程序文件（使用LL库）
├── lcd_uc1701.h           # LCD驱动头文件
├── lcd_uc1701.c           # LCD驱动实现文件
├── ll_config.h            # LL库配置头文件
├── startup_stm32f10x_md.s # 启动文件（完整中断向量表）
├── stm32f103c8t6.ld       # 链接脚本
├── Makefile               # 构建配置
├── README.md              # 说明文档
└── STM32CubeF1/           # ST官方库文件
```

## 编译说明

### 工具链要求
- arm-none-eabi-gcc
- make
- stlink工具（用于烧录）

### 编译命令
```bash
# 编译工程
make

# 清理编译文件
make clean

# 烧录到芯片（需要连接ST-Link）
make flash

# 查看程序大小
make size

# 生成反汇编文件
make debug
```

## 串口命令

通过串口发送命令（以回车结束）：

| 命令 | 说明 |
|------|------|
| `date` | 显示当前时间，LCD显示时分秒 |
| `date -d` | 显示当前时间，LCD显示年月日 |
| `date -s YYYY-MM-DD HH:MM:SS` | 设置时间，例如: `date -s 2024-02-23 14:30:00` |
| `clear` | 清除LCD屏幕 |
| `ls` | 列出所有可用命令 |

### 时间设置说明
- 时间基准从 `2000-01-01 00:00:00` 开始
- 使用 `date -s` 命令设置时间后，系统会保持时间偏移
- 掉电后时间会重置（无RTC硬件）

## 串口调试

使用串口调试工具连接：
- **波特率**: 115200
- **数据位**: 8
- **停止位**: 1
- **校验位**: 无
- **流控制**: 无

### 预期输出
```
STM32F103C8T6 UC1701 LCD12864 Demo Started!
Clock: External 8MHz HSE (Real LL Library)
Baudrate: 115200
LCD on PB0,1,10,13,15
LED on PB12 will blink every 500ms
UART Rx enabled (interrupt mode, buffer size: 128)
Available commands: date, clear, ls
> ls
Available commands:
  date
  clear
  ls
> date
Sat Feb 23 14:30:45 2024
> date -s 2024-02-23 15:00:00
Time set
```

## LCD12864显示功能

### 基本函数

```c
// LCD初始化
LCD_Init();

// 清屏
LCD_Clear();

// 显示字符串
LCD_WriteString("Hello World!");

// 绘制像素点
LCD_DrawPixel(x, y, color);  // color: 0=关闭, 1=开启

// 光标控制
LCD_Cursor_Enable();    // 使能光标
LCD_Cursor_Disable();   // 禁用光标
LCD_Cursor_Blink();     // 光标闪烁（在主循环调用）
```

### 显示效果

程序启动后，LCD将显示：
```
:) Welcome!
```

## LL库使用说明

### 包含的头文件
```c
#include "ll_config.h"  // 自动包含所有必要的LL库头文件
```

### 主要LL库函数
- **时钟控制**: `LL_RCC_HSE_Enable()`, `LL_RCC_GetSystemClocksFreq()`
- **GPIO控制**: `LL_GPIO_SetPinMode()`, `LL_GPIO_TogglePin()`
- **USART控制**: `LL_USART_SetBaudRate()`, `LL_USART_TransmitData8()`, `LL_USART_ReceiveData8()`
- **总线时钟**: `LL_APB2_GRP1_EnableClock()`
- **中断控制**: `NVIC_EnableIRQ()`, `NVIC_SetPriority()`

## 项目特点

### 使用官方LL库的优势
1. **标准化** - 符合ST官方开发规范
2. **可移植性** - 易于在不同STM32型号间移植
3. **维护性** - 官方维护，bug修复及时
4. **文档支持** - 完整的官方文档和示例

### 构建系统特点
- 自动包含所有必要的LL库源文件
- 正确的头文件包含路径设置
- 标准的目标文件链接处理
- 支持调试信息生成

## 注意事项

1. **晶振确认** - 确保外部8MHz晶振正常工作
2. **电平匹配** - 串口连接时注意3.3V电平匹配
3. **烧录工具** - 使用ST-Link或其他兼容烧录器
4. **库文件** - 项目包含完整的STM32CubeF1库，无需额外下载
5. **中断向量** - 启动文件包含完整的外设中断向量表

## LCD12864技术特点

1. **低功耗设计** - 使用STM32 LL库，代码效率高
2. **硬件SPI模拟** - 无需硬件SPI外设，通用性强
3. **光标闪烁** - 支持输入光标闪烁显示
4. **可扩展性** - 易于添加图形绘制功能
5. **调试支持** - 通过串口输出状态信息

## 自定义配置

### 修改引脚定义
在 `lcd_uc1701.h` 中修改引脚定义：
```c
#define LCD_CS_PIN      LL_GPIO_PIN_X   // 修改X为实际引脚号
#define LCD_RST_PIN     LL_GPIO_PIN_Y
// ... 其他引脚
```

### 调整对比度
在 `LCD_Init()` 函数中修改对比度值：
```c
LCD_WriteCommand(LCD_CMD_SET_VOLUME_FIRST);
LCD_WriteCommand(0x20);  // 修改0x20为其他值（0x00-0x3F）
```

## 故障排除

### LCD无显示
1. 检查硬件连接是否正确
2. 确认电源电压（3.3V）
3. 检查背光是否正常
4. 使用示波器检查SPI信号

### 显示乱码
1. 检查初始化序列是否正确
2. 确认对比度设置是否合适
3. 检查数据/命令选择信号

### 串口接收无反应
1. 确认PA10配置为输入模式并上拉
2. 检查中断向量表是否包含USART1_IRQHandler
3. 确认USART接收中断已使能

### 编译错误
1. 确认STM32CubeF1库路径正确
2. 检查工具链安装是否完整
3. 确认Makefile配置正确

## 资源参考

- [STM32CubeF1官方文档](https://www.st.com/en/embedded-software/stm32cubef1.html)
- [STM32F103xx参考手册](https://www.st.com/resource/en/reference_manual/rm0008-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [UC1701数据手册](https://www.displayfuture.com/Display/datasheet/controller/UC1701.pdf)

---

**基于STM32CubeF1 LL库开发 - 集成UC1701 LCD12864驱动和串口命令系统**
