#ifndef SSL_H_
#define SSL_H_

/*
 * Author: jrahm
 * created: 2013/11/29
 * ssl.h: <description>
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

typedef struct {
    /* This is the socket that is streight from the
     * kernel */
    int _m_socket;

    /* The stream that will be encrypted */
    SSL * _m_ssl_stream;

    /* The context for performing SSL
     * operations */
    SSL_CTX *_m_ssl_context;

} SSLConnection;

#define REASON_LEN 256
char connect_tcp_reason[ REASON_LEN ] ;

int connect_tcp( SSLConnection* ssl_connection, const char* server, uint16_t port ) {
    int err;
    int sock;

    struct hostent* host;
    struct sockaddr_in servaddr;

    host = gethostbyname( server );
    sock = socket( AF_INET, SOCK_STREAM, 0 );

    if( sock == -1 ) {
        snprintf( connect_tcp_reason, REASON_LEN, "%s", strerror( sock ) );
        return sock;
    } else {
        servaddr.sin_family = AF_INET ;
        servaddr.sin_port = htons( port ) ;
        servaddr.sin_addr = *( (struct in_addr *) host->h_addr ) ;
        memset( &servaddr.sin_zero, 0, sizeof( servaddr.sin_zero ) ) ;

        err = connect( sock, (struct sockaddr*) &server, sizeof ( struct sockaddr ) );
        if ( err == -1 ) {
            return -1;
        }
    }

    ssl_connection->_m_socket = sock;
}

int connect_ssl( SSLConnection* conn, const char* server, uint16_t port ) {
    conn -> _m_ssl_stream = NULL ;
    conn -> _m_ssl_context = NULL ;

    conn -> _m_socket = connect_tcp( conn, server, port ) ;
    if ( conn -> _m_socket > 0 ) {
        SSL_load_error_strings () ;
        SSL_library_init () ;

        conn -> _m_ssl_context = SSL_CTX_new( SSLv23_client_method() ) ;
        // TODO CONTINUE TO IMPLEMENT THIS
    }
    return 0;
}

#endif /* SSL_H_ */
