#include "router.h"

#include <memory.h>

int parse_router( uint8_t router_id, router_t* router, const char* filename ) {
	FILE* file = fopen( filename, "r" );
	routing_entry_t temp_entry;
	char tmp_id;
	int n_filled;
	int line = 0;
	/* The the cost as an int; we cant fill a byte
	 * directly */
	int cost;
	memset( router, 0, sizeof( * router ) ); 

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
		if( tmp_id == router_id ) {
			/* If this line is actually for this router we're configuring */
			temp_entry.cost = (uint8_t) cost;
			router->_m_table[ router->_m_num_routers ++ ] = temp_entry;
		}
	}
	return 0;
}


void serialize(const routing_packet_t *packet, uint8_t *outbuf) {
	int i;
	outbuf[0] = packet->should_close;
	outbuf[1] = packet->num_entries;
	for (i = 0; i < packet->num_entries; i++) {
		outbuf[ (2*i)+2 ] = packet->dest_id[i];
		outbuf[ (2*i)+2+1 ] = packet->cost[i];
	}
}

void deserialize(routing_packet_t *packet, const uint8_t *inbuf) {
	int i;
	packet->should_close = inbuf[0];
	packet->num_entries = inbuf[1];
	for (i = 0; i < packet->num_entries; i++) {
		packet->dest_id[i] = inbuf[(2*i)+2];
		packet->cost[i] = inbuf[(2*i)+2+1];
	}
}
