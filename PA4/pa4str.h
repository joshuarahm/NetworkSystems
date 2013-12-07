#ifndef PA4STR_H_
#define PA4STR_H_

/*
 * Author: jrahm
 * created: 2013/12/03
 * pa4str.h: <description>
 */

#include <inttypes.h>
#include <stdio.h>


/* name/size:owner:host:port */
typedef struct {
    char*    _m_file_name ;
    uint64_t _m_file_size ;
    char*    _m_owner ;
    char*    _m_host_name ;
    uint16_t _m_port ;
    int eof ;
} file_stat_t ;

/* Reads a file_stat_t from a file descriptor */
file_stat_t* read_file_stat( FILE* fd ) ;

void write_file_stat( int fd, file_stat_t* filestat ) ;

file_stat_t* read_file_stat_end( FILE* fd, int* end ) ;

#endif /* PA4STR_H_ */
