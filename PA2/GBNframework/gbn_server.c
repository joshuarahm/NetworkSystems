#include <stdio.h>
#include "gbn_socket.h"

#define DEFAULT_BUF_SIZE 4096

int main( int argc, char** argv ) {
    (void) argc;

    gbn_socket_t* sock;
	uint8_t buf[DEFAULT_BUF_SIZE];

    if( ! argv[1] ) {
        fprintf( stderr, "File argument required\n" );
        return 1;
    }

    if( ! argv[2] ) {
        fprintf( stderr, "Host port required" );
        return 1;
    }

    sock = gbn_socket_open_server( atoi( argv[2] ) ) ;
    init_net_lib( 0.01, time(NULL) );

    if( sock == NULL ) {
        perror( "Unable to open socket on port 5432\n" );
        return 1;
    }
    FILE* read = fopen(argv[1], "w");

    uint32_t bytes_read=1;

    while ( (bytes_read = gbn_socket_read(sock, (char*)buf, DEFAULT_BUF_SIZE))) {
		printf("Read %d bytes.\n", bytes_read);
        
		fwrite(buf, 1, bytes_read, read);
		//for (tmp=0; tmp < bytes_read; tmp++) {
		//	printf("%02x ", buf[tmp]);
		//}
		//printf("\n");
    }

	fclose(read);
    
    gbn_socket_close(sock);
    return 0;
}
