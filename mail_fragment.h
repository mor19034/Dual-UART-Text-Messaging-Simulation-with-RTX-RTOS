#ifndef MAIL_FRAGMENT_H
#define MAIL_FRAGMENT_H

#include "comm_types.h" //for using structs 

/*----------------------------------------------------------------------------
 * Fragment_And_Send
 *
 * Takes whatever has been accumulated in UART_ContextData[sender_id].local_buffer
 * (length = .index), splits it into <= CHUNK_SIZE pieces (breaking on spaces
 * where possible), wraps each piece in a Mail block tagged with the sender's
 * current Recipient, and posts it to mail_queue_id.
 *---------------------------------------------------------------------------*/
void Fragment_And_Send(UART_ID sender_id);

#endif /* MAIL_FRAGMENT_H */