#include "gbn_socket.h"

#include <unistd.h>
#include <stdlib.h>
#include <assert.h>

#include <netdb.h>
#include <string.h>
#include "debugprint.h"

static void init_gbn_window( gbn_window_t* win ) {
    pthread_mutex_init( & win->_m_mutex, 0 );
}

static void init_gbn_socket( gbn_socket_t* sock ) {
    /* Initialize the windows */
    init_gbn_window( & sock->_m_receive_window );   
    init_gbn_window( & sock->_m_sending_window );

    /* Mutexes and conditions */
    pthread_mutex_init( & sock->_m_mutex, 0 );
    pthread_cond_init( & sock->_m_wait_for_ack, 0 );

	/* Set the socket status: */
	sock->_m_status = socket_status_open;
	sock->_m_read_status = socket_status_open;

    /* Lock the mutex, this is used to avoid race
     * conditions with the condition variable */
    pthread_mutex_trylock( & sock->_m_mutex );

    /* Initialize out block queues */
    block_queue_init( & sock->_m_receive_buffer, 128 );
    block_queue_init( & sock->_m_sending_buffer, 128 );

    /* Kick off reading and writing
     * threads */
    pthread_create( & sock->_m_write_thread, NULL,
        ( void*(*)(void*) )gbn_socket_write_thread_main, sock );

    pthread_create( & sock->_m_read_thread, NULL,
        ( void*(*)(void*) )gbn_socket_read_thread_main, sock );
}

/*
 * Get the host by resolving the hostname and port number
 * and store the results in `addr`
 */
int gethost( const char* hostname, uint16_t port, struct sockaddr_in* addr ) {
    struct hostent *he;

    /* Get the host */
    if( ( he = gethostbyname( hostname ) ) == NULL ) {
        return 1;
    }

    /* Null-out the address */
    memset( addr, 0, sizeof( struct sockaddr_in ) );
    /* copy over the address of the host */
    memcpy( & addr->sin_addr.s_addr, he->h_addr, he->h_length );

    /* set the family and port */
    addr->sin_family = AF_INET;
    addr->sin_port = htons( port );

    return 0;
}

/* Opens a new client and connects to the
 * host. Returns a pointer to the socket
 * which can brea read from and written to
 */
gbn_socket_t* gbn_socket_open_client( const char* hostname, uint16_t port ) {
    /* Create the underlying socket descriptor */
    int sock = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP ) ;
    gbn_socket_t* ret = NULL;

    if ( sock > 0 ) {
        /* Allocate the new file if we can */
        ret = calloc( sizeof( gbn_socket_t ), 1 );
        ret->_m_sockfd = sock;
        gethost( hostname, port, & ret->_m_to_addr );
        init_gbn_socket( ret );
    }

    /* Return the file descriptor */
    return ret;
}

/* Opens a connection that listens on the
 * port `port` */
gbn_socket_t* gbn_socket_open_server( uint16_t port ) {
    gbn_socket_t* ret;

    /* Create the socket */
    int sock = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP );

    if( sock < 0 )
        /* Return NULL to signal failure */
        return NULL;

    /* Create the socket address to bind to */
	struct sockaddr_in sin;
	memset( &sin, 0, sizeof( sin ) );

    /* Set some boiler */
	sin.sin_family = AF_INET;
	sin.sin_port = htons( port );
	sin.sin_addr.s_addr = INADDR_ANY;

    /* Bind the socket to the address */
	if( bind( sock, (struct sockaddr*)&sin, sizeof( sin ) ) < 0 ) {
		return NULL;
	}

    /* Create a new socket and return int */
    ret = calloc( sizeof( gbn_socket_t ), 1 );
    ret -> _m_sockfd = sock;
    init_gbn_socket( ret );

    /* Return the socket */
    return ret;
}

int gbn_destroy_window( gbn_window_t* win ) {
    pthread_mutex_destroy( & win->_m_mutex );
    return 0;
}

#define min( a, b ) \
    ( (a)>(b) ? (b) : (a) )

int gbn_socket_read( gbn_socket_t* sock, char* bytes, uint32_t len ) {
    data_block_t* block;
    size_t bytes_read = 0;

	/* If the socket is closed, we will do no more reading */
	if (sock->_m_status == socket_status_closed) 
		return 0;

    /* Handle the original case where
     * the first block may be partially
     * read already */
    block = block_queue_peek_chunk( & sock->_m_receive_buffer );
    bytes_read = min( len, block->_m_len - sock->_m_current_block_ptr );
    memcpy( bytes, block->_m_data, bytes_read );

    if( bytes_read != len ) {
        /* Exhaused the current chunk */
        free( block->_m_data );
        free( block );
        block_queue_pop_chunk( & sock->_m_receive_buffer );
        sock->_m_current_block_ptr = 0;
    
        while( bytes_read < len ) {
			debug4("block len: %d\n", block->_m_len);
			if ( block->_m_len == 0 ) {
				debug4("Found EOF block. Closing socket...\n");
				sock->_m_status = socket_status_closed;
				return bytes_read;
			}

            block = block_queue_peek_chunk( & sock->_m_receive_buffer );
    
            if( block->_m_len <= len - bytes_read ) {
                /* The read will consume the block */
    
                /* Read the full block of data */
                memcpy( bytes + bytes_read, block->_m_data, block->_m_len );
                bytes_read += block->_m_len;
    
                /* Free the block, we don't need it any
                * more */
                free( block->_m_data );
                free( block );
    
                /* Pop the chunk off the queue */
                block_queue_pop_chunk( & sock->_m_receive_buffer );
            } else {
                /* The block will be partially read */
                memcpy( bytes + bytes_read, block->_m_data, len - bytes_read );
                sock->_m_current_block_ptr = len - bytes_read;
                bytes_read = len;
            }
        }
    } else {
        /* The first chunk was still not exhausted */
        sock->_m_current_block_ptr += bytes_read;
    }

    return bytes_read;
}

