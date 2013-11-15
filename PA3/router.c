#include "router.h"

#include <memory.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 
#include <assert.h>
#include "debugprint.h"
#include <fcntl.h>

#include <pthread.h>
#include <sys/select.h>

int read_packet( int fd, ls_packet_t* packet, router_t* router ) {
    uint8_t input[255];
    int tmpfd;
    int i;

    if( read( fd, input, 1 ) < 1 ) {
        return -1;
    }

    i = read( fd, input + 1, input[0] );
    debug3( "Read %d bytes from socket\n", i );

    deserialize( packet, input );
    if( update_routing_table( router, packet ) ) {
        
        for( i = 0; i < router->_m_num_neighbors; ++ i ) {
            tmpfd = router->_m_neighbors_table[i].sock_fd;
            if( tmpfd != fd ) {
                write( tmpfd, input, input[0] + 1 );
            }
        }
    
    }

    return 0;
}

void broadcast_packet( router_t* router ) {
    uint8_t outbuf[255];
    uint8_t size = create_packet( router, 0, outbuf + 1 );
    outbuf[0] = size;
    int i;
    for( i = 0; i < router->_m_num_neighbors; ++ i ) {
        write( router->_m_neighbors_table[i].sock_fd, outbuf, size );
    }
}

void Router_Main( router_t* router ) {
    struct timeval timeout;

    fd_set select_set;
    ls_packet_t packet;
    int i;
    int fd;
    int rv;
    int max_fd = 0;

    while( 1 ) {
        FD_ZERO( &select_set );
        for( i = 0; i < router->_m_num_neighbors; ++ i ) {
            fd = router->_m_neighbors_table[i].sock_fd;
            if( fd > max_fd ) {
                max_fd = fd;
            }
            FD_CLR( fd, &select_set );
            FD_SET( fd, &select_set );
        }
        
        timeout.tv_sec = 5;
        timeout.tv_usec = 500000;
        rv = select( max_fd + 1, & select_set, NULL, NULL, &timeout );

        if( rv == 0 ) {
            debug1( "Select() timed out, sending packet\n" );
            broadcast_packet( router );
        } else {
    
            debug1( "Select() returned %d sockets ready for read\n", rv );
            for( i = 0; i < router->_m_num_neighbors; ++ i ) {
                fd = router->_m_neighbors_table[i].sock_fd;

                if( FD_ISSET( fd, &select_set ) ) {
                    debug1( "fd %d in set; reading packet\n", fd );
                    if( read_packet( fd, &packet, router ) < 0 ) {
                        debug1( "No data ready to read from socket WTF?!?!\n" );
                    }
                }
            }
    
        }
    }
}

int try_connect( uint16_t port ) {
	int sockfd = 0;
    struct sockaddr_in serv_addr; 

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Error : Could not create socket \n");
        return 0;
    } 

    memset(&serv_addr, 0, sizeof(serv_addr)); 

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port); 

    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) {
        printf("\n inet_pton error occured\n");
        return 0;
    } 

    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
       printf("Error: Connect failed on port %d\n", port);
       return 0;
    } 


    return sockfd;
}

/* If trying to connect to the neighbors failed,
 * then this function is called which waits for the
 * neighbors to connect to us.
 *
 * The file descriptor for the server
 * is stored in serv_fd and the accepted
 * client file descriptor is returned
 */
int listen_connect( uint16_t port, int* serv_fd ) {
    int listen_fd = 0;
    struct sockaddr_in serv_addr; 

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port); 

    if( bind(listen_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) ) {
        perror( "Error on bind" );
        return -1;
    }

    if( listen(listen_fd, 1) ) {
        perror( "Error on listen\n" );
        return -1;
    }

    *serv_fd = listen_fd;
    return accept(listen_fd, (struct sockaddr*)NULL, NULL); 
}

