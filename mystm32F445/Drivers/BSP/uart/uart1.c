#include "uart1.h"

#include "rtt_debug.h"
#include "stdio.h"

/*是否使能DMA接收*/
#define UART_USE_DMA_RX 1
/*是否使能DMA发送*/
#define UART_USE_DMA_TX 1

#if UART_USE_DMA_RX
    /*是否使能空闲中断*/
    #define UART_USE_IDLE_IT 1
#endif

/*配置接收缓冲区的大小*/
#define UART_BUF_SIZE 256

#define RCC_UART_CLK_ENABLE() do {              \
    __HAL_RCC_USART2_CLK_ENABLE();              \
    if(UART_USE_DMA_RX || UART_USE_DMA_TX)      \
    {                                           \
        __HAL_RCC_DMA1_CLK_ENABLE();            \
    }                                           \
    __HAL_RCC_GPIOA_CLK_ENABLE();               \
}while(0)

#define UART_INSTANCE                  USART2
#define UART_IRQn                      USART2_IRQn
#define USART_IRQHandler               USART2_IRQHandler

#define UART_TX_PIN                    GPIO_PIN_2
#define UART_TX_GPIO_PORT              GPIOA
#define UART_RX_PIN                    GPIO_PIN_3
#define UART_RX_GPIO_PORT              GPIOA

#if (UART_USE_DMA_RX > 0U)
#define UART_RX_DMA_INST               DMA1_Stream5
#define UART_RX_DMA_CHANNEL            DMA_CHANNEL_4

#define UART_DMA_RX_IRQn               DMA1_Stream5_IRQn
#define UART_DMA_RX_IRQHandler         DMA1_Stream5_IRQHandler  //DMA1_Stream5_IRQHandler
#endif

#if (UART_USE_DMA_TX > 0U)
#define UART_TX_DMA_INST              DMA1_Stream6
#define UART_TX_DMA_CHANNEL            DMA_CHANNEL_4

#define UART_DMA_TX_IRQn               DMA1_Stream6_IRQn
#define UART_DMA_TX_IRQHandler         DMA1_Stream6_IRQHandler
#endif

/*========================================================================================*/

static UART_HandleTypeDef UartHandle;

#if (UART_USE_DMA_RX > 0U)
static DMA_HandleTypeDef hdma_rx;
#endif

#if (UART_USE_DMA_TX > 0U)
static DMA_HandleTypeDef hdma_tx;
#endif

/*串口接收缓冲*/
static uint8_t recv_buf[UART_BUF_SIZE];

static void *IdleIrqCallback;

void Uart1_Init(uint32_t baudRate)
{
    //asynchronous
    UartHandle.Instance             = UART_INSTANCE;

    UartHandle.Init.BaudRate        = baudRate;
    UartHandle.Init.HwFlowCtl       = UART_HWCONTROL_NONE;
    UartHandle.Init.Mode            = UART_MODE_TX_RX;
    UartHandle.Init.OverSampling    = UART_OVERSAMPLING_16;
    UartHandle.Init.Parity          = UART_PARITY_NONE;
    UartHandle.Init.StopBits        = UART_STOPBITS_1;
    UartHandle.Init.WordLength      = UART_WORDLENGTH_8B;

    if(HAL_UART_Init(&UartHandle) != HAL_OK)
    {
        //PRINTF_Dbg("uart init failed..\n");
		printf("uart init failed..\n");
    }

    /*触发接收*/
    if(UartHandle.hdmarx)
    {
        HAL_UART_Receive_DMA(&UartHandle, recv_buf, sizeof(recv_buf));
		printf("uart enable\n");
    }
    else
    {
        HAL_UART_Receive_IT(&UartHandle, recv_buf, sizeof(recv_buf));
		printf("uart ID enable\n");
    }
}
//HAL_UART_MspInit
void HAL_UART_2_MspInit(UART_HandleTypeDef * huart)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    RCC_UART_CLK_ENABLE();
	
    GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

//    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
//    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
//    GPIO_InitStruct.Pin       = UART_TX_PIN;
//    HAL_GPIO_Init(UART_TX_GPIO_PORT, &GPIO_InitStruct);

