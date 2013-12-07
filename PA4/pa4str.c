#include "pa4str.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUF_LEN 1024

static char reason[1024] ;

file_stat_t* read_file_stat_end( FILE* fd, int* end ) {
    char buf[ BUF_LEN ];
    size_t buf_pos = 0;
    file_stat_t* ret = calloc( sizeof( file_stat_t ), 1 );

    if( end ) {
        *end = 0 ;
    }
    int ch;
    while ( ( ch = fgetc( fd ) ) != '/'  && ch != -1 ) {
        if ( ch == '\n' ) {
            buf[buf_pos] = '\0' ;
            if ( strcmp( buf, "END" ) == 0 ) {
                if( end ) {
                    *end = 1 ;
                }
            }
            return NULL ;
        }
        if ( buf_pos >= BUF_LEN - 1 ) {
            snprintf( reason, 1024, "Filename too long" ) ;
            free( ret ) ;
            return NULL ;
        }

        buf[buf_pos ++] = ch ;
    }

    buf[buf_pos ++] = 0 ;
    ret->_m_file_name = strdup( buf ) ;
    
    uint64_t size = 0;
    while ( (ch = fgetc( fd )) != ':'  && ch != -1 ) {
        if ( ch < 0x30 || ch >= 0x40 ) {
            snprintf( reason, 1024, "Invalid character in file size field %c\n", ch ) ;
            free( ret ) ;
            return NULL ;
        }

        size = size * 10 + (ch - 0x30) ;
    }
    ret->_m_file_size = size ;

    buf_pos = 0;
    while ( (ch = fgetc( fd )) != ':'  && ch != -1 ) {
        if ( buf_pos >= BUF_LEN - 1 ) {
            snprintf( reason, 1024, "Owner name too long" ) ;
            free( ret ) ;
            return NULL ;
        }

        buf[buf_pos ++] = ch ;
    }

    buf[buf_pos ++] = 0 ;
    ret->_m_owner = strdup( buf ) ;

    buf_pos = 0;
    while ( (ch = fgetc( fd )) != ':'  && ch != -1 ) {
        if ( buf_pos >= BUF_LEN - 1 ) {
            snprintf( reason, 1024, "Host name too long" ) ;
            free( ret ) ;
            return NULL ;
        }

        buf[buf_pos ++] = ch ;
    }

    buf[buf_pos ++] = 0 ;
    ret->_m_host_name = strdup( buf ) ;

    size = 0;
    while ( (ch = fgetc( fd )) != '\n'  && ch != -1 ) {
        if ( ch < 0x30 || ch >= 0x40 ) {
            snprintf( reason, 1024, "Invalid character in port field %c\n", ch ) ;
            free( ret ) ;
            return NULL ;
        }

        size = size * 10 + (ch - 0x30) ;
    }
    ret->_m_port = size ;

    if( ch == -1 ) {
        free( ret );
        snprintf( reason, 1024, "EOF reached" );
        return NULL;
    }

    return ret;
}

file_stat_t* read_file_stat( FILE* fd ) {
    return read_file_stat_end( fd, NULL ) ;
}

void write_file_stat( int fd, file_stat_t* filestat ) {
    char buf[2048];
    snprintf( buf, 2048, "%s/%lu:%s:%s:%u\n",
        filestat->_m_file_name,
        filestat->_m_file_size,
        filestat->_m_owner,
        filestat->_m_host_name,
        filestat->_m_port ) ;
    write( fd, buf, strlen(buf) );
}

#ifdef FILE_STAT_TESTING
int main( int argc, char** argv ) {
    FILE* file;
    file = fopen( "test_file_stat.txt", "r" );

    if( ! file ) {
        perror( "Unable to open file" );
        return 1;
    }

    while( ! feof( file ) ) {
        file_stat_t* tmp = read_file_stat( file );
        if( tmp ) {
            printf( "file_stat_t{ filename => %s, filesize => %02fKB, owner => %s, hostname => %s, port => %u }\n",
            tmp->_m_file_name, tmp->_m_file_size / 1000.0, tmp->_m_owner, tmp->_m_host_name, tmp->_m_port ); 
        }
    }

    return 0;
}
#endif
