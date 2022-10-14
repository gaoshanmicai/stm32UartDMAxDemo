#ifndef __RTT_DEBUG_H
#define __RTT_DEBUG_H

/***************************************************************/

#ifndef DEBUG_ON
#define DEBUG_ON PRINTF_DEBUG
#endif
#include "stdio.h"
/*用于总的调试信息输出控制开关*/
#define PRINTF_DEBUG    0

#if (PRINTF_DEBUG && DEBUG_ON)
    #include "SEGGER_RTT.h"

    #define PRINTF_Dbg(fmt, ...)                                                        \
    do {                                                                                \
        SEGGER_RTT_printf(0,"file:%s,line:%d," fmt,__FILE__,__LINE__,##__VA_ARGS__);    \
    }while(0)

    #define PRINTF(...)                                                                 \
    do {                                                                                \
        SEGGER_RTT_printf(0,__VA_ARGS__);                                               \
    }while(0)
#else
    #define PRINTF_Dbg(fmt, ...)
    #define PRINTF(...) 
#endif
/***************************************************************/

#endif/*__RTT_DEBUG_H*/
