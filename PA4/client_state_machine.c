#include "client_state_machine.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <dirent.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define CAST( func ) \
    ( void*(*)( client_t* ) )(func)


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

void* exit_client( client_t* cli ) {
	char buf[2048];
	snprintf(buf, 2048, "DEREGISTER %s\n", cli->name);
	write_str( cli->fd, buf);
	return NULL;
}

void* wait_for_input( client_t* cli ) {
    (void) cli ;
    void* ret ;

    size_t len = 20 ;
    char* line = malloc( len );
    int rc ;

    printf( "p2pget > " ) ;
    rc = getline( & line, & len, stdin ) ;
    if( rc <= 0 ) {
        printf( "\n" ) ;
        return wait_for_input ;
    }

    if( !strcmp( line, "ls\n" ) ) {
        ret = list_files ;
    } else if ( ! strncmp( line, "get ", 4) ) {
        char* filename = line  + 4 ;
        int len = strlen( filename );

        if( filename[len-1] == '\n' ) {
            filename[len-1] = '\0' ;
        }

        file_stat_t* fs = trie_get( &cli->files, filename ) ;
        if( fs ) {
            int fd = connect_client( fs->_m_host_name, fs->_m_port ) ;

            if( fd <= 0 ) {
                fprintf( stderr, "Unable to connect to peer\n" ) ;
            } else {
                uint32_t len ;
                FILE* output ;
                output = fopen( filename, "w" ) ;

                if( ! output ) {
                    perror( "Unable to open file for read" ) ;
                } else {
                    char buf[ 4096 ] ;
                    snprintf( buf, sizeof( buf ), "%s\n", filename ) ;
                    write( fd, buf, strlen(buf) ) ;
                    read( fd, & len, sizeof( len ) ) ;
                    len = ntohl( len ) ;

                    while ( len > 0 ) {
                        int bytes_read = read( fd, buf, sizeof( buf ) ) ; 

                        if ( bytes_read <= 0 ) {
                            fprintf( stderr, "Unable to finish download file\n" ) ;
                            break ;   
                        }

                        fwrite( buf, 1, bytes_read, output ) ; 
                        len -= bytes_read ;
                        fflush( output ) ;
                    }

                    fclose( output ) ;
                }
            }
        } else {
            fprintf( stderr, "No such file, maybe try an ls to refresh\n" ) ;
        }

        ret = wait_for_input ;
	} else if ( ! strcmp( line, "exit\n" ) ) {
		return exit_client;

    } else {
		if (strlen(line) > 0)
			line[strlen(line)-1] = '\0';
		if (strlen(line) > 0)
			fprintf( stderr, "%s: command not found!\n", line ) ;
        ret = wait_for_input ;
    }

    free( line ) ;
    return ret ;
};

static void write_stats( int fd, struct stat* stats, char* filename, char* name, int peer_fd ) {
	struct sockaddr_in sockinfo;
	socklen_t socklen = sizeof( sockinfo ) ;
	getsockname(peer_fd, (struct sockaddr*) &sockinfo, &socklen);
    file_stat_t filestats ;   
    filestats._m_file_name = filename ;
    filestats._m_file_size = stats->st_size ;
    
    // TODO change this
    filestats._m_owner = name ;
    filestats._m_host_name = "";
    filestats._m_port = ntohs(sockinfo.sin_port);

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
                write_stats( cli->fd, & stats, ent->d_name, cli->name, cli->peer_fd ) ;
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
	char buf[1024];
	snprintf(buf, 1024, "REGISTER %s\n", cli->name);
	write_str(cli->fd, buf);

    cli->as_file = fdopen( cli->fd, "r") ;
    memset( &cli->files, 0, sizeof( cli->files ) ) ;
    return post_files ;
}

int run_client( client_t* client ) {
    void*(*state)(client_t*) ;
    state = init_state ;
    while( state ) state = CAST( state( client ) );
}
