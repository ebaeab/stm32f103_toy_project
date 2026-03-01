// LL库配置头文件
#ifndef LL_CONFIG_H
#define LL_CONFIG_H

// 注意：USE_LCD_UC1701 和 USE_WS2812B 宏在 Makefile 中定义
// 不需要在此处重复定义

// 只需要包含stm32f1xx.h，它会自动包含所有必要的文件
// 注意：设备型号和外设宏定义已经在Makefile的CFLAGS中设置
#include "STM32CubeF1/Drivers/CMSIS/Device/ST/STM32F1xx/Include/stm32f1xx.h"

// 包含真正的STM32 LL库头文件
#include "STM32CubeF1/Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_ll_rcc.h"
#include "STM32CubeF1/Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_ll_gpio.h"
#include "STM32CubeF1/Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_ll_usart.h"
#include "STM32CubeF1/Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_ll_bus.h"
#include "STM32CubeF1/Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_ll_utils.h"
#include "STM32CubeF1/Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_ll_system.h"
#include "STM32CubeF1/Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_ll_cortex.h"

#endif // LL_CONFIG_H
