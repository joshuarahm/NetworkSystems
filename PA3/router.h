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
typedef uint8_t node_t;

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
    /* The id of the destination, e.g. 'A', 'B' etc. */
	node_t dest_id;

    /* The index into the neighbors table of the 
     * gateway to this destination */
    uint8_t gateway_idx;

    /* The total cost to get to the destination */
	uint8_t cost;

    /* The last seen sequence number from the
     * destination */
	uint32_t seq_num;
} routing_entry_t;

typedef struct {
	uint16_t outgoing_tcp_port;
	uint16_t dest_tcp_port;

    SOCKET   serv_fd;
    SOCKET   sock_fd;
} neighbor_t;

typedef struct {
	uint8_t num_routers;
	uint8_t id[MAX_NUM_ROUTERS];
	uint32_t distmap[256];
} router_set_t;

#define MAX_NUM_ROUTERS 32
#define LS_PACKET_OVERHEAD 7
/*
 * A struct that defines a router.
 *
 * _m_id: the id of this router (private)
 */
typedef struct {
	uint8_t         _m_id;
	uint32_t		_m_seq_num;

    /* The array of destinations known about */
	uint8_t         _m_num_destinations;
	routing_entry_t _m_destinations[MAX_NUM_ROUTERS];
    /* This table contains indexes of the destinations according
     * to their IDs. So the routing_entry for A can be found
     * by doing _m_destinations[ _m_routing_table[ 'A' ] ] */
    int _m_routing_table[ 255 ];

    /* The array of neighbors */
	uint8_t         _m_num_neighbors;
	neighbor_t _m_neighbors_table[MAX_NUM_ROUTERS];
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
	uint8_t origin;
	uint8_t should_close;
	uint8_t num_entries;
	uint32_t seq_num;
	uint8_t dest_id[MAX_NUM_ROUTERS];
	uint8_t cost[MAX_NUM_ROUTERS];
} ls_packet;

routing_entry_t* Router_GetRoutingEntryForNode( node_t routing_node );

/* Read a router and it's starting table from the file */
int parse_router( uint8_t router_id, router_t* router, const char* filename );

/* Wait for the neighbors to come online */
void wait_for_neighbors( router_t* router );

void serialize(const ls_packet *packet, uint8_t *outbuf);

void deserialize(ls_packet *packet, const uint8_t *inbuf);

uint8_t *create_packet(router_t *router, uint8_t should_close);

/* Returns -1 when node id not found in routing table */
int32_t get_routing_index(router_t *router, uint8_t id);

uint8_t update_routing_table(router_t *router, ls_packet *packet);

#endif /* ROUTER_H_ */
