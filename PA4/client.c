#include "pa4str.h"
#include "trie.h"
#include "trie_set.h"
#include "client_state_machine.h"

#include <unistd.h>
#include <netdb.h>
#include <memory.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int parse_port_num( const char* str, uint16_t* into ) {
    uint16_t ret = 0;
    char ch ;

    while ( *str != 0 ) {
        ch = * str ;
        if ( ch >= 0x40 || ch < 0x30 ) {
            return 0 ;
        }
        ret *= 10 ;
        ret += ch - 0x30 ;
        str ++ ;
    }

    * into = ret ;
    return 1 ;
}

int connect_client( const char* hostname, uint16_t port ) {
    int sock = socket( PF_INET, SOCK_STREAM, IPPROTO_TCP ) ;   
    struct hostent *he ;
    struct sockaddr_in server ;

    if ( sock > 0 ) {

        if( ( he = gethostbyname( hostname ) ) == NULL ) {
            return 0 ;
        }

        memset( & server, 0, sizeof( struct sockaddr_in ) ) ;
        memcpy( & server.sin_addr.s_addr, he->h_addr, he->h_length ) ;

        server.sin_family = AF_INET ;
        server.sin_port = htons( port ) ;

        if( connect( sock, (struct sockaddr *) & server, sizeof( server ) ) < 0 ) {
            return 0 ;
        }
    }

    return sock ;
}

int main( int argc, char** argv ) {
    if ( argc < 3 ) {
        fprintf( stderr, "must supply a server address and port\n" ) ;
        return 1 ;
    }
    
    uint16_t port ;
    if( ! parse_port_num( argv[2], & port ) ) {
        fprintf( stderr, "Must supply valid port number\n" ) ;
        return 2 ;
    }
    
    /* open a connection to the serever */
    int fd = connect_client( argv[1], port ) ;
    if ( fd == 0 ) {
        perror( "Unable to open client" ) ;
        return 3 ;
    }

    client_t cli ;
    cli.fd = fd ;
    run_client( & cli ) ;

    return 0 ;
}
