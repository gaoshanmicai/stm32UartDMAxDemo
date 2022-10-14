#include "uart1_rt.h"

#define DEBUG_ON 1
#include "rtt_debug.h"

#include "ring_fifo.h"

#include "uart1.h"

#define SEND_BY_POLL    0
#define SEND_BY_IT      1
#define SEND_BY_DMA     2

/* 数据发送方式 */
#define UART_SEND_TYPE      SEND_BY_DMA
/* 单次从发送fifo读取的最大数据量(非轮询发送有效) */
#define UART_SEND_BUF_SIZE  1024

struct {
    struct ring_fifo_t *rx_fifo;
    uint8_t rx_fifo_buf[8192];
    
#if (SEND_BY_POLL != UART_SEND_TYPE)
    struct ring_fifo_t *tx_fifo;
    uint8_t tx_fifo_buf[4096];
#endif
    
    uint32_t head_ptr;
    volatile uint32_t tc_flag;
    uint32_t send_type;
}static uart_rt;

static inline void uart1_write_rx_fifo(const void *data, uint32_t len)
{
    uint32_t copied;
    if((NULL == data) || (0 == len)) { return ; }
    
    copied = ring_fifo_write(uart_rt.rx_fifo, data, len);
    if(copied != len)
    {
        //PRINTF("%s is full.\n", __FUNCTION__);
		printf("%s is full.\n", __FUNCTION__);
    }
}

uint32_t uart1_read(void *buf, uint32_t len)
{
    if((NULL == buf) || (0 == len)) { return 0; }
    
    return ring_fifo_read(uart_rt.rx_fifo, buf, len);
}

#if (SEND_BY_POLL != UART_SEND_TYPE)
void uart1_write(const void *data, uint32_t len)
{
    uint32_t copied;
    if((NULL == data) || (0 == len)) { return ; }
    
    copied = ring_fifo_write(uart_rt.tx_fifo, data, len);
    if(copied != len)
    {
        //PRINTF("%s is full.\n", __FUNCTION__);
		printf("%s is full.\n", __FUNCTION__);
    }
}

static inline uint32_t uart1_read_tx_fifo(void *buf, uint32_t len)
{
    if((NULL == buf) || (0 == len)) { return 0; }
    
    return ring_fifo_read(uart_rt.tx_fifo, buf, len);
}

void uart1_poll_send(void)
{
    uint32_t len;
    /* Notice static */
    static uint8_t buf[UART_SEND_BUF_SIZE];
    UART_HandleTypeDef *huart;
    
    huart = Uart1GetHandle();
    if(0 != uart_rt.tc_flag)
    {
        len = uart1_read_tx_fifo(buf, sizeof(buf));
        if(len > 0)
        {
            uart_rt.tc_flag = 0;
            switch(uart_rt.send_type)
            {
                case SEND_BY_IT:
                    HAL_UART_Transmit_IT(huart, buf, len);
                    break;
                case SEND_BY_DMA:
                    if(NULL == huart->hdmatx)
                    {
                        //PRINTF("uart1 not enable dma tx.\n");
						printf("uart1 not enable dma tx.\n");
                        for(;;);
                    }
                    HAL_UART_Transmit_DMA(huart, buf, len);
                    break;
            }
        }
    }
}
#else
void uart1_write(const void *data, uint32_t len)
{
    HAL_UART_Transmit(Uart1GetHandle(), (uint8_t *)data, len, 0xffff);
}

void uart1_poll_send(void)
{
    /* do nothing */
}
#endif

void uart1_clear_tc_flag(void)
{
    uart_rt.tc_flag = 1;
}

void HAL_UART2_RxIdleCallback(UART_HandleTypeDef *huart)
{
    uint32_t tail_ptr;
    uint32_t copy, offset;
    
    /*
     * +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
     * |     head_ptr          tail_ptr         |
     * |         |                 |            |
     * |         v                 v            |
     * | --------*******************----------- |
     * +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
     */
    
    /* 已接收 */
    tail_ptr = huart->RxXferSize - __HAL_DMA_GET_COUNTER(huart->hdmarx);
    
    offset = uart_rt.head_ptr % huart->RxXferSize;
    copy = tail_ptr - offset;
    uart_rt.head_ptr += copy;
    
    uart1_write_rx_fifo(huart->pRxBuffPtr + offset, copy);
}

void HAL_UART2_RxHalfCpltCallback(UART_HandleTypeDef *huart)
{
    uint32_t tail_ptr;
    uint32_t offset, copy;
    
    /*
     * +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
     * |                  half                  |
     * |     head_ptr   tail_ptr                |
     * |         |          |                   |
     * |         v          v                   |
     * | --------*******************----------- |
     * +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
     */
    
    tail_ptr = (huart->RxXferSize >> 1) + (huart->RxXferSize & 1);
    
    offset = uart_rt.head_ptr % huart->RxXferSize;
    copy = tail_ptr - offset;
    uart_rt.head_ptr += copy;
    
    uart1_write_rx_fifo(huart->pRxBuffPtr + offset, copy);
}

void HAL_UART2_RxCpltCallback(UART_HandleTypeDef *huart)
{
    uint32_t tail_ptr;
    uint32_t offset, copy;
    
    /*
     * +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
     * |                  half                  |
     * |                    | head_ptr tail_ptr |
     * |                    |    |            | |
     * |                    v    v            v |
     * | ------------------------************** |
     * +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
     */

    tail_ptr = huart->RxXferSize;
    
    offset = uart_rt.head_ptr % huart->RxXferSize;
    copy = tail_ptr - offset;
    uart_rt.head_ptr += copy;
    
    uart1_write_rx_fifo(huart->pRxBuffPtr + offset, copy);
}

void uart1_rt_init(void)
{
    uart_rt.head_ptr = 0;
    uart_rt.tc_flag = 1;
    uart_rt.send_type = UART_SEND_TYPE;
    
    uart_rt.rx_fifo = ring_fifo_init(uart_rt.rx_fifo_buf, sizeof(uart_rt.rx_fifo_buf) / sizeof(uart_rt.rx_fifo_buf[0]), RF_TYPE_STREAM);
    if(NULL == uart_rt.rx_fifo)
    {
        //PRINTF("uart1 rx fifo init failed.\n");
		printf("uart1 rx fifo init failed.\n");
        for(;;);
    }
    
#if (SEND_BY_POLL != UART_SEND_TYPE)
    uart_rt.tx_fifo = ring_fifo_init(uart_rt.tx_fifo_buf, sizeof(uart_rt.tx_fifo_buf) / sizeof(uart_rt.tx_fifo_buf[0]), RF_TYPE_STREAM);
    if(NULL == uart_rt.tx_fifo)
    {
        //PRINTF("uart1 tx fifo init failed.\n");
		printf("uart1 tx fifo init failed.\n");
        for(;;);
    }
#endif
    
    Uart1RegisterIdleIrqCallback(HAL_UART2_RxIdleCallback);
}