/* Closes the socket and frees the pointer */
int gbn_socket_close( gbn_socket_t* sock ) {
	//Only send out ending blocks if the socket is still open
	if (sock->_m_status == socket_status_open) {
		data_block_t *block = malloc(sizeof(data_block_t));
		*block = (data_block_t){ NULL, 0, 0};
		block_queue_push_chunk( & sock->_m_sending_buffer, block );
		block = malloc(sizeof(data_block_t));
		*block = (data_block_t){ NULL, 0, IS_CLOSING};
		block_queue_push_chunk( & sock->_m_sending_buffer, block );
		pthread_join( sock->_m_write_thread, NULL );
	} else {
		pthread_join( sock->_m_read_thread, NULL );
	}

    close( sock->_m_sockfd );
	// pthread_join( sock->_m_read_thread, NULL );

    pthread_mutex_destroy( & sock->_m_mutex );
    pthread_cond_destroy(  & sock->_m_wait_for_ack );
    gbn_destroy_window( & sock->_m_sending_window );
    gbn_destroy_window( & sock->_m_receive_window );

    pthread_cancel( sock->_m_read_thread );
    pthread_cancel( sock->_m_write_thread );

    block_queue_free( & sock->_m_sending_buffer );
    block_queue_free( & sock->_m_receive_buffer );;

    free( sock );
	return 0;
}

uint32_t gbn_socket_serialize( gbn_packet_t *packet, uint8_t *buf, uint32_t buf_len ) {
	assert(packet->_m_size + SERIALIZE_OVERHEAD <= buf_len); //Our buf should be large enough for the payload + network header
	uint32_t *buf_longs = (uint32_t *) buf;

	buf_longs[0] = htonl( packet->_m_type );
	buf_longs[1] = htonl( packet->_m_seq_number );
	buf_longs[2] = htonl( packet->_m_size );

	memcpy(buf+SERIALIZE_OVERHEAD, packet->_m_payload, packet->_m_size);
	return packet->_m_size + SERIALIZE_OVERHEAD;
}

uint32_t gbn_socket_deserialize(uint8_t *buf, uint32_t buf_len, gbn_packet_t *packet ) {
	assert(DEFAULT_PACKET_SIZE + sizeof(gbn_packet_t) >= buf_len); //Out packet should be no larger than payload + network header
	uint32_t *buf_longs = (uint32_t *) buf;

	packet->_m_type       = ntohl( buf_longs[0] );
	packet->_m_seq_number = ntohl( buf_longs[1] );
	packet->_m_size       = ntohl( buf_longs[2] );

	packet->_m_payload = malloc(packet->_m_size);
	memcpy(packet->_m_payload, buf+SERIALIZE_OVERHEAD, packet->_m_size);
	return packet->_m_size + SERIALIZE_OVERHEAD;
}

int32_t gbn_socket_write ( gbn_socket_t* socket, const char* bytes, uint32_t len ) {
	uint32_t whole_blocks = (len/DEFAULT_PACKET_SIZE), cntr = 0, final_len = 0;
	data_block_t *my_block;
	uint8_t *out_buffer;
    const uint8_t* cursor = (const uint8_t*)bytes;

	debug3("Client attempting to write %d bytes, in %d or %d blocks.\n", len, whole_blocks, whole_blocks+1);

	//Copy as many whole blocks as we can. This loop should only generate blocks of
	//DEFAULT_PACKET_SIZE in length.
	for (cntr = 0; cntr < whole_blocks; cntr++) {
		out_buffer = malloc(DEFAULT_PACKET_SIZE);
		my_block = malloc(sizeof(data_block_t));
		my_block->_m_len = DEFAULT_PACKET_SIZE;
		my_block->_m_data = out_buffer;
		my_block->_m_flags = 0;
		memcpy(out_buffer, cursor, DEFAULT_PACKET_SIZE);
		block_queue_push_chunk(&socket->_m_sending_buffer, my_block);
		cursor += DEFAULT_PACKET_SIZE;
	}

	//If the input was not a proper multiple, there will be leftover bytes.
	//We are guaranteed the number of bytes B is 0 <= B < DEFAULT_PACKET_SIZE
	if (whole_blocks*DEFAULT_PACKET_SIZE < len) {
		final_len = len - (whole_blocks*DEFAULT_PACKET_SIZE);
		debug3("Client writing leftover block of size %d.\n", final_len);
		out_buffer = malloc(final_len);
		my_block = malloc(sizeof(data_block_t));

		my_block->_m_len = final_len;
		my_block->_m_data = out_buffer;
		my_block->_m_flags = 0;
		memcpy(out_buffer, cursor, final_len);
		block_queue_push_chunk(&socket->_m_sending_buffer, my_block);
	}

	return whole_blocks*DEFAULT_PACKET_SIZE + final_len;
}
