#include <stdio.h>
#include "gbn_socket.h"

#define DEFAULT_BUF_SIZE 4096

FILE *logger;

//<server_port> <error_rate> <random_seed> <output_file> <receive_log>
int main( int argc, char** argv ) {
    (void) argc;

    gbn_socket_t* sock;
	uint8_t buf[DEFAULT_BUF_SIZE];

	if (argc < 5) {
		fprintf( stderr, "Usage:\n");
		fprintf( stderr, "./server <server_port> <error_rate> <random_seed> <output_file> <receive_log>\n");
		exit(1);
	}

	logger = fopen(argv[5], "w");

    sock = gbn_socket_open_server( atoi( argv[1] ) ) ;
    init_net_lib( atof(argv[2]), atoi(argv[3]));

    if( sock == NULL ) {
        perror( "Unable to open socket on port 5432\n" );
        return 1;
    }
    FILE* read = fopen(argv[4], "w");

    uint32_t bytes_read=1;
    uint32_t total_size=0;
    while ( (bytes_read = gbn_socket_read(sock, (char*)buf, DEFAULT_BUF_SIZE))) {
        total_size += bytes_read;
		printf("Read %d bytes.\r", total_size);
        
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
