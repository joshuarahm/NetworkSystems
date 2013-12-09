#include "pa4str.h"
#include "trie.h"
#include "trie_set.h"
#include "client_state_machine.h"

#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <memory.h>
#include <pthread.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int parse_port_num( const char* str, uint16_t* into ) {
    uint16_t ret = 0;
    char ch ;

    while ( *str != 0 ) {
        ch = * str ;
        if ( ch >= 0x40 || ch < 0x30 ) {
            return 0 ;
        }
        ret *= 10 ;
        ret += ch - 0x30 ;
        str ++ ;
    }

    * into = ret ;
    return 1 ;
}

int peer_socket() {
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP );   
	struct sockaddr_in peer_sock;

	memset(&peer_sock, 0, sizeof(struct sockaddr_in)) ;
	peer_sock.sin_family = AF_INET ;
	peer_sock.sin_addr.s_addr = htonl(INADDR_ANY);
	peer_sock.sin_port = htons(0); //Any port

	if (bind(sock, (struct sockaddr*) &peer_sock, sizeof(peer_sock))) {
		fprintf(stderr, "Failed to bind.\n");
		return -1;
	}

	if ( listen(sock, 1)) {
		fprintf(stderr, "Failed to listen.\n");
		return -1;
	}

	return sock;
}

void peer_listener(int *fd) {
	int peer_sock = *fd, peer, bytes_read, i;
	uint32_t f_size;
	FILE *tmp;
	char fname[8192], inbuf[4096];
	while (1) {
		peer = accept(peer_sock, (struct sockaddr*)NULL, NULL);
		if (peer <= 0) {
			fprintf(stderr, "Failed to accept()\n");
			return;
		}

		for (i=0; i < 8191; i++) {
			fname[i] = read(peer, fname+i, 1);
			if (fname[i] == '\00')
				break;
		}
		fname[i] = '\00';
		struct stat fstat;
		if (stat(fname, &fstat)) {
			f_size = htonl(fstat.st_size);
			write(peer, &f_size, sizeof(uint32_t));
			tmp = fopen(fname, "r");
			while ((bytes_read = fread(inbuf, 4096, sizeof(char), tmp))) {
				write(peer, inbuf, bytes_read);
			}
			fclose(tmp);
			close(peer);
		} else {
			fprintf(stderr, "Could not open file: %s\n", fname);
			close(peer);
			continue;
		}
	}
}

void start_listener(int fd) {
	int *listen_fd = malloc(sizeof(int));
	*listen_fd = fd;
	pthread_t listener;
	pthread_create(&listener, NULL, (void*(*)(void*)) peer_listener, listen_fd);
}

int connect_client( const char* hostname, uint16_t port ) {
    int sock = socket( PF_INET, SOCK_STREAM, IPPROTO_TCP ) ;   
    struct hostent *he ;
    struct sockaddr_in server ;

    if ( sock > 0 ) {

        if( ( he = gethostbyname( hostname ) ) == NULL ) {
            return 0 ;
        }

        memset( & server, 0, sizeof( struct sockaddr_in ) ) ;
        memcpy( & server.sin_addr.s_addr, he->h_addr, he->h_length ) ;

        server.sin_family = AF_INET ;
        server.sin_port = htons( port ) ;

        if( connect( sock, (struct sockaddr *) & server, sizeof( server ) ) < 0 ) {
            return 0 ;
        }
    }

    return sock ;
}

int main( int argc, char** argv ) {
    if ( argc < 3 ) {
        fprintf( stderr, "must supply a server address and port\n" ) ;
        return 1 ;
    }
    
    uint16_t port ;
    if( ! parse_port_num( argv[2], & port ) ) {
        fprintf( stderr, "Must supply valid port number\n" ) ;
        return 2 ;
    }
    
    /* open a connection to the server */
    int fd = connect_client( argv[1], port ) ;
    if ( fd == 0 ) {
        perror( "Unable to open client" ) ;
        return 3 ;
    }

	/* open a listening socket for peers to connect to */
	int peer_sock = peer_socket();

    client_t cli ;
    cli.fd = fd ;
	cli.peer_fd = peer_sock;
	start_listener(peer_sock);
    run_client( & cli ) ;

    return 0 ;
}
