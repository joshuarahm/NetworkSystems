#include "router.h"

int parse_router( uint8_t router_id, router_t* router, const char* filename ) {
	FILE* file = fopen( filename, "r" );
	routing_entry_t temp_entry;
	char tmp_id;
	int n_filled;
	int line = 0;
	/* The the cost as an int; we cant fill a byte
	 * directly */
	int cost;

	router->_m_id = router_id;

	if( ! file ) {
		return 1;
	}

	while( ( n_filled = fscanf( file, "<%c,%hu,%c,%hu,%d>", & tmp_id,
		& temp_entry.outgoing_tcp_port,
		& temp_entry.dest_id,
		& temp_entry.dest_tcp_port,
		& cost ) ) ) {

		if( n_filled < 5 ) {
			fprintf( stderr, "malformed file; line %d\n", line );
			return 2;
		}

		++ line;
	}

	return 0;
}
