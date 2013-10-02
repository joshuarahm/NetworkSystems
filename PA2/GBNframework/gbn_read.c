#include <stdlib.h>
#include "gbn_socket.h"

#define get_send_frame(sock, index) sock->_m_sending_window._m_packet_buffer[(sock->_m_sending_window._m_head+index)%DEFAULT_QUEUE_SIZE]
#define get_send_size(sock) sock->_m_sending_window._m_size
#define get_receive_frame(sock, index) sock->_m_receive_window._m_packet_buffer[(sock->_m_receive_window._m_head+index)%DEFAULT_QUEUE_SIZE]
#define get_receive_size(sock) sock->_m_receive_window._m_size

int32_t get_send_frame_id(gbn_socket_t *socket, gbn_packet_t *packet) {
	gbn_packet_t *head_frame = &get_send_frame(socket, 0);
	if (packet->_m_seq_number >= head_frame->_m_seq_number) {
		if (packet->_m_seq_number < head_frame->_m_size + get_send_size(socket)) {
			return packet->_m_seq_number - head_frame->_m_seq_number;
		}
	}
	return -1;
}

int gbn_read_thread_main ( gbn_socket_t *socket) {
	uint32_t pack_size = DEFAULT_PACKET_SIZE+sizeof(gbn_packet_t);
	uint32_t sockaddr_size, bytes_recvd, bytes_sent;
	gbn_packet_t *ack_packet;
	uint8_t *ack_serial = malloc(SERIALIZE_SIZE);
	data_block_t *to_rcv_queue = malloc(sizeof(data_block_t));

	while (true) { //Main loop
		uint8_t *buf = malloc(pack_size);
		gbn_packet_t *packet = malloc(sizeof(gbn_packet_t));

		bytes_recvd = recvfrom(socket->_m_sockfd, buf, pack_size, 0, (struct sockaddr *) &(socket->_m_to_addr), &sockaddr_size);

		gbn_socket_deserialize(buf, bytes_recvd, packet);
		switch (packet->_m_type) {
			case (gbn_packet_type_ack):
				pthread_mutex_lock(&socket->_m_sending_window._m_mutex);

				int32_t framediff = get_send_frame_id(socket, packet);
				if (framediff != -1) {
					socket->_m_sending_window._m_head += framediff;
					socket->_m_sending_window._m_head %= DEFAULT_QUEUE_SIZE;
				}

				pthread_mutex_unlock(&socket->_m_sending_window._m_mutex);
				pthread_mutex_lock(&socket->_m_mutex);

				pthread_cond_signal(&socket->_m_wait_for_ack);

				pthread_mutex_unlock(&socket->_m_mutex);
				break;
			case (gbn_packet_type_data):
				//if (socket->_m_receive_window._m_size == 0) {
					//Use as initial packet
				if (packet->_m_seq_number >= socket->_m_receive_window._m_recv_counter) {
					if (packet->_m_seq_number < socket->_m_receive_window._m_recv_counter + DEFAULT_QUEUE_SIZE) {
						socket->_m_receive_window._m_packet_buffer[socket->_m_receive_window._m_tail] = *packet;
						socket->_m_receive_window._m_tail++;

					}
				}

				while (get_receive_frame(socket, 0)->_m_type != gbn_packet_type_uninitialized) {
				ack_packet = malloc(sizeof(gbn_packet_t));
				ack_packet->_m_seq_number = packet->_m_seq_number;
				ack_packet->_m_size = 0;
				ack_packet->_m_type = gbn_packet_type_ack;

				gbn_socket_serialize(packet, ack_serial, SERIALIZE_SIZE);

				bytes_sent = sendto(socket->_m_sockfd, ack_serial, pack_size, 0, (struct sockaddr *) &(socket->_m_to_addr), sizeof(struct sockaddr_in));

				to_rcv_queue->_m_data = packet->_m_payload;
				to_rcv_queue->_m_len  = packet->_m_size;
				free(packet);
				block_queue_push_chunk(&(socket->_m_receive_buffer), to_rcv_queue);
				break;
			default:
				break;

		}
	}
}
