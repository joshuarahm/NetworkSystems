#include "client_state_machine.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

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
    if( !strcmp( line, "ls\n" ) ) {
        ret = list_files ;
    } else {
        fprintf( stderr, "%s: command not found!\n", line ) ;
        ret = wait_for_input ;
    }

    free( line ) ;
    return ret ;
};

static void write_stats( int fd, struct stat* stats, char* filename, char* name ) {
    file_stat_t filestats ;   
    filestats._m_file_name = filename ;
    filestats._m_file_size = stats->st_size ;
    
    // TODO change this
    filestats._m_owner = name ;
    filestats._m_host_name = "";
    filestats._m_port = 0 ;

    const char* nf = "NEWFILE " ;
    write( fd, nf, strlen( nf ) ) ;
    write_file_stat( fd, &filestats ) ;
}

void* post_files( client_t* cli ) {
    printf( "Posting Files\n" ) ;
    DIR *dir ;
    struct dirent *ent ;

    dir= opendir( "." ) ;
    struct stat stats ;

    if( dir ) {
        while( (ent = readdir( dir )) != NULL ) {
            if( stat( ent->d_name, & stats ) == 0 ) {
                write_stats( cli->fd, & stats, ent->d_name, cli->name ) ;
            } else {
                fprintf( stderr, "Warning: unable to stat file %s\n", ent->d_name ) ;
            }
        }
    } else {
        fprintf( stderr, "Warning: unable to open current directory\n" ) ;
    }

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
