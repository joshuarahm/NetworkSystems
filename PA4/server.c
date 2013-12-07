/*
 * Author: jrahm
 * created: 2013/12/04
 * server.h: <description>
 */

#include <pthread.h>
#include "pa4str.h"

/* The map of file names to
 * file file_stat_t */
#include "trie.h"
#include "trie_set.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>

pthread_mutex_t g_map_mutex;
trie_t          g_file_map;

pthread_mutex_t g_client_set_mutex;
trie_set_t      g_client_set;

int VERBOSE = 1;

#define verbose( fmt, ... ) \
    if( VERBOSE ) { printf( fmt, ##__VA_ARGS__ ); }

typedef struct {
    int out;
} printitr_args_t;

typedef struct {
    const char* name ;
} deregister_args_t;

typedef struct {
    int fd;
} callback_args_t;

static void printitr( printitr_args_t* args, file_stat_t* element ) {
    verbose( "Writing to output\n" );
    write_file_stat( args->out, element );
}

static void deregister_itr( deregister_args_t* args, file_stat_t* element ) {
    if( ! strcmp( element->_m_owner, args->name ) ) {
        trie_put( &g_file_map, element->_m_file_name, NULL ) ;
        // TODO change this to use a real destructor
        free( element );
    }
}

static int read_word( FILE* file, char* into, int size ) {
    int wordptr = 0;
    char ch;
    while( !isspace(ch = fgetc( file )) && ch != -1 && wordptr < size - 1) {
        into[wordptr ++] = ch ;
    }
    into[ wordptr ++ ] = 0;
    return wordptr;
}

void connection_callback( callback_args_t* args ) {
    FILE* asfile = fdopen( args->fd, "r" );
    char word[128];

    while( ! feof( asfile ) ) {
        read_word( asfile, word, 128 );
        if( ! strcmp( word, "REGISTER" ) ) {
            read_word( asfile, word, 128 );      
            verbose( "Determining if client %s exists\n", word );
    
            if( trie_set_contains( &g_client_set, word ) ) {
                verbose( "Client %s is already registered, aborting\n", word );
                fclose( asfile );
                return ;
            } else {
                trie_set_insert( &g_client_set, word ) ;
            }
        } else if ( ! strcmp( word, "NEWFILE" ) ) {
            file_stat_t* tmp = read_file_stat( asfile ) ;
            if( ! tmp ) {
                fprintf( stderr, "For some reason unable to read the file stat for newfile\n" );
            } else {
                verbose( "Adding file %s to file map\n", tmp->_m_file_name );
                trie_put( &g_file_map, tmp->_m_file_name, tmp );
            }
        } else if ( ! strcmp( word, "LIST" ) ) {
            printitr_args_t tmpargs;
            tmpargs.out = args->fd;
            verbose( "Sending list of files to peer\n" );
            trie_iterate( &g_file_map, ITERATE_FUNCTION(printitr), &tmpargs );
            write( args->fd, "END\n", 4 ) ;
        } else if ( ! strcmp( word, "DEREGISTER" ) ) {
            read_word( asfile, word, 128 );
            deregister_args_t tmpargs;
            tmpargs.name = word;
            verbose( "Deregistering client %s\n", word );
            trie_iterate( &g_file_map, ITERATE_FUNCTION( deregister_itr ), &tmpargs );
        }
    }
}

int start_server_socket( uint16_t port, void (*callback)( callback_args_t* args ) ) {
    int listen_fd = 0 ;
    struct sockaddr_in serv_addr ;

    listen_fd = socket( AF_INET, SOCK_STREAM, 0 ) ;
    memset( &serv_addr, 0, sizeof(serv_addr) ) ;

    serv_addr.sin_family = AF_INET ;
    serv_addr.sin_addr.s_addr = htonl( INADDR_ANY ) ;
    serv_addr.sin_port = htons( port ) ;

    if( bind( listen_fd, (struct sockaddr*)&serv_addr, sizeof( serv_addr ) ) ) {
        perror( "Error on bind" ) ;
        return -1 ;
    }

    if( listen(listen_fd, 1) ) {
        perror( "Error on listen" );
        return -1;
    }

    pthread_t tmp;
    callback_args_t argstmp;
    int fd;
    while( 1 ) {
        fd = accept( listen_fd, NULL, NULL ) ;
        if( fd <= 0 ) {
            perror( "Error on accept()" );
            return -1;
        }
        argstmp.fd = fd;
        verbose( "Accepted connection\n" );
        pthread_create( &tmp, NULL, ((void*(*)(void*))callback), &argstmp ) ;
    }
    
    return 0;
}

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

int main( int argc, char** argv ) {
    if ( argc != 2 ) {
        fprintf( stderr, "Must supply a port number\n" ) ;
        return 1 ;
    }
    
    uint16_t port;
    if( parse_port_num( argv[1], &port ) ) {
        start_server_socket( port, connection_callback );
        return 0 ;
    }

    fprintf( stderr, "Invalid port name\n" ) ;
    return 1;
}

