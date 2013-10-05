#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include "blockqueue.h"

#include "debugprint.h"

void block_queue_init(block_queue_t *queue, uint32_t qsize) {
    (void) qsize;
	queue->_m_head = queue->_m_tail = queue->_m_size = 0;

    pthread_cond_init( & queue->_m_write_cond, NULL );
    pthread_cond_init( & queue->_m_read_cond, NULL );
	pthread_mutex_init(& queue->_m_mutex, NULL);
}

void block_queue_push_chunk(block_queue_t *queue, data_block_t *data) {
    debug2( "[Queue %p] data block pushed to queue: %p\n", queue, data );
	pthread_mutex_lock( & queue->_m_mutex );

    if( queue->_m_size == QUEUE_CAPACITY ) {
        pthread_cond_wait( & queue->_m_write_cond, & queue->_m_mutex );
    }

	queue->_m_data[queue->_m_head] = data;
	queue->_m_head++;
	queue->_m_head %= QUEUE_CAPACITY;
	queue->_m_size++;

	pthread_mutex_unlock( & queue->_m_mutex );
    pthread_cond_signal( & queue->_m_read_cond );
}

int block_queue_pop_chunk(block_queue_t *queue) {
    debug2( "[Queue %p] data block popped from queue\n", queue );
    if( queue->_m_size == 0 )
        return -1;

	pthread_mutex_lock( &queue->_m_mutex );;

	queue->_m_tail++;
	queue->_m_tail %= QUEUE_CAPACITY;
	queue->_m_size--;

    pthread_cond_signal( & queue->_m_write_cond );

	pthread_mutex_unlock(&queue->_m_mutex);
	return 0;
}


data_block_t *block_queue_peek_chunk(block_queue_t *queue) {
	data_block_t *ret;
	pthread_mutex_lock( & queue->_m_mutex );

    if( queue->_m_size == 0 ) {
        pthread_cond_wait( & queue->_m_read_cond, & queue->_m_mutex );
    }

	ret = queue->_m_data[queue->_m_tail];

	pthread_mutex_unlock( & queue->_m_mutex );
    debug2( "[Queue] data block peeked: %p\n", ret );
	return ret;
}

void block_queue_free(block_queue_t *queue) {
    pthread_cond_destroy( & queue->_m_read_cond );
    pthread_cond_destroy( & queue->_m_write_cond );
	pthread_mutex_destroy( & queue->_m_mutex );
}
