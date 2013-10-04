#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include "blockqueue.h"

void block_queue_init(block_queue_t *queue, uint32_t qsize) {
	if (queue->_m_data)
		free(queue->_m_data);
	queue->_m_size = qsize;
	queue->_m_head = 0;
	queue->_m_tail = 0;

	queue->_m_data = malloc(sizeof(data_block_t)*qsize);
	queue->_m_write_sem = malloc(sizeof(sem_t));
	queue->_m_read_sem = malloc(sizeof(sem_t));
	queue->_m_modify_lock = malloc(sizeof(pthread_mutex_t));
	
	sem_init(queue->_m_write_sem, 0, queue->_m_size);
	sem_init(queue->_m_read_sem, 0, 0);
	pthread_mutex_init(queue->_m_modify_lock, NULL);
}

void block_queue_push_chunk(block_queue_t *queue, data_block_t *data) {
	while (sem_wait(queue->_m_write_sem)); //sem_wait can be interrupted, so we loop until it returns success. (return code 0)

	pthread_mutex_lock(queue->_m_modify_lock);

	sem_post(queue->_m_read_sem);
	queue->_m_data[queue->_m_head] = data;
	queue->_m_head++;
	queue->_m_head %= queue->_m_size;

	pthread_mutex_unlock(queue->_m_modify_lock);
}

data_block_t* block_queue_pop_chunk(block_queue_t *queue) {
	data_block_t *ret;
	while (sem_wait(queue->_m_read_sem)); //sem_wait can be interrupted, so we loop until it returns success. (return code 0)

	pthread_mutex_lock(queue->_m_modify_lock);

	sem_post(queue->_m_read_sem);
	ret = queue->_m_data[queue->_m_tail];
	queue->_m_tail++;
	queue->_m_tail %= queue->_m_size;

	pthread_mutex_unlock(queue->_m_modify_lock);
	return ret;
}


data_block_t *block_queue_peek_chunk(block_queue_t *queue) {
	data_block_t *ret;

	pthread_mutex_lock(queue->_m_modify_lock);

	ret = queue->_m_data[queue->_m_tail];

	pthread_mutex_unlock(queue->_m_modify_lock);
	return ret;
}

void block_queue_free(block_queue_t *queue) {
	sem_destroy(queue->_m_read_sem);
	sem_destroy(queue->_m_write_sem);
	pthread_mutex_destroy(queue->_m_modify_lock);

	free(queue->_m_data);
	free(queue->_m_read_sem);
	free(queue->_m_write_sem);
	free(queue->_m_modify_lock);
}
