#ifndef CLIENT_STATE_MACHINE_H_
#define CLIENT_STATE_MACHINE_H_

/*
 * Author: jrahm
 * created: 2013/12/06
 * client_state_machine.h: <description>
 */

#include "trie.h"
#include "pa4str.h"

typedef struct {
    int fd ;
	int peer_fd;
    char* name ;
    FILE* as_file ;
    trie_t files ;
} client_t ;

int run_client( client_t* client ) ;

int connect_client( const char* hostname, uint16_t port ) ;

#endif /* CLIENT_STATE_MACHINE_H_ */
