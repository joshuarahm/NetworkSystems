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

#include <pthread.h>

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

    bind(listen_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 
    listen(listen_fd, 1); 

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


void serialize(const ls_packet_t *packet, uint8_t *outbuf) {
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
}

void deserialize(ls_packet_t *packet, const uint8_t *inbuf) {
	int i;
	packet->should_close = inbuf[0];
	packet->num_entries = inbuf[1];
	packet->origin = inbuf[2];
	uint32_t *seq_num_ptr = (uint32_t*) inbuf+3;
	*seq_num_ptr = htonl(packet->seq_num);
	for (i = 0; i < packet->num_entries; i++) {
		packet->dest_id[i] = inbuf[(2*i)+LS_PACKET_OVERHEAD];
		packet->cost[i] = inbuf[(2*i)+LS_PACKET_OVERHEAD+1];
	}
}

uint8_t *create_packet(router_t *router, uint8_t should_close) {
	int i;
	ls_packet_t tmp;
	uint8_t *outbuf;
	tmp.should_close = should_close;
	tmp.num_entries = router->_m_num_neighbors;
	tmp.seq_num = ++(router->_m_seq_num);
	outbuf = malloc(LS_PACKET_OVERHEAD + 2*tmp.num_entries);
	for (i = 0; i < tmp.num_entries; i++) {
		tmp.dest_id[i] = router->_m_neighbors_table[i].node_id;
	}

	serialize(&tmp, outbuf);

	return outbuf;
}

routing_entry_t* Router_GetRoutingEntryForNode( router_t* router, node_t routing_node ) {
    int dest = router->_m_routing_table[ routing_node ];

    if( dest > router->_m_num_destinations ) {
        return NULL;
    }

    return & router->_m_destinations[ dest ];
}

neighbor_t* Router_GetNeighborForRoutingEntry( router_t* router, routing_entry_t* routing_entry ) {
    if( routing_entry->gateway_idx > router->_m_num_neighbors ) {
        return NULL;
    }

    return & router->_m_neighbors_table[ routing_entry->gateway_idx ];
}

/* Try to connect to a neighbor. This function
 * will set the sock_fd member of the neighbor
 * to the handle for communication with the neighbor */
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

        if( neighbor->serv_fd > 0 ) {
            close( neighbor->serv_fd );
        }

        close( neighbor->sock_fd );
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

uint8_t set_has_node(ls_set_t *set, node_t nodeid) {
	int i;
	for (i=0; i < set->num_routers; i++) {
		if (set->id[i] == nodeid)
			return 1;
	}
	return 0;
}

uint8_t update_routing_table(router_t *router, ls_packet_t *packet) {
	int curr_node_index;
	routing_entry_t *old_entry, *new_entry;
	
	ls_set_t current;
	if ((old_entry = Router_GetRoutingEntryForNode(router, packet->origin))) {
		if (!packet_has_update(old_entry->packet, packet)) {
			return 0;
		} else {
			//Update existing info
		}
	} else {
		new_entry = &router->_m_destinations[router->_m_num_destinations++];
		new_entry->dest_id = packet->origin;
		new_entry->packet = malloc(sizeof(ls_packet_t));
	}
	//Packet must contain new info


	current.num_routers = 1;
	current.id[0] = router->_m_id;
	for (current.num_routers = 1; current.num_routers < router->_m_num_destinations+1; current.num_routers++) {
		for (curr_node_index = 0; curr_node_index < current.num_routers; curr_node_index++) {
			 if (current.id[curr_node_index] == router->_m_id) {
				 //We don't have an LSP packet for ourself, we instead check our neighbor costs
			 } else {
				 //Everyone else should have an entry in the routing table

			 }
		}
	}
	return 0;
}
