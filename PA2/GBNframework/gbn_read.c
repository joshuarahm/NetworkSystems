#include <stdlib.h>
#include "gbn_socket.h"
#include "debugprint.h"

#define GET_SEND_FRAME(sock, index) sock->_m_sending_window._m_packet_buffer[(sock->_m_sending_window._m_head+(index))%DEFAULT_QUEUE_SIZE]
#define GET_SEND_SIZE(sock) sock->_m_sending_window._m_size
#define GET_RECEIVE_FRAME(sock, index) sock->_m_receive_window._m_packet_buffer[(sock->_m_receive_window._m_head+(index))%DEFAULT_QUEUE_SIZE]
#define GET_RECEIVE_SIZE(sock) sock->_m_receive_window._m_size

#include <stdio.h>
#include <errno.h>

/* Given a packet, check whether it is a valid packet with the current window range.
   If so, return the difference between the first frame and the given packet seq number.
   Otherwise, return -1.
   */
int32_t GET_SEND_FRAME_id_delta(gbn_socket_t *socket, gbn_packet_t *packet) {
	gbn_packet_t *head_frame = &GET_SEND_FRAME(socket, 0);
	if (packet->_m_seq_number >= head_frame->_m_seq_number) {
		if (packet->_m_seq_number < head_frame->_m_seq_number + DEFAULT_QUEUE_SIZE) {
			return packet->_m_seq_number - head_frame->_m_seq_number;
		}
	}
	return -2;
}

