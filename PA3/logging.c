#include "logging.h"
#include "router.h"

void write_routing_table(FILE* out, router_t *current_state) {
	int i;
	routing_entry_t *curs;
	fprintf(out, "Forwarding table for node %c\n", current_state->_m_id);
	for (i = 0; i < current_state->_m_num_routers; i++) {
		curs = &current_state->_m_table[i];
		fprintf(out, "Destination: %c, Cost: %d, Outgoing TCP Port: %d, Destination TCP Port: %d\n", curs->dest_id, curs->cost, curs->outgoing_tcp_port, curs->dest_tcp_port);
	}
}
