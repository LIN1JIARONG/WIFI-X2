#ifndef _UART_H_
#define _UART_H_

#include <stdio.h>

void IOT_UART1_Init(void);
uint16_t IOT_UART1SendData(uint8_t *uSendDATABuf, uint16_t uSendLen );


#endif


