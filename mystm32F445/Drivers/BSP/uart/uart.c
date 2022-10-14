#define DEBUG_ON 1
#include "rtt_debug.h"

#include "uart1.h"
#include "uart1_rt.h"

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if((huart->Instance) == USART1)
    {
        
    }
    else if((huart->Instance) == USART2)
    {
		uart1_clear_tc_flag();
    }
}

void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart)
{
    /* 当接收buf大小为1时忽略半满中断 */
    if(1 == huart->RxXferSize) { return ; }
    
    if((huart->Instance) == USART1)
    {
        
    }
    else if((huart->Instance) == USART2)
    {
		HAL_UART2_RxHalfCpltCallback(huart);
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(NULL == huart->hdmarx)
    {
        /* 如果为中断模式，恢复接收地址指针到初始化buffer位置 */
        huart->pRxBuffPtr -= huart->RxXferSize;
    }
    
    if((huart->Instance) == USART1)
    {
        
    }
    else if((huart->Instance) == USART2)
    {
		HAL_UART2_RxCpltCallback(huart);
    }
  
    if(NULL != huart->hdmarx)
    {
        if(huart->hdmarx->Init.Mode != DMA_CIRCULAR)
        {
            while(HAL_OK != HAL_UART_Receive_DMA(huart, huart->pRxBuffPtr, huart->RxXferSize))
            {
                __HAL_UNLOCK(huart);
            }
        }
    }
    else
    {
        while(HAL_UART_Receive_IT(huart, huart->pRxBuffPtr, huart->RxXferSize))
        {
            __HAL_UNLOCK(huart);
        }
    }
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART1)
    {
//        uart1_rt_init();
//        
//        HAL_UART_2_MspInit(huart);
    }
    else if(huart->Instance == USART2)
    {
		uart1_rt_init();
		HAL_UART_2_MspInit(huart);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    __IO uint32_t tmpErr = 0x00U;
    
    tmpErr = HAL_UART_GetError(huart);
    if(HAL_UART_ERROR_NONE == tmpErr)
    {
        return ;
    }
    
    switch(tmpErr)
    {
        case HAL_UART_ERROR_PE:
            __HAL_UART_CLEAR_PEFLAG(huart);
            break;
        case HAL_UART_ERROR_NE:
            __HAL_UART_CLEAR_NEFLAG(huart);
            break;
        case HAL_UART_ERROR_FE:
            __HAL_UART_CLEAR_FEFLAG(huart);
            break;
        case HAL_UART_ERROR_ORE:
            __HAL_UART_CLEAR_OREFLAG(huart);
            break;
        case HAL_UART_ERROR_DMA:
            
            break;
        default:
            break;
    }
    
    if(NULL != huart->hdmarx)
    {
        while(HAL_UART_Receive_DMA(huart, huart->pRxBuffPtr, huart->RxXferSize))
        {
          __HAL_UNLOCK(huart);
        }
    }
    else
    {
        /* 恢复接收地址指针到初始 buffer 位置 ，初始地址 = 当前地址 - 已接收的数据个数，已接收的数据个数 = 需要接收数 - 还未接收数*/
        while(HAL_UART_Receive_IT(huart, huart->pRxBuffPtr - (huart->RxXferSize - huart->RxXferCount), huart->RxXferSize))
        {
          __HAL_UNLOCK(huart);
        }
    }
}
