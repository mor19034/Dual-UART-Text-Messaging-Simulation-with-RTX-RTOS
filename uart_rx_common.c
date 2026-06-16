/*----------------------------------------------------------------------------
 * uart_rx_common.c
 *
 * Generic per-UART receive state machine, shared by UART1/2/3_Rx_Thread.
 * Each thread keeps only its own for(;;) { wait signal; get byte; } loop and
 * calls UART_Rx_Process() with its UART_ID - everything else (echo, input
 * buffering, menu, mute toggle, fragmentation) lives here in one place.
 *---------------------------------------------------------------------------*/
#include "uart_rx_common.h"
#include "uart_dispatch.h"
#include "mail_fragment.h"
#include <stdio.h>

void UART_Context_Init(UART_ID id)
{
    uint8_t j;

    UART_ContextData[id].Emergency_Mode = false;
    for (j = 0; j < NUM_UARTS; j++) {
        UART_ContextData[id].Mute_Sender[j] = false;
    }
    UART_ContextData[id].Recipient   = Recipient_Group;
    UART_ContextData[id].UART_State  = UART_IDLE;
    UART_ContextData[id].Mute_Active = false;
    UART_ContextData[id].index       = 0;
}

void UART_Rx_Process(UART_ID id, uint8_t key)
{
    uint8_t u;
    char msg[32];

    switch (UART_ContextData[id].UART_State) {

    /*------------------------------------------------------------------*/
    case UART_IDLE:
    {
        if (key == '\r' || key == '\n') {
            UART_SendChar(id, '\r');
            UART_SendChar(id, '\n');
        }
        else if (key == '1') {
            UART_SendChar(id, key);
            Send_User_Menu(id);
            UART_ContextData[id].UART_State = UART_MENU;
        }
        else {
            UART_ContextData[id].UART_State = UART_INPUT;
            UART_SendChar(id, key);
            if (UART_ContextData[id].index < LOCAL_BUF_SIZE - 1) {
                UART_ContextData[id].local_buffer[UART_ContextData[id].index] = key;
                UART_ContextData[id].index++;
            }
        }
        break;
    }

    /*------------------------------------------------------------------*/
    case UART_INPUT:
    {
        if (key == '\r' || key == '\n') {
            UART_SendChar(id, '\r');
            UART_SendChar(id, '\n');

            /* End of message - fragment & queue whatever was accumulated */
            Fragment_And_Send(id);

            UART_ContextData[id].UART_State = UART_IDLE;
            UART_ContextData[id].index = 0; /* Clear for next message */
        }
        else {
            UART_SendChar(id, key);

            /* Accumulate normal typing into buffer, avoiding overflow */
            if (UART_ContextData[id].index < LOCAL_BUF_SIZE - 1) {
                UART_ContextData[id].local_buffer[UART_ContextData[id].index] = key;
                UART_ContextData[id].index++;
            }
        }
        break;
    }

    /*------------------------------------------------------------------*/
    case UART_MENU:
    {
        if (key == '\r' || key == '\n') {
            UART_SendChar(id, '\r');
            UART_SendChar(id, '\n');
        }

        if (key == 'g') {
            UART_ContextData[id].Recipient  = Recipient_Group;
            UART_ContextData[id].UART_State = UART_IDLE;
            UART_SendText(id, (uint8_t *)"Messaging: Group\n");
        }
        else if (key == '9') {
            UART_SendText(id, (uint8_t *)"Menu Closed, Message traffic will resume...\n");
            UART_ContextData[id].UART_State = UART_IDLE;
        }
        else if (key == 'm') {
            /* Toggle mute on/off for all other UARTs */
            UART_ContextData[id].Mute_Active = UART_ContextData[id].Mute_Active ? false : true;

            for (u = 0; u < NUM_UARTS; u++) {
                if ((UART_ID)u != id) {
                    UART_ContextData[id].Mute_Sender[u] = UART_ContextData[id].Mute_Active;
                }
            }

            if (UART_ContextData[id].Mute_Active) {
                UART_SendText(id, (uint8_t *)"Messages Muted\n");
            } else {
                UART_SendText(id, (uint8_t *)"Messages UnMuted\n");
            }

            UART_ContextData[id].UART_State = UART_IDLE;
        }
        else if (key >= '1' && key <= ('0' + NUM_UARTS) && (UART_ID)(key - '1') != id) {
            /* '1'/'2'/'3' selects one of the other UARTs as recipient */
            UART_ContextData[id].Recipient  = (Recipient_ID)(key - '1');
            UART_ContextData[id].UART_State = UART_IDLE;
            sprintf(msg, "Messaging: User %d\n", (key - '1') + 1);
            UART_SendText(id, (uint8_t *)msg);
        }
        else if (key != '\r' && key != '\n') {
            UART_SendText(id, (uint8_t *)"Invalid Entry, please select valid menu item...!\nOr press '9' to exit!\n");
        }
        break;
    }

    default:
        break;
    }
}
