/*----------------------------------------------------------------------------
 * mail_fragment.c
 *
 * Shared fragmentation/mail-queueing logic used by all three UART Rx threads.
 *---------------------------------------------------------------------------*/
#include "mail_fragment.h"
#include "uart_dispatch.h"

void Fragment_And_Send(UART_ID sender_id)
{
	//All of the neccesary local variables for the process 
	uint16_t remaining_bytes = UART_ContextData[sender_id].index; //on the original function we used the index, here we assigned a variable for it
  uint16_t buffer_ptr  = 0;
  uint16_t chunk_index = 0;
  uint16_t bytes_to_copy;
  uint16_t i;
  uint8_t  fragmented_flag;
  Mail *mail;
	// 1.User pressed Enter / Carriage Return
	UART_ContextData[sender_id].local_buffer[remaining_bytes] = '\0'; // we force a Null to terminate the string
	
  if (remaining_bytes == 0) {
		return; /* Nothing to send */
	}	
	
	// set a flag if the message is bigger than 32 characters 
	fragmented_flag = (remaining_bytes > CHUNK_SIZE) ? 1 : 0;
				
	// Alert the user on their local screen that fragmentation is happening
	if (fragmented_flag){
		UART_SendText(sender_id, (uint8_t *)"\r\n[System: Message too long. Segmenting into packets...]\r\n");
	}
	
	// ---------------------- FRAGMENTATION LOOP -------------------------------------------
	while (remaining_bytes > 0){
		// Allocate RTX Mail, populate it with data and send it 
		Mail *mail;
		mail = (Mail*)osMailAlloc(mail_queue_id, osWaitForever);
		if (mail == NULL){
			// Alloc failed: bail out so we never spin forever and starve MailOutput
			break;
		}
		
		//  allocates a mail slot and fill it with data
		mail->Sender = (Sender_ID)sender_id; // Tell from which UART it was Originating 
		// 0 = not fragmented, 1 = first chunk of a fragmented message, 2 = continuation chunk
		mail->is_fragmented = (!fragmented_flag) ? 0 : ((chunk_index == 0) ? 1 : 2);
		mail->Receiver = 	UART_ContextData[sender_id].Recipient;

		// Calculate how many characters fit in this specific mail slice.
		// If we'd split mid-word, back up to the last space so whole words
		// stay together on each line.
		if (remaining_bytes <= CHUNK_SIZE){
			bytes_to_copy = remaining_bytes;
		} else {
			// Search backwards from CHUNK_SIZE for the last space
			bytes_to_copy = CHUNK_SIZE; // Fallback: hard split (no space found)
			for (i = CHUNK_SIZE; i > 0; i--) {
				if (UART_ContextData[sender_id].local_buffer[buffer_ptr + i - 1] == ' ') {
					bytes_to_copy = i - 1; // Copy up to (not including) the space
					break;
				}
			}
		}
		
		//Fragmentate the message and copy chunk into mail payload 
		for (i = 0; i < bytes_to_copy; i++){
			mail->payload[i] = UART_ContextData[sender_id].local_buffer[buffer_ptr + i];
		}
		
		mail->payload[bytes_to_copy] = '\0'; // Explicitly force null-termination 
		
		// Ship it to the Mail Queue
		osMailPut(mail_queue_id, mail);
		
		// Shift tracking pointers forward past the copied bytes
		buffer_ptr += bytes_to_copy;
		remaining_bytes -= bytes_to_copy;
		chunk_index++;
		
		// Skip the space we split on so the next line has no leading space
		if (remaining_bytes > 0 && UART_ContextData[sender_id].local_buffer[buffer_ptr] == ' ') {
			buffer_ptr++;
			remaining_bytes--;
		}
	}
}