int parse_router( uint8_t router_id, router_t* router, const char* filename ) {
	FILE* file = fopen( filename, "r" );
	neighbor_t temp_entry;
	char tmp_id;
	int n_filled;
	int line = 0;
	/* The the cost as an int; we cant fill a byte
	 * directly */
	int cost;
	memset( router, 0, sizeof( router_t ) ); 
    memset( &temp_entry, 0, sizeof( neighbor_t ) );
    memset( &router->_m_routing_table, 0xFF, 255 ); 

	router->_m_id = router_id;

	if( ! file ) {
		return 1;
	}

	while( ( n_filled = fscanf( file, "<%c,%hu,%c,%hu,%d>", & tmp_id,
		& temp_entry.outgoing_tcp_port,
		& temp_entry.node_id,
		& temp_entry.dest_tcp_port,
		& cost ) ) != EOF ) {

		/* Read off newline (If there is one) */
		fgetc( file );

		if( n_filled < 5 ) {
			fprintf( stderr, "malformed file; line %d\n", line );
			return 2;
		}

		++ line;
		if( tmp_id == router_id ) {
			/* If this line is actually for this router we're configuring */
			temp_entry.cost = (uint8_t) cost;
			router->_m_neighbors_table[ router->_m_num_neighbors ++ ] = temp_entry;
		}
	}
	return 0;
}


uint8_t serialize(const ls_packet_t *packet, uint8_t *outbuf) {
	int i;
	outbuf[0] = packet->should_close;
	outbuf[1] = packet->num_entries;
	outbuf[2] = packet->origin;
	uint32_t *seq_num_ptr = (uint32_t*) outbuf+3;
	*seq_num_ptr = htonl(packet->seq_num);
	for (i = 0; i < packet->num_entries; i++) {
		outbuf[(2*i)+LS_PACKET_OVERHEAD] = packet->dest_id[i];
		outbuf[(2*i)+LS_PACKET_OVERHEAD+1] = packet->cost[i];
	}
	return LS_PACKET_OVERHEAD + 2*(packet->num_entries);
}

void deserialize(ls_packet_t *packet, const uint8_t *inbuf) {
	int i;
	packet->should_close = inbuf[0];
	packet->num_entries = inbuf[1];
	packet->origin = inbuf[2];
	debug4("Deserialized: should_close: %d, num_entries: %d, origin: %c\n", packet->should_close, packet->num_entries, packet->origin);
	uint32_t *seq_num_ptr = (uint32_t*) inbuf+3;
	*seq_num_ptr = htonl(packet->seq_num);
	for (i = 0; i < packet->num_entries; i++) {
		packet->dest_id[i] = inbuf[(2*i)+LS_PACKET_OVERHEAD];
		packet->cost[i] = inbuf[(2*i)+LS_PACKET_OVERHEAD+1];
	}
}

uint8_t create_packet(router_t *router, uint8_t should_close, uint8_t *outbuf) {
	int i;
	ls_packet_t tmp;
	tmp.should_close = should_close;
	tmp.num_entries = router->_m_num_neighbors;
	tmp.seq_num = ++(router->_m_seq_num);
	debug4("New: should_close: %d, num_entries: %d, origin: %c\n", packet->should_close, packet->num_entries, packet->origin);

	for (i = 0; i < tmp.num_entries; i++) {
		tmp.dest_id[i] = router->_m_neighbors_table[i].node_id;
	}

	return serialize(&tmp, outbuf);
}

routing_entry_t* Router_GetRoutingEntryForNode( router_t* router, const node_t routing_node ) {
    int dest = router->_m_routing_table[ routing_node ];

    if( dest > router->_m_num_destinations ) {
        return NULL;
    }

    return & router->_m_destinations[ dest ];
}

neighbor_t* Router_GetNeighborForRoutingEntry( router_t* router, const routing_entry_t* routing_entry ) {
    if( routing_entry->gateway_idx > router->_m_num_neighbors ) {
        return NULL;
    }

    return & router->_m_neighbors_table[ routing_entry->gateway_idx ];
}

/* Try to connect to a neighbor. This function
 * will set the sock_fd member of the neighbor
 * to the handle for communication with the neighbor */
