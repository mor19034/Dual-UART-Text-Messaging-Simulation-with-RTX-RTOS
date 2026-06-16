/*----------------------------------------------------------------------------
 * uart_dispatch.c
 *
 * Thin per-UART send wrappers plus generic UART_ID based dispatch tables,
 * so common code (uart_rx_common.c, mail_fragment.c, Tx_Routing_Thread, ...)
 * does not need to switch on UART_ID itself.
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

/* SendChar1/2/3 return int - wrap them so they match a void(uint8_t) signature */
static void SendChar1_w(uint8_t ch) { SendChar1(ch); }
static void SendChar2_w(uint8_t ch) { SendChar2(ch); }
static void SendChar3_w(uint8_t ch) { SendChar3(ch); }

typedef void (*SendChar_fn)(uint8_t);
typedef void (*SendText_fn)(uint8_t *);

static const SendChar_fn SendChar_Table[NUM_UARTS] = { SendChar1_w, SendChar2_w, SendChar3_w };
static const SendText_fn SendText_Table[NUM_UARTS] = { SendText1,   SendText2,   SendText3   };

void UART_SendChar(UART_ID id, uint8_t ch)
{
    SendChar_Table[id](ch);
}

void UART_SendText(UART_ID id, uint8_t *text)
{
    SendText_Table[id](text);
}

/*----------------------------------------------------------------------------
  Send_User_Menu - print the recipient/menu options for a given UART.
  Each UART can message the *other two* UARTs, plus group/mute/exit.
 *---------------------------------------------------------------------------*/
void Send_User_Menu(UART_ID Output_ID)
{
    static const char * const menu_text[NUM_UARTS] = {
        "\nWelcome to message menu: \n"
        "2 - Message User 2\n"
        "3 - Message User 3\n"
        "g - Message GROUP\n"
        "m - Mute messages\n",

        "\nWelcome to message menu: \n"
        "1 - Message User 1\n"
        "3 - Message User 3\n"
        "g - Message GROUP\n"
        "m - Mute messages\n",

        "\nWelcome to message menu: \n"
        "1 - Message User 1\n"
        "2 - Message User 2\n"
        "g - Message GROUP\n"
        "m - Mute messages\n"
    };

    UART_SendText(Output_ID, (uint8_t *)menu_text[Output_ID]);
}

/*----------------------------------------------------------------------------
  Prepare_Output_Header - build the "[Rec from User X:]" header for a mail item
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
