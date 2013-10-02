#include "gbn_socket.h"

#include <unistd.h>
#include <stdlib.h>

#include <netdb.h>
#include <string.h>

int gethost( const char* hostname, uint16_t port, struct sockaddr_in* addr ) {
    struct hostent *he;

    if( ( he = gethostbyname( hostname ) ) == NULL ) {
        return 1;
    }

    memset( addr, 0, sizeof( struct sockaddr_in ) );
    memcpy( & addr->sin_addr.s_addr, he->h_addr, he->h_length );

    addr->sin_family = AF_INET;
    addr->sin_port = htons( port );

    return 0;
}

gbn_socket_t* gbn_socket_open_client( const char* hostname, uint16_t port ) {
    int sock = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP ) ;
    gbn_socket_t* ret = NULL;
    if ( sock > 0 ) {
        ret = calloc( sizeof( gbn_socket_t ), 1 );
        ret->_m_sockfd = sock;
    }

    gethost( hostname, port, & ret->_m_to_addr );
    return ret;
}

gbn_socket_t* gbn_socket_open_server( uint16_t port ) {
    gbn_socket_t* ret;
    int sock = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP );

    if( sock < 0 )
        return NULL;

	struct sockaddr_in sin;
	memset( &sin, 0, sizeof( sin ) );

	sin.sin_family = AF_INET;
	sin.sin_port = htons( port );
	sin.sin_addr.s_addr = INADDR_ANY;

	if( bind( sock, (struct sockaddr*)&sin, sizeof( sin ) ) < 0 ) {
		return NULL;
	}

    ret = calloc( sizeof( gbn_socket_t ), 1 );
    ret -> _m_sockfd = sock;

    return ret;
}

int gbn_socket_close( gbn_socket_t* sock ) {
    close( sock->_m_sockfd );
    free( sock );
}
