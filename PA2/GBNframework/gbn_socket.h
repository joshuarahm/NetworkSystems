#ifndef GBN_SOCKET_H_
#define GBN_SOCKET_H_

/*
 * Author: jrahm
 * created: 2013/09/29
 * gbn_socket.h: <description>
 */

#include <inttypes.h>

#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "blockqueue.h"

#define DEFAULT_QUEUE_SIZE 6

typedef int SOCKET;

typedef int bool;
#define false 0
#define true  1

typedef enum {
      gbn_packet_type_ack
    , gbn_packet_type_data
    , gbn_packet_type_uninitialized
} gbn_packet_type_e;

typedef struct {
    gbn_packet_type_e _m_type;

    /* Sequence number */
    uint32_t         _m_seq_num;
        
    /* Number of bytes in
     * the array _m_payload */
    uint32_t         _m_size;
    
    /* Pointer to this packet's
     * payload */
    uint8_t*         _m_payload;
} gbn_packet_t;

typedef struct {
    /* The head pointer */
    uint8_t _m_head;

    /* The tail of the queue */
    uint8_t _m_tail;

    /* The number of elements in the queue */
    uint8_t _m_size;

    /* The mutex used to synchronize access to
     * the queue */
    pthread_mutex_t _m_mutex;

    /* The buffer of the queue */
    uint32_t     _m_recv_counter;
    gbn_packet_t _m_packet_buffer[ DEFAULT_QUEUE_SIZE ];
} gbn_window_t;

typedef struct {
    /* The filedescriptor to the socket
     * that we are listenting on */
    SOCKET _m_sockfd;

    /* The reader thread for this socket */
    pthread_t _m_read_thread;
    pthread_t _m_write_thread;

    pthread_mutex_t _m_mutex;

    /* Is the buffer which the higher
     * layers write to and is eventually
     * digested by the socket */
    block_queue_t _m_sending_buffer;
    block_queue_t _m_receive_buffer;

    /* The pointer into the first block that
     * tells how much a partial first block has
     * been read */
    uint32_t      _m_current_block_ptr;

    /* The condition the writing thread
     * waits for */
    pthread_cond_t _m_wait_for_ack;

    gbn_window_t _m_sending_window;
    gbn_window_t _m_receiving_window;

    struct sockaddr_in  _m_to_addr;
} gbn_socket_t;

/* Opens a socket to the specified host name
 * and port number. Returns 0 on success
 * otherwise an error code is returned */
gbn_socket_t* gbn_socket_open_client  ( const char* hostname, uint16_t port );

gbn_socket_t* gbn_socket_open_server( uint16_t port );

/* Writes an array of bytes to the socket.
 * The write will block until all bytes are
 * successfully queued for sending */
int gbn_socket_write ( gbn_socket_t* socket, const char* bytes, uint32_t len );

/*
 * Reads up to `len` bytes and blocks unitl all bytes
 * are read
 */
int gbn_socket_read  ( gbn_socket_t* socket, char* bytes, uint32_t len );

/*
 * Closes the connection to the socket
 */
int gbn_socket_close ( gbn_socket_t* socket );

/* The main reading thread of execution for
 * the socket */
void gbn_socket_read_thread_main( gbn_socket_t* sock );

void gbn_socket_write_thread_main( gbn_socket_t* sock );

#endif /* GBN_SOCKET_H_ */
