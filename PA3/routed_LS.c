#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>

#include "router.h"


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
	for( i = 0 ; i < router._m_num_routers; ++ i ) {
		print_entry( &router._m_table[i] );
		printf("\n");
	}

	return 0;
}
