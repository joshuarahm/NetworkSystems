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
	uint32_t seq_num;

	uint16_t outgoing_tcp_port;
	uint16_t dest_tcp_port;
    SOCKET   serv_fd;
    SOCKET   sock_fd;
} routing_entry_t;

#define MAX_NUM_ROUTERS 32
/*
 * A struct that defines a router.
 *
 * _m_id: the id of this router (private)
 */
typedef struct {
	uint8_t         _m_id;
	uint32_t		_m_seq_num;
	uint8_t         _m_num_routers;
	routing_entry_t _m_routing_table[MAX_NUM_ROUTERS];
	uint8_t         _m_num_neighbors;
	routing_entry_t _m_neighbors_table[MAX_NUM_ROUTERS];
} router_t;


/* 
 * A struct that defines the packet header used to communicate
 * between link-state routers.
 * should_close: A boolean to tell the receiver to close
 * num_entries: The number of entries in the packet
 * dest_id[]: List of destinations.
 * cost[]: List of costs for the above destinations.
 */
typedef struct {
	uint8_t should_close;
	uint8_t num_entries;
	uint32_t seq_num;
	uint8_t dest_id[MAX_NUM_ROUTERS];
	uint8_t cost[MAX_NUM_ROUTERS];
} ls_packet;

/* Read a router and it's starting table from the file */
int parse_router( uint8_t router_id, router_t* router, const char* filename );

/* Wait for the neighbors to come online */
void wait_for_neighbors( router_t* router );

void serialize(const ls_packet *packet, uint8_t *outbuf);

void deserialize(ls_packet *packet, const uint8_t *inbuf);

void create_packet(router_t *router, uint8_t should_close);

#endif /* ROUTER_H_ */
