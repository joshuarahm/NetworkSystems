#include "gbn_socket.h"

#include <time.h>

typedef void*(*gbn_function_t)(gbn_socket_t*);

#define FCAST(a) \
    ((gbn_function_t)(a))

static gbn_function_t window_empty_q( gbn_socket_t* sock ) ;
static gbn_function_t block_on_queue( gbn_socket_t* sock ) ;
static gbn_function_t send_packet   ( gbn_socket_t* sock );
static gbn_function_t is_win_full_q ( gbn_socket_t* sock );
static gbn_function_t wait_on_read  ( gbn_socket_t* sock );
static gbn_function_t retransmit    ( gbn_socket_t* sock );

gbn_function_t window_empty_q( gbn_socket_t* sock ) {
    if( sock->_m_sending_window._m_size == 0 ) {
        /* the window is empty */
        return FCAST(block_on_queue);
    } else {
        return FCAST(send_packet);
    }
}

gbn_function_t block_on_queue( gbn_socket_t* sock ) {
    block_queue_peek_chunk( & sock->_m_sending_buffer );
    return FCAST(send_packet);
}

gbn_function_t send_packet( gbn_socket_t* sock ) {
    gbn_window_t* tmp = & sock->_m_sending_window;
    data_block_t* block = block_queue_pop_chunk( & sock->_m_sending_buffer );

    gbn_packet_t packet;
    packet._m_type = gbn_packet_type_data;
    packet._m_size = block->_m_len;
    packet._m_payload = block->_m_data;
    packet._m_seq_number = sock->_m_seq_number ++ ;;

    /* We may want to make this our own
     * function */
    pthread_mutex_lock( & tmp->_m_mutex );
    tmp->_m_packet_buffer[tmp->_m_tail] =  packet;
    tmp->_m_tail = (tmp->_m_tail + 1) % DEFAULT_QUEUE_SIZE ;
    pthread_mutex_unlock( & tmp->_m_mutex );

    uint32_t size;

    uint8_t buffer[ SERIALIZE_SIZE ];
    size = gbn_socket_serialize( & packet, buffer, SERIALIZE_SIZE );

    sendto( sock->_m_sockfd, buffer, size, 0,
        (struct sockaddr*)&sock->_m_to_addr,
            sizeof( struct sockaddr_in ) );

    return FCAST(is_win_full_q);
}


static gbn_function_t is_win_full_q ( gbn_socket_t* sock ) {
    if( !(sock->_m_sending_window._m_size < DEFAULT_QUEUE_SIZE || 
      block_queue_is_empty( & sock->_m_sending_buffer ) ) ) {
        return FCAST(send_packet);
    }

    return FCAST(wait_on_read) ;
}

static gbn_function_t wait_on_read  ( gbn_socket_t* sock ) {
    struct timespec ts;
    clock_gettime( CLOCK_REALTIME, & ts );
    ts.tv_nsec += 50 * (1000000);
    if( !pthread_cond_timedwait( & sock->_m_wait_for_ack, & sock->_m_mutex, & ts ) ) {
        return FCAST(window_empty_q);
    }
    return FCAST(retransmit);
}

static gbn_function_t retransmit ( gbn_socket_t* sock ) {
    uint32_t size;
    uint8_t buffer[ SERIALIZE_SIZE ];
    gbn_window_t* tmp = &sock->_m_sending_window;

    int i = sock->_m_sending_window._m_head;

    do {
    
        size = gbn_socket_serialize( & tmp->_m_packet_buffer[i],
          buffer, SERIALIZE_SIZE );
    
        sendto( sock->_m_sockfd, buffer, size, 0,
            (struct sockaddr*)&sock->_m_to_addr,
                sizeof( struct sockaddr_in ) );
    
        i = (i + 1) % DEFAULT_QUEUE_SIZE;
    } while( i != tmp->_m_tail );

    return FCAST(wait_on_read);
}


void gbn_socket_write_thread_main( gbn_socket_t* sock ) {
    gbn_function_t next = FCAST(window_empty_q);
    while ( true ) {
        next = next( sock );
    }
}
