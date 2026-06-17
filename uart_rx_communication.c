#include "uart_rx_communication.h"
#include "uart_dispatch.h"
#include "mail_fragment.h"
#include <stdio.h>

// This part of the code is to handle the state into which 
// The uart is currently in, if there are mutes, or if it a group chat
void UART_Context_Init(UART_ID id)
{
	uint8_t j;
	
	//silence all of the UARTS (even sender)
	UART_ContextData[id].Emergency_Mode = false;
	for (j = 0; j <NUM_UARTS; j++){
		UART_ContextData[id].Mute_Sender[j] = false;
	}
	

	UART_ContextData[id].Recipient = Recipient_Group;
	UART_ContextData[id].UART_State = UART_IDLE;
	UART_ContextData[id].index  = 0;
	UART_ContextData[id].Mute_Active = false;
	//UART_ContextData[id].cur_Mute_Flag = false;
	//UART_ContextData[id].prev_Mute_Flag = true;
}

void UART_Rx_Process(UART_ID id, uint8_t intkey){
	uint8_t u;
	char message[32];
	
	switch(UART_ContextData[id].UART_State){
		
		/*IDLE STATE - we are doing */
		case(UART_IDLE):
			{
			if (intkey == '\r' || intkey == '\n') {
				UART_SendChar(id, '\r');
				UART_SendChar(id, '\n');
			}
			else if(intkey == '1'){
				UART_SendChar(id, intkey);
				Send_User_Menu(id);
				UART_ContextData[id].UART_State = UART_MENU;								 
			}
			
			else{
				UART_ContextData[id].UART_State = UART_INPUT;
				UART_SendChar(id, intkey);
				//Now we are using the UART_Context structure, there the index exist for each UART
				if(UART_ContextData[id].index < LOCAL_BUF_SIZE - 1){
					UART_ContextData[id].local_buffer[UART_ContextData[id].index] = intkey; //we store the input on the local buffer
					UART_ContextData[id].index++; // we move to the next element of the buffer
				}	
			}
			break;
			}
			/*---------------------------------INPUT STATE--------------------------------- 
			here each input is stored on the local buffer and then 
			broken down into chunk sizes of 32 bytes once user hits enter 
			*/
			case(UART_INPUT):
				{
					// 1. Check for End of Message (User pressed Enter / Carriage Return)
					if (intkey == '\r' || intkey == '\n') 
						{
							UART_SendChar(id, '\r');
							UART_SendChar(id, '\n');
							
						/* 2. Once the user hits enter we enter the fragmentation loop*/
						Fragment_And_Send(id);	

            UART_ContextData[id].UART_State = UART_IDLE; //Go back to IDLE 
            UART_ContextData[id].index = 0; /* Clear for next message */		
						}
						else{
							UART_SendChar(id, intkey); // echo the character back to the terminal
							// 3. Accumulate normal typing into buffer safely avoiding memory overflow
							if(UART_ContextData[id].index < LOCAL_BUF_SIZE - 1) {
								UART_ContextData[id].local_buffer[UART_ContextData[id].index] = intkey;
								UART_ContextData[id].index++;
							}
						}
				break;
				}
				case(UART_MENU): //instead of switch cases we have different if conditions for the menu inputs
					{
						if (intkey == '\r' || intkey == '\n') 
							{
								UART_SendChar(id, '\r');
			          UART_SendChar(id, '\n');
				      }
							if (intkey == 'g')
								{
									UART_ContextData[id].Recipient = Recipient_Group;
									UART_ContextData[id].UART_State = UART_IDLE;	
									UART_SendText(id, (uint8_t *)"Messaging: Group\n"); //parcing message
							}
							else if (intkey == '9')
								{
									UART_SendText(id, (uint8_t *)"Menu Closed, Message traffic will resume...\n");
								  UART_ContextData[id].UART_State = UART_IDLE;
							}
							else if (intkey == 'm')
								{
									/* Toggle mute on/off for all other UARTs */
									UART_ContextData[id].Mute_Active = UART_ContextData[id].Mute_Active ? false : true;
									
								for (u = 0; u < NUM_UARTS; u++) {
									if ((UART_ID)u != id) {
										UART_ContextData[id].Mute_Sender[u] = UART_ContextData[id].Mute_Active;
									}
								}
								
								if (UART_ContextData[id].Mute_Active) { //checks if mute is true, if it's then prints a message
									UART_SendText(id, (uint8_t *)"Messages Muted\n");
								} else {
									UART_SendText(id, (uint8_t *)"Messages UnMuted\n");
								}
								UART_ContextData[id].UART_State = UART_IDLE; //goes back to idle state
								}
								//Firs we check if the input is 1, 2 or 3 
								// (UART_ID)(intkey - '1') Converts the key character to a UART index ('1'->0, '2'->1, '3'->2)
								else if(intkey >= '1' && intkey <= ('0' + NUM_UARTS) && (UART_ID)(intkey - '1') != id){ //at last we check input if different from id number (current UART)
									/* '1'/'2'/'3' selects one of the other UARTs as recipient */
									UART_ContextData[id].Recipient  = (Recipient_ID)(intkey - '1'); //This handles the logic of who is recivieng the message and holds it 
									UART_ContextData[id].UART_State = UART_IDLE;
									sprintf(message, "Messaging: User %d\n", (intkey - '1') + 1);
									UART_SendText(id, (uint8_t *)message);									
								}
								else if (intkey != '\r' && intkey != '\n'){
									 UART_SendText(id, (uint8_t *)"Invalid Entry, please select valid menu item...!\nOr press '9' to exit!\n");
								}
								break; 
								
					}
				default:
					break;
	}
}
	