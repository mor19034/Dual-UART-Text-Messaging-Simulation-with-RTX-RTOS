/*----------------------------------------------------------------------------
 * uart_dispatch.c
 *
 * Per-UART send wrappers and generic UART_ID dispatch using simple switch
 * statements — no function pointer tables.
 *---------------------------------------------------------------------------*/
#include "uart_dispatch.h"
#include "uart.h"
#include <stdio.h>

/*----------------------------------------------------------------------------
  SendText1/2/3 - send a null-terminated string out the given UART
 *---------------------------------------------------------------------------*/
void SendText1(uint8_t *text)
{
    while (*text) {
        SendChar1(*text);
        text++;
    }
}

void SendText2(uint8_t *text)
{
    while (*text) {
        SendChar2(*text);
        text++;
    }
}

void SendText3(uint8_t *text)
{
    while (*text) {
        SendChar3(*text);
        text++;
    }
}

/*----------------------------------------------------------------------------
  UART_SendChar - send one byte to the UART identified by id
 *---------------------------------------------------------------------------*/
void UART_SendChar(UART_ID id, uint8_t ch)
{
    switch (id) {
        case UART_1: SendChar1(ch); break;
        case UART_2: SendChar2(ch); break;
        case UART_3: SendChar3(ch); break;
        default: break;
    }
}

/*----------------------------------------------------------------------------
  UART_SendText - send a null-terminated string to the UART identified by id
 *---------------------------------------------------------------------------*/
void UART_SendText(UART_ID id, uint8_t *text)
{
    switch (id) {
        case UART_1: SendText1(text); break;
        case UART_2: SendText2(text); break;
        case UART_3: SendText3(text); break;
        default: break;
    }
}

/*----------------------------------------------------------------------------
  Send_User_Menu - print the recipient/menu options for a given UART
 *---------------------------------------------------------------------------*/
void Send_User_Menu(UART_ID Output_ID)
{
    switch (Output_ID) {
        case UART_1:
            SendText1((uint8_t *)"\nWelcome to message menu: \n"
                                 "2 - Message User 2\n"
                                 "3 - Message User 3\n"
                                 "g - Message GROUP\n"
                                 "m - Mute messages\n");
            break;
        case UART_2:
            SendText2((uint8_t *)"\nWelcome to message menu: \n"
                                 "1 - Message User 1\n"
                                 "3 - Message User 3\n"
                                 "g - Message GROUP\n"
                                 "m - Mute messages\n");
            break;
        case UART_3:
            SendText3((uint8_t *)"\nWelcome to message menu: \n"
                                 "1 - Message User 1\n"
                                 "2 - Message User 2\n"
                                 "g - Message GROUP\n"
                                 "m - Mute messages\n");
            break;
        default:
            break;
    }
}

/*----------------------------------------------------------------------------
  Prepare_Output_Header - build the "[Rec from User X:]" header string
 *---------------------------------------------------------------------------*/
void Prepare_Output_Header(char *Header_Buffer, Sender_ID Sender)
{
    switch (Sender) {
        case Sender_1:    sprintf(Header_Buffer, "\n[Rec from User 1:] \n"); break;
        case Sender_2:    sprintf(Header_Buffer, "\n[Rec from User 2:] \n"); break;
        case Sender_3:    sprintf(Header_Buffer, "\n[Rec from User 3:] \n"); break;
        case Sender_Auto: sprintf(Header_Buffer, "\n[Rec from Auto:] \n");   break;
        default: break;
    }
}
