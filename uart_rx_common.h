#ifndef UART_RX_COMMON_H
#define UART_RX_COMMON_H

#include "comm_types.h"

/*----------------------------------------------------------------------------
 * UART_Context_Init
 *
 * Resets UART_ContextData[id] to its default startup state. Call once at the
 * top of each UART's Rx thread, before entering its for(;;) loop.
 *---------------------------------------------------------------------------*/
void UART_Context_Init(UART_ID id);

/*----------------------------------------------------------------------------
 * UART_Rx_Process
 *
 * Runs one state-machine step for UART 'id' given the next received byte
 * 'key'. Handles the IDLE / INPUT / MENU states, local echo, message
 * accumulation, menu navigation and mute toggling. When a full line has been
 * entered it calls Fragment_And_Send() to push it onto the mail queue.
 *
 * This is the body that used to be duplicated across
 * UART1/2/3_Rx_Thread's for(;;) loops.
 *---------------------------------------------------------------------------*/
void UART_Rx_Process(UART_ID id, uint8_t key);

#endif /* UART_RX_COMMON_H */
