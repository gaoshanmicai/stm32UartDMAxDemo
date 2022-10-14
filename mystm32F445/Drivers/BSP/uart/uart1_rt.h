#ifndef __UART1_RT_H
#define __UART1_RT_H

#include "stm32f4xx_hal.h"

/* 从接收fifo读取数据 */
uint32_t uart1_read(void *buf, uint32_t len);
/* 写数据到发送fifo(轮询发送时,不写入fifo,直接发送) */
void uart1_write(const void *data, uint32_t len);
/* 轮询从发送fifo取出数据并发送(轮询发送时为空) */
void uart1_poll_send(void);
/* 清除发送完成标志(非轮询发送有效) */
void uart1_clear_tc_flag(void);
/* 接收空闲回调处理 */
void HAL_UART2_RxIdleCallback(UART_HandleTypeDef *huart);
/* 接收过半回调处理 */
void HAL_UART2_RxHalfCpltCallback(UART_HandleTypeDef *huart);
/* 接收满回调处理 */
void HAL_UART2_RxCpltCallback(UART_HandleTypeDef *huart);
/* 初始化上层收发配置 */
void uart1_rt_init(void);

#endif/*__UART1_RT_H*/
