
#ifndef THREAD_POOL_H
#define THREAD_POOL_H

typedef struct thread_pool_s {
	int work_queue_id;
} thread_pool_t;

typedef struct work_order_s {
	struct timespec *timelimit;
	void *job_ctx;
	void *result;
} work_order_t;

typedef struct pool_ctx_s {
	pthread_mutex_t *mx;
	void *(*routine)(void *);
	thread_pool_t *info; 	/* public info */
	int count_thread;	/* number of threads in the pool */
	int count_idle;		/* idling threads */
} pool_ctx_t;

int submit_job(thread_pool_t *pool, void *job, struct timespec *timeout);
thread_pool_t *create_thread_pool(void *(*routine)(void *));

#endif /* THREAD_POOL_H */