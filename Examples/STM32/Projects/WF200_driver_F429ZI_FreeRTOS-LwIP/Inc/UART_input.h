#ifndef UART_INPUT_H
#define UART_INPUT_H

void vUARTInputStart( void );
extern SemaphoreHandle_t stringRcvSemaphore;
extern SemaphoreHandle_t uartInputSemaphore;
extern char UART_Input_String[ ];
void vUARTInputStop (void);
#endif