/* Entry point for the socket reader thread. */
void gbn_socket_read_thread_main ( gbn_socket_t *socket) {
	int32_t bytes_recvd, wind_index;
	socklen_t sockaddr_size = 0;

	uint32_t pack_size = DEFAULT_PACKET_SIZE+sizeof(gbn_packet_t);
	gbn_packet_t ack_packet, incoming_packet;
	gbn_packet_t *window_sliding_cursor;
	uint8_t ack_serial[SERIALIZE_SIZE], incoming_buf[pack_size];
	int32_t framediff;
	uint32_t ack_size;
	data_block_t *block;

	data_block_t *to_rcv_queue;

	//Read thread should loop infinitely
	while (true) {
		//malloc a new packet to receive into.
		//incoming_packet = malloc(sizeof(gbn_packet_t));
		if (socket->_m_read_status == socket_status_closed && socket->_m_receive_window._m_size == 0)
			return;

		//TODO: Do we need to timeout here?
		debug4("Attempting to read from socket...\n");
		if ((bytes_recvd = recvfrom(socket->_m_sockfd, incoming_buf, pack_size, 0, (struct sockaddr *) &(socket->_m_to_addr), &sockaddr_size)) < 0) 
			return;

		debug4("Received %d bytes of data\n", bytes_recvd);

		//Deserialize the packet we received into packet
		gbn_socket_deserialize(incoming_buf, bytes_recvd, &incoming_packet);

		//We take one of two actions depending on what type of packet this is
		switch (incoming_packet._m_type) {
			case (gbn_packet_type_ack):
				debug4("Received ACK packet #%d.\n", incoming_packet._m_seq_number);
				/* If this is an ack packet, we need to:
				   		- Determine how many packets to acknowledge. (framediff)
							I.E. If the first frame is 4, and an ack for 5 comes in,
							we should ack both 4 AND 5.

						- Shift the send window right by that many packets.

						- Poke the writer, in case they are waiting for free space in the window
				 */
				pthread_mutex_lock(&socket->_m_sending_window._m_mutex);

				framediff = GET_SEND_FRAME_id_delta(socket, &incoming_packet)+1;
				if (framediff != -1) {
					//Our ack is valid and corresponds to some number of packets in our window
					socket->_m_sending_window._m_head += framediff;
					socket->_m_sending_window._m_head %= DEFAULT_QUEUE_SIZE;
					socket->_m_sending_window._m_size -= framediff;
					
				} 
				pthread_mutex_unlock(&socket->_m_sending_window._m_mutex);
				debug4("Curr sending window size: %d\n", socket->_m_sending_window._m_size);

				if (framediff != -1) {
					//Poke the writer
					pthread_mutex_lock(&socket->_m_mutex);
					pthread_cond_signal(&socket->_m_wait_for_ack);
					pthread_mutex_unlock(&socket->_m_mutex);
				}
				break;
			case (gbn_packet_type_data):
				debug4("Received data packet.\n");
				/* In this case we have received a data packet. We neet to:
				   		- Put the packet in the window, if it is valid

						- Iterate over the window from beginning until the first uninitialized packet
							Remove from the window, and push on the receive buffer

						- Send an ACK.
				   */
				//Make sure data packet is valid:
				debug4("Received packet id: %d\n", incoming_packet._m_seq_number);
				if (incoming_packet._m_seq_number >= socket->_m_receive_window._m_recv_counter) {
					if (incoming_packet._m_seq_number < socket->_m_receive_window._m_recv_counter + DEFAULT_QUEUE_SIZE) {
						debug4("Accepted frame %d\n", incoming_packet._m_seq_number);
						framediff = GET_SEND_FRAME_id_delta(socket, &incoming_packet);
						/* Seq number - recv counter determines the position to put into the window
						I.E. 	Seq number = 0. Packet 0 comes in. 0-0 = position [0] in window.
								Seq number = 0. Packet 1 comes in. 1-0 = position [1] in window.
								Seq number = 5. Packet 7 comes in. 7-5 = position [2] in window.
						*/

						wind_index = incoming_packet._m_seq_number - socket->_m_receive_window._m_recv_counter;
						GET_RECEIVE_FRAME(socket, wind_index) = incoming_packet;

						//If the packet is outside the current window but still valid, we need to extend the window out to it.
						//I.E. window contains [X,1] and 3 comes in, it needs to be expanded to [0,1,X,3] This increases size by 2.
						framediff = incoming_packet._m_seq_number - GET_RECEIVE_FRAME(socket, socket->_m_receive_window._m_size)._m_seq_number+1;
						if (framediff > 0) {
							debug4("Shifting frame by %d.\n", framediff);
							//The received frame is outside our window by 'framediff' number of frames. Expand to compensate.
							socket->_m_receive_window._m_tail += framediff;
							socket->_m_receive_window._m_tail %= DEFAULT_QUEUE_SIZE;
							socket->_m_receive_window._m_size += framediff;
						}
					}
				}

				//Remove as many received frames as possible from the front of the window
				while (GET_RECEIVE_FRAME(socket, 0)._m_type != gbn_packet_type_uninitialized) {
					debug4("Removing front frame from window.\n");
					//If the type is not uninitialized, we can post it to the block queue
					window_sliding_cursor = &GET_RECEIVE_FRAME(socket, 0);

					//Data_block_t's are malloc'd here, but it is the responsibility of whoever pops them to free them.
					to_rcv_queue = malloc(sizeof(data_block_t));
					to_rcv_queue->_m_data = window_sliding_cursor->_m_payload; //We don't free payload either because it also goes up onto the block queue
					to_rcv_queue->_m_len  = window_sliding_cursor->_m_size;
					to_rcv_queue->_m_flags = 0;

					block_queue_push_chunk(&(socket->_m_receive_buffer), to_rcv_queue);

					//Now we need to move the window right one entry.
					socket->_m_receive_window._m_recv_counter++; //We have ack'd one packet
					socket->_m_receive_window._m_head++; //Head moves right one
					socket->_m_receive_window._m_size--; //One less frame now.
					
					//We keep marking packets as uninitialized so we can tell received entries apart from non-received ones.
					window_sliding_cursor->_m_type = gbn_packet_type_uninitialized;
				}


				debug4("Preparing to send cumulative ack for #%d\n", socket->_m_receive_window._m_recv_counter-1);
				//Send a cumulative ack for all packets received up to this point
				ack_packet._m_seq_number = socket->_m_receive_window._m_recv_counter-1;
				//Ack packets are empty
				ack_packet._m_size = 0;
				ack_packet._m_type = gbn_packet_type_ack;

				ack_size = gbn_socket_serialize(&ack_packet, ack_serial, SERIALIZE_SIZE);
				sendto_(socket->_m_sockfd, ack_serial, ack_size, 0, (struct sockaddr *) &(socket->_m_to_addr), sizeof(struct sockaddr_in));

				if (incoming_packet._m_size == 0)
					socket->_m_read_status = socket_status_closed;
					ack_packet._m_seq_number = socket->_m_receive_window._m_recv_counter-1;
					//Ack packets are empty
					ack_packet._m_size = 0;
					ack_packet._m_type = gbn_packet_type_ack;

					ack_size = gbn_socket_serialize(&ack_packet, ack_serial, SERIALIZE_SIZE);
					sendto_(socket->_m_sockfd, ack_serial, ack_size, 0, (struct sockaddr *) &(socket->_m_to_addr), sizeof(struct sockaddr_in));
					block = malloc(sizeof(data_block_t));
					*block = (data_block_t){ NULL, 0, IS_CLOSING};
					block_queue_push_chunk( & socket->_m_sending_buffer, block );

				break;
			default:
				break;
			case gbn_packet_type_ack_EOF:
				return;
				break;
		}
	}
}
