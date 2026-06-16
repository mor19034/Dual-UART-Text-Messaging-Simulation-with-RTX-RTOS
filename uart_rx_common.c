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
    UART_Context *ctx = &UART_ContextData[id];
    uint8_t j;

    ctx->Emergency_Mode = false;
    for (j = 0; j < NUM_UARTS; j++) {
        ctx->Mute_Sender[j] = false;
    }
    ctx->Recipient  = Recipient_Group;
    ctx->UART_State = UART_IDLE;
    ctx->Mute_Active = false;
    ctx->index = 0;
}

/* Returns the 'which' (0 or 1) UART_ID that is NOT 'self' */
static UART_ID Other_UART(UART_ID self, uint8_t which)
{
    UART_ID others[2];
    uint8_t n = 0;
    uint8_t u; //u for user 

    for (u = 0; u < NUM_UARTS; u++) {
        if ((UART_ID)u != self) {
            others[n++] = (UART_ID)u;
        }
    }
    return others[which];
}

void UART_Rx_Process(UART_ID id, uint8_t key)
{
    UART_Context *ctx = &UART_ContextData[id];

    switch (ctx->UART_State) {

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
            ctx->UART_State = UART_MENU;
        }
        else {
            ctx->UART_State = UART_INPUT;
            UART_SendChar(id, key);
            if (ctx->index < LOCAL_BUF_SIZE - 1) {
                ctx->local_buffer[ctx->index++] = key;
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

            ctx->UART_State = UART_IDLE;
            ctx->index = 0; /* Clear index tracking for the next fresh message */
        }
        else {
            UART_SendChar(id, key);

            /* Accumulate normal typing into buffer, avoiding overflow */
            if (ctx->index < LOCAL_BUF_SIZE - 1) {
                ctx->local_buffer[ctx->index++] = key;
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
            ctx->Recipient = Recipient_Group;
            ctx->UART_State = UART_IDLE;
            UART_SendText(id, (uint8_t *)"Messaging: Group\n");
        }
        else if (key == '9') {
            UART_SendText(id, (uint8_t *)"Menu Closed, Message traffic will resume...\n");
            ctx->UART_State = UART_IDLE;
        }
        else if (key == 'm') {
            UART_ID other1 = Other_UART(id, 0);
            UART_ID other2 = Other_UART(id, 1);

            ctx->Mute_Active = ctx->Mute_Active ? false : true;
            ctx->Mute_Sender[other1] = ctx->Mute_Active;
            ctx->Mute_Sender[other2] = ctx->Mute_Active;

            UART_SendText(id, ctx->Mute_Active ?
                          (uint8_t *)"Messages Muted\n" :
                          (uint8_t *)"Messages UnMuted\n");
            ctx->UART_State = UART_IDLE;
        }
        else if (key >= '1' && key <= ('0' + NUM_UARTS) && (UART_ID)(key - '1') != id) {
            /* '1'/'2'/'3' selects one of the *other* UARTs as recipient */
            UART_ID requested = (UART_ID)(key - '1');
            char msg[32];

            ctx->Recipient = (Recipient_ID)requested;
            ctx->UART_State = UART_IDLE;
            sprintf(msg, "Messaging: User %d\n", requested + 1);
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
