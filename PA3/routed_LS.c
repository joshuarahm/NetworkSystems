#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>

#include "router.h"
#include "logging.h"


int main(int argc, char* argv[]) {
	int i;

	if( argc < 2 ) {
		fprintf( stderr, "Not enough arguments\n" );
		return -1;
	}

	if( argv[1][0] == 0 || argv[1][1] != 0 ) {
		fprintf( stderr, "Expected single char\n" );
		return -1;
	}

	router_t router;
	parse_router( argv[1][0], & router, "PA3_initialization.txt" );

	/* Just print the entries for now */
	printf( "Entries:\n" );
	for( i = 0 ; i < router._m_num_neighbors; ++ i ) {
		neighbor_t* neighbor = & router._m_neighbors_table[i];
		printf( "[neighbor node_id=%c outgoing_tcp_port=%d dest_tcp_port=%d cost=%d serv_fd=%d sock_fd=%d",
			neighbor->node_id, neighbor->outgoing_tcp_port, neighbor->dest_tcp_port,
			neighbor->cost, neighbor->serv_fd, neighbor->sock_fd );
		printf("\n");
	}

	return 0;
}
