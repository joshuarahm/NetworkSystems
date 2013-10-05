#include <stdio.h>
#include <errno.h>

#include <time.h>

#include "gbn_socket.h"

#define READ_SIZE (4096<<1)


FILE* logger;

//<server_ip_address> <server_port> <error_rate> <random_seed> <send_file> <send_log>
int main( int argc, char** argv ) {
    (void) argc;

    gbn_socket_t* sock;

	if (argc < 6) {
		fprintf("Usage:\n");
		fprintf("./client <server_ip_address> <server_port> <error_rate> <random_seed> <send_file> <send_log>\n");
		exit(1);
	}

	logger = fopen(argv[6], "w");
        
    init_net_lib( atof(argv[3]), atoi(argv[4]) );

	// 192.168.0.170
    sock = gbn_socket_open_client( argv[1], atoi(argv[2]) ) ;

    FILE* read = fopen( argv[5], "r" );

    if( ! read ) {
        perror( "Could not open file" );
        return errno;
    }

    char buffer[READ_SIZE];
    size_t bytes_read;

    while( (bytes_read = fread(buffer, 1, READ_SIZE, read)) > 0 ) {
        gbn_socket_write( sock, buffer, bytes_read );
    }
    
    gbn_socket_close( sock );
    return 0;
}
