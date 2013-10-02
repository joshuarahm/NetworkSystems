#include "gbn_socket.h"

#include <unistd.h>
#include <stdlib.h>
#include <assert.h>

#include <netdb.h>
#include <string.h>

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

    /* Lock the mutex, this is used to avoid race
     * conditions with the condition variable */
    pthread_mutex_trylock( & sock->_m_mutex );

    /* Kick off reading and writing
     * threads */
    pthread_create( & sock->_m_write_thread, NULL,
        ( void*(*)(void*) )gbn_socket_write_thread_main, sock );

    pthread_create( & sock->_m_read_thread, NULL,
        ( void*(*)(void*) )gbn_socket_read_thread_main, sock );

    /* Initialize out block queues */
    block_queue_init( & sock->_m_receive_buffer, 128 );
    block_queue_init( & sock->_m_sending_buffer, 128 );
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
        init_gbn_socket( ret );
        ret->_m_sockfd = sock;
        gethost( hostname, port, & ret->_m_to_addr );
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
    init_gbn_socket( ret );
    ret -> _m_sockfd = sock;

    /* Return the socket */
    return ret;
}

int gbn_destroy_window( gbn_window_t* win ) {
    pthread_mutex_destroy( & win->_m_mutex );
}

/* Closes the socket and frees the pointer */
int gbn_socket_close( gbn_socket_t* sock ) {
    close( sock->_m_sockfd );
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

