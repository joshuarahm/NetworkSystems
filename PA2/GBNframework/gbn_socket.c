#include "gbn_socket.h"

#include <unistd.h>
#include <stdlib.h>

#include <netdb.h>
#include <string.h>

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

    /* Return the socket */
    return ret;
}

/* Closes the socket and frees the pointer */
int gbn_socket_close( gbn_socket_t* sock ) {
    close( sock->_m_sockfd );
    free( sock );
}
