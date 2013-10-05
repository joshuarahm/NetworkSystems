#ifndef _CHUNK_QUEUE_H
#define _CHUNK_QUEUE_H

#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>

#define IS_CLOSING 1

/* Synchronized blocking queue for passing chunks between threads. */
typedef struct {
	uint8_t *_m_data;
	uint32_t _m_len;
	uint8_t _m_flags;
} data_block_t;

#define QUEUE_CAPACITY 128

typedef struct {
	uint32_t _m_head;
	uint32_t _m_tail;
	uint32_t _m_size;

	pthread_mutex_t _m_mutex;

    pthread_cond_t  _m_read_cond; /* Blocks peek and pop */
    pthread_cond_t  _m_write_cond; /* Blocks push */

    data_block_t*   _m_data[ QUEUE_CAPACITY ];

} block_queue_t;

#define block_queue_is_empty( queue ) \
    ( (queue)->_m_size == 0 )

/* Initialize the block queue. */
void block_queue_init(block_queue_t *queue, uint32_t qsize);

/* Push a data block onto the queue. */
void block_queue_push_chunk(block_queue_t *queue, data_block_t *data);

/* Remove the top data block from the queue. */
int block_queue_pop_chunk(block_queue_t *queue);

/* Return the first data block, but do not delete it. */
data_block_t *block_queue_peek_chunk(block_queue_t *queue);

/* Frees internal queue resources */
void block_queue_free(block_queue_t *queue);

#endif

