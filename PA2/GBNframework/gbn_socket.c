#include "gbn_socket.h"

#include <unistd.h>
#include <stdlib.h>

gbn_socket_t* gbn_socket_open( const char* hostname, uint16_t port ) {

}

int gbn_socket_close( gbn_socket_t* socket ) {
    close( socket->_m_sockfd );
    free( socket );
}
