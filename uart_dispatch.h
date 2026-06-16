#ifndef UART_DISPATCH_H
#define UART_DISPATCH_H

#include "comm_types.h"

/* Per-UART text senders (also used directly by Tx_Routing/Auto_Mess threads) */
void SendText1(uint8_t *text);
void SendText2(uint8_t *text);
void SendText3(uint8_t *text);

/* Generic dispatch: routes to the correct SendChar/SendText for the given UART */
void UART_SendChar(UART_ID id, uint8_t ch);
void UART_SendText(UART_ID id, uint8_t *text);

void Send_User_Menu(UART_ID Output_ID);
void Prepare_Output_Header(char *Header_Buffer, Sender_ID Sender);

#endif /* UART_DISPATCH_H */
