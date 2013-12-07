#include "client_state_machine.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define CAST( func ) \
    ( void*(*)( client_t* ) )(func)

void* wait_for_input( client_t* cli ) ;

int write_str( int fd, const char* str ) {
   return write( fd, str, strlen(str) ) ;
}

void* list_files( client_t* cli ) {
    write_str( cli->fd, "LIST\n" ) ;
    file_stat_t* stat = NULL ;
    int end = 0 ;

    printf( "Filename\tSize\tOwner\tHost\tPort\n" ) ;
    do {
        if( stat ) {
            trie_put( & cli->files, stat->_m_file_name, stat ) ;
            printf( "%s\t%02fKB\t%s\t%s\t%d\n",
                stat->_m_file_name, stat->_m_file_size / 1000.0f,
                stat->_m_owner, stat->_m_host_name, stat->_m_port ) ;
        }
        stat = read_file_stat_end( cli->as_file, & end ) ;
    } while ( stat && ! end ) ;

    return wait_for_input ;
}

void* wait_for_input( client_t* cli ) {
    (void) cli ;
    void* ret ;

    size_t len = 20 ;
    char* line = malloc( len );

    printf( "p2pget > " ) ;
    getline( & line, & len, stdin ) ;
    if( strcmp( line, "ls" ) ) {
        ret = list_files ;
    }

    free( line ) ;
    return ret ;
};

void* post_files( client_t* cli ) {
    
    return wait_for_input ;
}

void* init_state( client_t* cli ) {
    cli->as_file = fdopen( cli->fd, "r") ;
    memset( &cli->files, 0, sizeof( cli->files ) ) ;
    return post_files ;
}

int run_client( client_t* client ) {
    void*(*state)(client_t*) ;
    state = init_state ;
    while( state ) state = CAST( state( client ) );
}
