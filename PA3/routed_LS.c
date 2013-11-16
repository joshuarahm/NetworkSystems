#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>

#include "router.h"
#include "logging.h"
#include <signal.h>

router_t router;
int m_exit( ) {
    printf( "Closing router\n" );
    close_router( &router );   
    return 0;
}

void sighandler( int sig ) {
    m_exit();
    exit(0);
}

int main(int argc, char* argv[]) {
	int i;
	FILE *log;

	if( argc < 4 ) {
		fprintf( stderr, "Not enough arguments\n" );
		fprintf( stderr, "Usage: ./routed_LS <nodename> <logfile> <initfile>" );
		return -1;
	}

	if( argv[1][0] == 0 || argv[1][1] != 0 ) {
		fprintf( stderr, "Expected single char\n" );
		return -1;
	}
	
	if (!(log = fopen(argv[2], "w"))) {
		fprintf( stderr, "Failed to open log file %s.\n", argv[2]);
		return -1;
	}
	

	parse_router( argv[1][0], & router, argv[3] );
	router.log = log;

	/* Just print the entries for now */
	/*printf( "Neighbors:\n" );
	for( i = 0 ; i < router._m_num_neighbors; ++ i ) {
		neighbor_t* neighbor = & router._m_neighbors_table[i];
		printf( "[neighbor node_id=%c outgoing_tcp_port=%d dest_tcp_port=%d cost=%d serv_fd=%d sock_fd=%d]",
			neighbor->node_id, neighbor->outgoing_tcp_port, neighbor->dest_tcp_port,
			neighbor->cost, neighbor->serv_fd, neighbor->sock_fd );
		printf("\n");
	}
	*/

    signal( SIGINT, sighandler );

	fprintf( log, "Waiting For Neighbors ...\n" );
	wait_for_neighbors( & router );

	fprintf( log, "All Neighbors are Up!!!\n" );
	fprintf( log, "Connected Neighbors:\n" );
	for( i = 0 ; i < router._m_num_neighbors; ++ i ) {
		neighbor_t* neighbor = & router._m_neighbors_table[i];
		fprintf( log, "[neighbor node_id=%c outgoing_tcp_port=%d dest_tcp_port=%d cost=%d serv_fd=%d sock_fd=%d]",
			neighbor->node_id, neighbor->outgoing_tcp_port, neighbor->dest_tcp_port,
			neighbor->cost, neighbor->serv_fd, neighbor->sock_fd );
		fprintf( log, "\n");
	}

	fprintf( log, "\n");
    Router_Main( & router );
	sleep( 1000 );
    m_exit();

	return 0;
}
