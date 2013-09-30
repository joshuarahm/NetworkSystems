#ifndef GBN_SOCKET_H_
#define GBN_SOCKET_H_

/*
 * Author: jrahm
 * created: 2013/09/29
 * gbn_socket.h: <description>
 */

#include <inttypes.h>

#include <pthread.h>

#define DEFAULT_QUEUE_SIZE 6

typedef int SOCKET;

typedef enum {
      gbn_packet_type_ack
    , gbn_packet_Type_data
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
    gbn_packet_t _m_packet_buffer[ DEFAULT_QUEUE_SIZE ];
} gbn_window_t;

typedef struct {
    int sockfd;
    pthread_t _m_read_thread;
} gbn_socket_t;

/* Opens a socket to the specified host name
 * and port number. Returns 0 on success
 * otherwise an error code is returned */
int gbn_socket_open  ( gbn_socket_t* socket, char* hostname, uint16_t port );

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

#endif /* GBN_SOCKET_H_ */