static int set_blocking( int fd, int blocking ) {
   int flags = fcntl(fd, F_GETFL, 0);
   if (flags < 0) return 1;
   flags = blocking ? (flags&~O_NONBLOCK) : (flags|O_NONBLOCK);
   return (fcntl(fd, F_SETFL, flags) == 0) ? 0 : -1;
}
void* open_neighbor( neighbor_t* neighbor ) {
    printf( "Trying to connect to: %c\n", neighbor->node_id );
    SOCKET fd = try_connect( neighbor->dest_tcp_port );
    SOCKET serv = -1;

    if( fd == 0 ) {
        /* The host was not up yet */
        fd = listen_connect( neighbor->outgoing_tcp_port, & serv );
        printf( "Accepted connection from %c\n", neighbor->node_id );
    } else {
        printf( "Connected to %c\n", neighbor->node_id );
    }

    /* We don't want our sockets to block */
    set_blocking( fd, 0 );
    neighbor->sock_fd = fd;
    neighbor->serv_fd = serv;

    pthread_exit( NULL );
    return NULL;
}

void wait_for_neighbors( router_t* router ) {
    pthread_t threads[ MAX_NUM_ROUTERS ];
    size_t nthreads;
    nthreads = router->_m_num_neighbors;
    size_t i;

    for( i = 0; i < nthreads; ++ i ) {
        pthread_create( &threads[i], NULL, (void*(*)(void*))open_neighbor,
            & router->_m_neighbors_table[i] );
    }

    for( i = 0; i < nthreads; ++ i ) {
        pthread_join( threads[i], NULL );
    }
}

void close_router( router_t* router ) {
    int i;
    for( i = 0 ; i < router->_m_num_neighbors; ++ i ) {
        neighbor_t* neighbor = &router->_m_neighbors_table[i];

        if( close( neighbor->sock_fd ) ) {
            perror( "Error on close" );
        }

        if( neighbor->serv_fd > 0 ) {
            if( close( neighbor->serv_fd ) ) {
                perror( "Error on close" );
            }
        }
    }
}

uint8_t packet_has_update(ls_packet_t *orig, ls_packet_t *incoming) {
	uint8_t i;
	//p1->
	if (incoming->seq_num > orig->seq_num) {
		if (incoming->num_entries == orig->num_entries) {
			for (i = 0; i < incoming->num_entries; i++) {
				if (incoming->dest_id[i] == orig->dest_id[i] &&
						incoming->cost[i] == orig->cost[i]) {
					return 0; //Packet contains all the same entries
				}
				return 1; //Same number of entries, but different contents
			}
		}
		return 1; //The number of entries vary
	} 
	return 0; //Seq num was too old
}

uint8_t set_has_node(const ls_set_t *set, const node_t nodeid) {
	int i;
	for (i=0; i < set->num_routers; i++) {
		if (set->id[i] == nodeid)
			return 1;
	}
	return 0;
}

uint8_t get_neighbor_idx(router_t *router, const node_t nodeid) {
	int i;
	for (i=0; i < router->_m_num_neighbors; i++) {
		if (router->_m_neighbors_table[i].node_id == nodeid) {
			return i;
		}
	}
	assert(1==2); //This shouldn't occur D:
	return 255;
}

uint8_t get_gateway_idx(router_t *router, const ls_link_t *dest) {
	if (dest->src == router->_m_id) {
		//src is us, dest must be immediate neighbor
		return get_neighbor_idx(router, dest->dest);
	} else {
		//src is not us, use src to index into routing table and return src's gateway_idx
		return Router_GetRoutingEntryForNode(router, dest->src)->gateway_idx;
	}
}

void rebuild_routing_table(router_t *router) {
	int i;
	routing_entry_t entry;
	memset(router->_m_routing_table, 0xFF, 255);
	for (i=0; i < router->_m_num_destinations; i++) {
		entry = router->_m_destinations[i];
		router->_m_routing_table[entry.dest_id] = i;
	}
}

