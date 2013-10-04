#include "gbn_socket.h"

#include <time.h>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#include <stdlib.h>
#include "debugprint.h"

/* this would ideally be a typedef to
 * a fuction that returns a function that
 * returns a function ... */
typedef void*(*gbn_function_t)(gbn_socket_t*);

/* Cast something to a gbn_function_t */
#define FCAST(a) \
    ((gbn_function_t)(a))

/* These are functions that represent
 * nodes in a state-machine diagram */
static gbn_function_t window_empty_q ( gbn_socket_t* sock ) ;
static gbn_function_t block_on_queue ( gbn_socket_t* sock ) ;
static gbn_function_t send_packet    ( gbn_socket_t* sock );
static gbn_function_t is_win_full_q  ( gbn_socket_t* sock );
static gbn_function_t wait_on_read   ( gbn_socket_t* sock );
static gbn_function_t retransmit     ( gbn_socket_t* sock );

/* If the window is empty, transition
 * to a block on queue state, otherwise
 * transition to the is_win_full state */
gbn_function_t window_empty_q( gbn_socket_t* sock ) {
	debug3("[Writer] Transitioning into state: window_empty_q\n")
    if( sock->_m_sending_window._m_size == 0 ) {
        /* the window is empty */
        return FCAST(block_on_queue);
    } else {
        return FCAST(is_win_full_q);
    }
}

/* blocks while there is no data on
 * the queue */
gbn_function_t block_on_queue( gbn_socket_t* sock ) {
	debug3("[Writer] Transitioning into state: block_on_queue\n")
    /* Block unitl there is some data */
    block_queue_peek_chunk( & sock->_m_sending_buffer );

    /* transition to sending 
     * packet */
    return FCAST(send_packet);
}

/* Sends a packet across the wire and updates
 * the window appropriately */
gbn_function_t send_packet( gbn_socket_t* sock ) {
	debug3("[Writer] Transitioning into state: send_packet\n")
    gbn_window_t* tmp = & sock->_m_sending_window;
    data_block_t* block = block_queue_pop_chunk( & sock->_m_sending_buffer );

	if( block->_m_flags & IS_CLOSING ) {
		debug2( "[Writer] Writer received EOF\n" );
		return NULL;
	}

    /* Construct a new packet */
    gbn_packet_t packet;
    packet._m_type = gbn_packet_type_data;
    packet._m_size = block->_m_len;
    packet._m_payload = block->_m_data;
    packet._m_seq_number = sock->_m_seq_number ++ ;;

    /* We may want to make this our own
     * function */
    pthread_mutex_lock( & tmp->_m_mutex );
    /* Update the sending window */
    tmp->_m_packet_buffer[tmp->_m_tail] =  packet;
    tmp->_m_tail = (tmp->_m_tail + 1) % DEFAULT_QUEUE_SIZE ;
	tmp->_m_size ++;
    pthread_mutex_unlock( & tmp->_m_mutex );

    uint32_t size;

    /* Construct a serialized packet */
    uint8_t buffer[ SERIALIZE_SIZE ];
    size = gbn_socket_serialize( & packet, buffer, SERIALIZE_SIZE );

    /* send the serialize packet */
	debug3( "Sending a packet of size: %d\n", size );
    sendto( sock->_m_sockfd, buffer, size, 0,
        (struct sockaddr*)&sock->_m_to_addr,
            sizeof( struct sockaddr_in ) );

	free( block->_m_data );
	free( block );

    /* Return the state asking if the window is full */
    return FCAST(is_win_full_q);
}

/* This function sends a packcet
 * if the window is not full and
 * the queue is not empty, otherwise
 * it transitions to waiting for
 * the acks */
static gbn_function_t is_win_full_q ( gbn_socket_t* sock ) {
	debug3("[Writer] Transitioning into state: is_win_full_q\n")
	debug4("[Writer] Sending Window Size: %d, Block Queue Empty? %d\n",
	  sock->_m_sending_window._m_size,
	  block_queue_is_empty( & sock->_m_sending_buffer ) );

    if( sock->_m_sending_window._m_size < DEFAULT_QUEUE_SIZE && 
      !block_queue_is_empty( & sock->_m_sending_buffer ) ) {
        /* Send yet another packet */
        return FCAST(send_packet);
    }

    /* Wait for the reader to acknowledge
     * all the packets */
    return FCAST(wait_on_read) ;
}

static void millis_in_future( struct timespec* ts, long millis ) {
	#ifdef __MACH__
		clock_serv_t cclock;
		mach_timespec_t mts;
		host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
		clock_get_time(cclock, &mts);
		mach_port_deallocate(mach_task_self(), cclock);
		ts->tv_sec = mts.tv_sec;
		ts->tv_nsec = mts.tv_nsec;
	#else
		clock_gettime(CLOCK_REALTIME, ts);
	#endif

    /* Add 50 ms */
    ts->tv_nsec += millis * 1000000;
	ts->tv_sec += ts->tv_nsec / 1000000000;
	ts->tv_nsec %= 1000000000;
}

/* Waits for the reader to signal. If the
 * reader does not signal within the specified
 * timeout, then the next state is retransmission
 * otherwise, this function returns to the start
 * state */
static gbn_function_t wait_on_read  ( gbn_socket_t* sock ) {
	debug3("[Writer] Transitioning into state: wait_on_read\n")
    /* set the time to wait
     * until */
    struct timespec ts;
	millis_in_future( &ts, 50 );
    /* Current time */
    //clock_gettime( CLOCK_REALTIME, & ts );

    /* Wait on the condition for the
     * reader to signal us on */
    if( !pthread_cond_timedwait( & sock->_m_wait_for_ack,
      & sock->_m_mutex, & ts ) ) {
        /* The wait did not time out */

        /* transition to asking
         * if the window is empty */
        return FCAST(window_empty_q);
    };

    /* [Writer] Transition to retransmit */
    return FCAST(retransmit);
}

/* Retransmission stage. Called when
 * the reader has not signaled us in
 * time. Sends all packets in the window
 * again */
static gbn_function_t retransmit ( gbn_socket_t* sock ) {
	debug3("[Writer] Transitioning into state: retransmit\n")
    uint32_t size;
    uint8_t buffer[ SERIALIZE_SIZE ];
    gbn_window_t* tmp = &sock->_m_sending_window;

    if( sock->_m_sending_window._m_size == 0 ) {
        /* should not happen */
		debug4("[Writer] Window size == 0\n" );
        return FCAST( wait_on_read );
    }

    /* start at the head */
    int i = sock->_m_sending_window._m_head;

    do {
        /* serialize and send the
         * next packet */
        size = gbn_socket_serialize( & tmp->_m_packet_buffer[i],
          buffer, SERIALIZE_SIZE );
    
		debug4("[Writer] Sending buffer to socket\n" );
        sendto( sock->_m_sockfd, buffer, size, 0,
            (struct sockaddr*)&sock->_m_to_addr,
                sizeof( struct sockaddr_in ) );
    
        /* Increment i */
        i = (i + 1) % DEFAULT_QUEUE_SIZE;

        /* Loop until i != tail. This is a do-while
         * because it is possible that tail == head */
    } while( i != tmp->_m_tail );

    return FCAST(wait_on_read);
}


void gbn_socket_write_thread_main( gbn_socket_t* sock ) {
    /* the start state is asking if the
     * window is empty */
    gbn_function_t next = FCAST(window_empty_q);

    while ( next ) {
        /* Forever execute the current function
         * and the execute it's successor and execute
         * it's successor and execute it's successor
         * and ...  */
        next = next( sock );
    }
}
