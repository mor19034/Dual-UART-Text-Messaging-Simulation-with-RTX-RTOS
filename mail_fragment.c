/*----------------------------------------------------------------------------
 * mail_fragment.c
 *
 * Shared fragmentation/mail-queueing logic used by all three UART Rx threads.
 *---------------------------------------------------------------------------*/
#include "mail_fragment.h"
#include "uart_dispatch.h"

void Fragment_And_Send(UART_ID sender_id)
{
    UART_Context *ctx = &UART_ContextData[sender_id];
    char *local_buffer = ctx->local_buffer;
    uint16_t remaining_bytes = ctx->index;
    uint16_t buffer_ptr = 0;
    uint16_t chunk_index = 0;
    uint16_t i;
    uint8_t fragmented_flag;

    local_buffer[remaining_bytes] = '\0'; /* Null terminate what the user typed */

    if (remaining_bytes == 0) {
        return; /* Nothing to send */
    }

    /* Flag set if the message is bigger than what fits in one mail payload */
    fragmented_flag = (remaining_bytes > CHUNK_SIZE) ? 1 : 0;

    if (fragmented_flag) {
        UART_SendText(sender_id, (uint8_t *)"\r\n[System: Message too long. Segmenting into packets...]\r\n");
    }

    /* --- FRAGMENTATION LOOP --- */
    while (remaining_bytes > 0) {
        Mail *mail = (Mail *)osMailAlloc(mail_queue_id, osWaitForever);
        uint16_t bytes_to_copy;

        if (mail == NULL) {
            /* Alloc failed: bail out so we never spin forever and starve MailOutput */
            break;
        }

        mail->Sender = (Sender_ID)sender_id;
        /* 0 = not fragmented, 1 = first chunk of a fragmented message, 2 = continuation chunk */
        mail->is_fragmented = (!fragmented_flag) ? 0 : ((chunk_index == 0) ? 1 : 2);
        mail->Receiver = ctx->Recipient;

        /* Calculate how many characters fit in this specific mail slice.
           If we'd split mid-word, back up to the last space so whole words
           stay together on each line. */
        if (remaining_bytes <= CHUNK_SIZE) {
            bytes_to_copy = remaining_bytes;
        } else {
            bytes_to_copy = CHUNK_SIZE; /* Fallback: hard split (no space found) */
            for (i = CHUNK_SIZE; i > 0; i--) {
                if (local_buffer[buffer_ptr + i - 1] == ' ') {
                    bytes_to_copy = i - 1; /* Copy up to (not including) the space */
                    break;
                }
            }
        }

        /* Copy this chunk into the mail payload */
        for (i = 0; i < bytes_to_copy; i++) {
            mail->payload[i] = local_buffer[buffer_ptr + i];
        }
        mail->payload[bytes_to_copy] = '\0'; /* Explicitly force null-termination */

        /* Ship it to the Mail Queue */
        osMailPut(mail_queue_id, mail);

        /* Shift tracking pointers forward past the copied bytes */
        buffer_ptr += bytes_to_copy;
        remaining_bytes -= bytes_to_copy;
        chunk_index++;

        /* Skip the space we split on so the next line has no leading space */
        if (remaining_bytes > 0 && local_buffer[buffer_ptr] == ' ') {
            buffer_ptr++;
            remaining_bytes--;
        }
    }
}
