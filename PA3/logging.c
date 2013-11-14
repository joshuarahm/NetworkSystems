#include "logging.h"
#include "router.h"

void write_routing_table(FILE* out, router_t *current_state) {
	int i;
	routing_entry_t *curs;
	fprintf(out, "Forwarding table for node %c\n", current_state->_m_id);
	for (i = 0; i < current_state->_m_num_routers; i++) {
		curs = &current_state->_m_table[i];
		print_entry(curs);
	}
}
