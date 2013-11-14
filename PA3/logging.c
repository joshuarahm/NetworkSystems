#include "logging.h"
#include "router.h"

void write_routing_table(FILE* out, router_t *current_state) {
	int i;
	routing_entry_t *curs;

	fprintf(out, "Forwarding table for node %c\n", current_state->_m_id);
	for (i = 0; i < current_state->_m_num_destinations; i++) {
		curs = &current_state->_m_destinations[i];
		print_entry( out, current_state, curs );
		fprintf( out, "\n" );
	}
}

void print_entry( FILE* out, router_t *router, routing_entry_t* entry ) {
	neighbor_t* neighbor = Router_GetNeighborForRoutingEntry( router, entry );
	fprintf( out, "[routing_entry dest_id=%c cost=%d outgoing_port=%d dest_port=%d]",
		entry->dest_id, entry->cost,
		neighbor ? neighbor->outgoing_tcp_port : -1,
		neighbor ? neighbor->dest_tcp_port : -1 );
}
