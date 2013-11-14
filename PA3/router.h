#ifndef ROUTER_H_
#define ROUTER_H_

/*
 * Author: jrahm / zanders
 * created: 2013/11/12
 * router.h: <description>
 */

#include <inttypes.h>
#include <stdio.h>

typedef int SOCKET;

/*
 * An entry in the routing table
 * of each router.
 *
 * dest_id: the id of the node it's connected to
 * cost:    the cost of the link
 *
 * outgoing_tcp_port: the port we are listening
 * dest_tcp_port:     the port the other guy is listening to
 */
typedef struct {
	uint8_t dest_id;
	uint8_t cost;

	uint16_t outgoing_tcp_port;
	uint16_t dest_tcp_port;
} routing_entry_t;

#define MAX_NUM_ROUTERS 32
/*
 * A struct that defines a router.
 *
 * _m_id: the id of this router (private)
 */
typedef struct {
	uint8_t _m_id;
	uint8_t         _m_num_routers;
	routing_entry_t _m_table[MAX_NUM_ROUTERS];
} router_t;

#define print_entry( routing_entry ) \
	printf( "[routing_entry_t dest_id=%c, cost=%d, outgoing_tcp_port=%d, dest_tcp_port=%d]", \
		(routing_entry)->dest_id, \
		(routing_entry)->cost, \
		(routing_entry)->outgoing_tcp_port, \
		(routing_entry)->dest_tcp_port )

/* Read a router and it's starting table from the file */
int parse_router( uint8_t router_id, router_t* router, const char* filename );

/* Wait for the neighbors to come online */
void wait_for_neighbors( router_t* router );

#endif /* ROUTER_H_ */
