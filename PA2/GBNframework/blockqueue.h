#ifndef _CHUNK_QUEUE_H
#define _CHUNK_QUEUE_H

#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>


/* Synchronized blocking queue for passing chunks between threads. */
typedef struct {
	uint8_t *_m_data;
	uint32_t _m_len;
} data_block_t;

typedef struct {
	data_block_t **_m_data;
	uint32_t _m_head;
	uint32_t  _m_tail;
	uint32_t _m_size;
	sem_t *_m_write_sem, *_m_read_sem;
	pthread_mutex_t* _m_modify_lock;
} block_queue_t;

/* Initialize the block queue. */
void queue_init(block_queue_t *queue, uint32_t qsize);

/* Push a data block onto the queue. */
void push_chunk(block_queue_t *queue, data_block_t *data);

/* Remove the top data block from the queue. */
data_block_t *pop_chunk(block_queue_t *queue);

/* Return the first data block, but do not delete it. */
data_block_t *peek_chunk(block_queue_t *queue);

/* Frees internal queue resources */
void queue_free(block_queue_t *queue);

#endif

