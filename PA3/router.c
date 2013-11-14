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

    if(inet_pton(AF_INET, "localhost", &serv_addr.sin_addr)<=0) {
        printf("\n inet_pton error occured\n");
        return 0;
    } 

    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
       printf("\n Error : Connect Failed \n");
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
	routing_entry_t temp_entry;
	char tmp_id;
	int n_filled;
	int line = 0;
	/* The the cost as an int; we cant fill a byte
	 * directly */
	int cost;
	memset( router, 0, sizeof( * router ) ); 

	router->_m_id = router_id;

	if( ! file ) {
		return 1;
	}

	while( ( n_filled = fscanf( file, "<%c,%hu,%c,%hu,%d>", & tmp_id,
		& temp_entry.outgoing_tcp_port,
		& temp_entry.dest_id,
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
			router->_m_neighbors_table[ router->_m_num_routers ++ ] = temp_entry;
		}
	}
	return 0;
}


void serialize(const ls_packet *packet, uint8_t *outbuf) {
	int i;
	outbuf[0] = packet->should_close;
	outbuf[1] = packet->num_entries;
	uint32_t *seq_num_ptr = (uint32_t*) outbuf+2;
	*seq_num_ptr = htonl(packet->seq_num);
	for (i = 0; i < packet->num_entries; i++) {
		outbuf[(2*i)+6] = packet->dest_id[i];
		outbuf[(2*i)+6+1] = packet->cost[i];
	}
}

void deserialize(ls_packet *packet, const uint8_t *inbuf) {
	int i;
	packet->should_close = inbuf[0];
	packet->num_entries = inbuf[1];
	uint32_t *seq_num_ptr = (uint32_t*) inbuf+2;
	*seq_num_ptr = htonl(packet->seq_num);
	for (i = 0; i < packet->num_entries; i++) {
		packet->dest_id[i] = inbuf[(2*i)+6];
		packet->cost[i] = inbuf[(2*i)+6+1];
	}
}

void create_packet(router_t *router, uint8_t should_close) {
	int i;
	ls_packet tmp;
	tmp.should_close = should_close;
	tmp.num_entries = router->_m_num_neighbors;
	tmp.seq_num = ++(router->_m_seq_num);
	for (i = 0; i < tmp.num_entries; i++) {
		tmp.dest_id[i] = router->_m_neighbors_table[i].dest_id;
	}
}

/* Try to connect to a neighbor. This function
 * will set the sock_fd member of the neighbor
 * to the handle for communication with the neighbor */
void* open_neighbor( routing_entry_t* neighbor ) {
    SOCKET fd = try_connect( neighbor->dest_tcp_port );
    SOCKET serv = -1;;
    if( fd == 0 ) {
        /* The host was not up yet */
        fd = listen_connect( neighbor->outgoing_tcp_port, & serv );
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