void set_shortest(ls_link_t *shortest, node_t src, node_t dest, uint16_t total_cost) {
	if (shortest->cost == -1) {
		shortest->src = src;
		shortest->dest = dest;
		shortest->cost = total_cost;
	} else {
		if (total_cost < shortest->cost ||
				(total_cost == shortest->cost && src < shortest->src) ||
				(total_cost == shortest->cost && src == shortest->src && dest < shortest->dest) ) {
			shortest->src = src;
			shortest->dest = dest;
			shortest->cost = total_cost;
		}
	}
}

void set_shortest_neighbor(router_t *router, ls_link_t *shortest, uint8_t neighbor_idx) {
	uint16_t node_cost = router->_m_neighbors_table[neighbor_idx].cost;
	node_t dest = router->_m_neighbors_table[neighbor_idx].node_id;
	node_t src = router->_m_id;
	set_shortest(shortest, src, dest, node_cost);
}

void set_shortest_distant(router_t *router, ls_link_t *shortest, node_t src, uint8_t router_idx) {
	uint16_t node_cost = Router_GetRoutingEntryForNode(router, src)->cost;
	node_t dest = router->_m_destinations[router_idx].dest_id;
	node_cost += router->_m_destinations[router_idx].cost;
	set_shortest(shortest, src, dest, node_cost);
}

uint8_t update_routing_table(router_t *router, ls_packet_t *packet) {
	int curr_neighbor, node_index;
	routing_entry_t *entry;
	ls_link_t shortest;
	shortest.cost=-1;
	
	ls_set_t current;
	assert(packet);
	if ((entry = Router_GetRoutingEntryForNode(router, packet->origin))) {
		if (packet_has_update(entry->packet, packet) && packet->origin != router->_m_id) {
			entry = Router_GetRoutingEntryForNode(router, packet->origin);
			memcpy(entry->packet, packet, sizeof(ls_packet_t));
			debug3("Processed packet with replacement information, nodeid = %d\n", packet->origin);
		} else {
			debug3("Processed packet with no useful information, discarding, nodeid = %d\n", packet->origin);
			return 0;
		}
	} else {
		debug3("added new packet to destinations, nodeid = %d\n", packet->origin);
		entry = &router->_m_destinations[router->_m_num_destinations++];
		entry->dest_id = packet->origin;
		entry->packet = malloc(sizeof(ls_packet_t));
		memcpy(entry->packet, packet, sizeof(ls_packet_t));
		rebuild_routing_table(router);
	}

	//Update routing table

	current.num_routers = 1;
	current.id[0] = router->_m_id;
	/* Add nodes to the set one at a time, until all destinations are in the set.
	   To determine which node to add, iterate through ever node in the current set and
	   find the link with the lowest cost. */
	while (current.num_routers < router->_m_num_destinations) { //Until all dests processed
		for (node_index = 0; node_index < current.num_routers; node_index++) { //Iterate through current set
			if (current.id[node_index] == router->_m_id) {
				//Check neighbors from the neighbors table
				for (curr_neighbor = 0; curr_neighbor < router->_m_num_neighbors; curr_neighbor++)
					set_shortest_neighbor(router, &shortest, curr_neighbor);
			} else {
				if (entry->packet) {
					//Check neighbors from routing entry in the routing table
					entry = Router_GetRoutingEntryForNode(router, current.id[node_index]);
					for (curr_neighbor = 0; curr_neighbor < entry->packet->num_entries; curr_neighbor++) //For current node's neighbors
						set_shortest_distant(router, &shortest, current.id[node_index], curr_neighbor);
				} else {
						debug3("Skipped parsing neighbors for an entry that does not have a packet stored: %c.\n", entry->dest_id);
				}
			}
		}


		current.id[current.num_routers++] = shortest.dest;
		entry = Router_GetRoutingEntryForNode(router, shortest.dest);
		if (!entry) {
			//Add entry into destinations
			entry = &router->_m_destinations[router->_m_num_destinations++];
			entry->dest_id = shortest.dest;
			entry->packet = NULL;
			rebuild_routing_table(router);
		}
		entry->gateway_idx = get_gateway_idx(router, &shortest);
		entry->cost = shortest.cost;
	}
	return 0;
}