//    GPIO_InitStruct.Pull      = GPIO_PULLUP;
//    GPIO_InitStruct.Mode      = GPIO_MODE_INPUT;
//    GPIO_InitStruct.Pin       = UART_RX_PIN;
//    HAL_GPIO_Init(UART_RX_GPIO_PORT, &GPIO_InitStruct);

#if(UART_USE_DMA_RX > 0U)
    hdma_rx.Instance                  = UART_RX_DMA_INST;
	hdma_rx.Init.Channel              = DMA_CHANNEL_4;
    hdma_rx.Init.Direction            = DMA_PERIPH_TO_MEMORY;
    hdma_rx.Init.MemDataAlignment     = DMA_MDATAALIGN_BYTE;
    hdma_rx.Init.MemInc               = DMA_MINC_ENABLE;
    hdma_rx.Init.Mode                 = DMA_CIRCULAR;
    hdma_rx.Init.PeriphDataAlignment  = DMA_PDATAALIGN_BYTE;
    hdma_rx.Init.PeriphInc            = DMA_PINC_DISABLE;
    hdma_rx.Init.Priority             = DMA_PRIORITY_HIGH;
   // hdma_rx.Init.FIFOMode             = DMA_FIFOMODE_DISABLE;

    HAL_DMA_Init(&hdma_rx);
    __HAL_LINKDMA(huart, hdmarx, hdma_rx);

    HAL_NVIC_SetPriority(UART_DMA_RX_IRQn, 0x06, 0);
    HAL_NVIC_EnableIRQ(UART_DMA_RX_IRQn);
#endif
  
#if(UART_USE_DMA_TX > 0U)
    hdma_tx.Instance                  = UART_TX_DMA_INST;
    hdma_tx.Init.Channel              = DMA_CHANNEL_4;
    hdma_tx.Init.Direction            = DMA_MEMORY_TO_PERIPH;
    hdma_tx.Init.MemDataAlignment     = DMA_MDATAALIGN_BYTE;
    hdma_tx.Init.MemInc               = DMA_MINC_ENABLE;
    hdma_tx.Init.Mode                 = DMA_NORMAL;
    hdma_tx.Init.PeriphDataAlignment  = DMA_PDATAALIGN_BYTE;
    hdma_tx.Init.PeriphInc            = DMA_PINC_DISABLE;
    hdma_tx.Init.Priority             = DMA_PRIORITY_LOW;
    hdma_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

    HAL_DMA_Init(&hdma_tx);
    __HAL_LINKDMA(huart, hdmatx, hdma_tx);

    HAL_NVIC_SetPriority(UART_DMA_TX_IRQn, 0x06, 0);
    HAL_NVIC_EnableIRQ(UART_DMA_TX_IRQn);
#endif

    HAL_NVIC_SetPriority(UART_IRQn, 0x06, 0);
    HAL_NVIC_EnableIRQ(UART_IRQn);

#if UART_USE_IDLE_IT
    __HAL_UART_ENABLE_IT(huart, UART_IT_IDLE);
#endif
}

#if (UART_USE_DMA_RX > 0U)
void UART_DMA_RX_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_rx);
}
#endif

#if (UART_USE_DMA_TX > 0U)
void UART_DMA_TX_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_tx);
}
#endif

void USART_IRQHandler(void)
{
    #if UART_USE_IDLE_IT
    
    if(__HAL_UART_GET_FLAG(&UartHandle, UART_FLAG_IDLE))
    {
        __HAL_UART_CLEAR_IDLEFLAG(&UartHandle);
        
        if(IdleIrqCallback)
        {
            ((void (*)(UART_HandleTypeDef *huart))IdleIrqCallback)(&UartHandle);
        }
    }
    #else
    IdleIrqCallback = IdleIrqCallback;
    #endif

    HAL_UART_IRQHandler(&UartHandle);
}

void Uart1RegisterIdleIrqCallback(void (*Callback)(UART_HandleTypeDef *huart))
{
    IdleIrqCallback = Callback;
}

UART_HandleTypeDef *Uart1GetHandle(void)
{
    return &UartHandle;
}
