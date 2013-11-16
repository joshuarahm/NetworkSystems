#ifndef ROUTER_H_
#define ROUTER_H_

/*
 * Author: jrahm / zanders
 * created: 2013/11/12
 * router.h: <description>
 */

#include <inttypes.h>
#include <stdio.h>

#define MAX_NUM_ROUTERS 32
#define LS_PACKET_OVERHEAD 7

typedef int SOCKET;
typedef uint8_t node_t;

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
} ls_packet_t;

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
	int16_t cost;

	/* The last packet we have seen that originated from dest_id */
	ls_packet_t *packet;
} routing_entry_t;

typedef struct {
    /* The id of the neighbor */
    node_t   node_id;

	uint16_t outgoing_tcp_port;
	uint16_t dest_tcp_port;

    /* edge cost for this neighbor */
    uint8_t  cost;

    SOCKET   serv_fd;
    SOCKET   sock_fd;
} neighbor_t;

typedef struct {
	uint8_t num_routers;
	uint8_t id[MAX_NUM_ROUTERS];
} ls_set_t;

typedef struct {
	node_t src;
	node_t dest;
	int16_t cost;
} ls_link_t;

/*
 * A struct that defines a router.
 *
 * _m_id: the id of this router (private)
 */
typedef struct {
	uint8_t         _m_id;
	uint32_t		_m_seq_num;

	//File handle to log to
	FILE *log;

    /* The array of destinations known about */
	uint8_t         _m_num_destinations;
	routing_entry_t _m_destinations[MAX_NUM_ROUTERS];

    /* This table contains indexes of the destinations according
     * to their IDs. So the routing_entry for A can be found
     * by doing _m_destinations[ _m_routing_table[ 'A' ] ] */
    uint8_t    _m_routing_table[ 255 ];

    /* The array of neighbors */
	uint8_t    _m_num_neighbors;
	neighbor_t _m_neighbors_table[MAX_NUM_ROUTERS];
} router_t;


/* Returns a routing entry for the node given */
routing_entry_t* Router_GetRoutingEntryForNode( router_t* router, const node_t routing_node );

/* Returns the neighbor that will route */
neighbor_t* Router_GetNeighborForRoutingEntry( router_t* router, const routing_entry_t* routing_entry );

void Router_Main( router_t* router );

/* Read a router and it's starting table from the file */
int parse_router( uint8_t router_id, router_t* router, const char* filename );

/* Wait for the neighbors to come online */
void wait_for_neighbors( router_t* router );

uint8_t serialize(const ls_packet_t *packet, uint8_t *outbuf);

void deserialize(ls_packet_t *packet, const uint8_t *inbuf);

/* Creates an LS packet from the router's current neighbors. */
uint8_t create_packet(router_t *router, uint8_t should_close, uint8_t *outbuf);

void close_router( router_t *router );

/* Returns 1 if the packet 'incoming' should/needs to be processed in comparison to the packet 'orig' */
uint8_t packet_has_update(ls_packet_t *orig, ls_packet_t *incoming);

/* Returns 1 if the given set contains the given nodeid */
uint8_t set_has_node(const ls_set_t *set, const node_t nodeid);

/* Manually search the neighbor's table for a given node. Returns an index into _m_neighbors_table */
uint8_t get_neighbor_idx(router_t *router, const node_t nodeid);

/* Given a link ( X->Y, 5) determine what the gateway_idx will be */
uint8_t get_gateway_idx(router_t *router, const ls_link_t *dest);

void rebuild_routing_table(router_t *router);

/* Use only for check distance from self to an immediate neighbor */
void set_shortest_neighbor(router_t *router, ls_link_t *shortest, uint8_t neighbor_idx);

/* Use only for checking distance from a non-self router to some other router */
void set_shortest_distant(router_t *router, ls_link_t *shortest, node_t src, ls_packet_t *packet, uint8_t router_idx);

void set_shortest(ls_link_t *shortest, node_t src, node_t dest, uint16_t total_cost); 

/* Returns 1 if an actual update occurred. */
uint8_t update_routing_table(router_t *router, ls_packet_t *incoming);

#endif /* ROUTER_H_ */
