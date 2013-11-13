#ifndef _LOGGING_H_
#define _LOGGING_H_

#include "router.h"

#define print_entry( routing_entry ) \
	printf( "[routing_entry_t dest_id=%c, cost=%d, outgoing_tcp_port=%d, dest_tcp_port=%d]", \
		(routing_entry)->dest_id, \
		(routing_entry)->cost, \
		(routing_entry)->outgoing_tcp_port, \
		(routing_entry)->dest_tcp_port )


void write_routing_table(FILE* out, router_t *current_state);

#endif
