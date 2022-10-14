#ifndef __UART1_H
#define __UART1_H

#include "stm32f4xx_hal.h"

/*串口初始化*/
void Uart1_Init(uint32_t baudRate);
/*串口硬件初始化*/
void HAL_UART_2_MspInit(UART_HandleTypeDef * huart);
/*注册空闲中断数据接收回调函数*/
void Uart1RegisterIdleIrqCallback(void (*Callback)(UART_HandleTypeDef *huart));
/* 获取串口配置句柄 */
UART_HandleTypeDef *Uart1GetHandle(void);

#endif/*__UART1_H*/
