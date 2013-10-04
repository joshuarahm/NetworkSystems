#include <stdio.h>
#include "gbn_socket.h"

#define DEFAULT_BUF_SIZE 4096

int main( int argc, char** argv ) {
    (void) argc;

    gbn_socket_t* sock;
	uint8_t buf[DEFAULT_BUF_SIZE];

    if(!argv[1]) {
        fprintf( stderr, "File argument required\n" );
        return 1;
    }

    sock = gbn_socket_open_server(5432) ;

    if( sock == NULL ) {
        perror( "Unable to open socket on port 5432\n" );
        return 1;
    }
    //FILE* read = fopen(argv[1], "w");

	uint32_t tmp;
    uint32_t bytes_read;

    while(true) {
        bytes_read = gbn_socket_read(sock, (char*)buf, DEFAULT_BUF_SIZE);
		for (tmp=0; tmp < bytes_read; tmp++) {
			printf("%02x ", buf[tmp]);
		}
		printf("\n");
    }
    
    gbn_socket_close(sock);
    return 0;
}
