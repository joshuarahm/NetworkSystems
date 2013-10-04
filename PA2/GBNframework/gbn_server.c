#include <stdio.h>
#include "gbn_socket.h"

#define DEFAULT_BUF_SIZE 4096

int main( int argc, char** argv ) {
    gbn_socket_t* sock;
	uint8_t buf[DEFAULT_BUF_SIZE];

    if(!argv[1]) {
        fprintf( stderr, "File argument required\n" );
        return 1;
    }

    sock = gbn_socket_open_server(5432) ;
    //FILE* read = fopen(argv[1], "w");

	uint32_t tmp;

    while(true) {
        gbn_socket_read(sock, buf, DEFAULT_BUF_SIZE);
		for (tmp=0; tmp<DEFAULT_BUF_SIZE; tmp++) {
			printf("%x", buf[tmp]);
		}
		printf("\n");
    }
    
    gbn_socket_close(sock);
    return 0;
}
