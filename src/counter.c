/*
 * Copyright (c) 2008
 *                    Eino Tuominen <eino@utu.fi>
 *                    Antti Siira <antti@utu.fi>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * This file implements generic counters. Use counter_create() to get a counter id,
 * then you can use access functions, which are all implemented in this file.
 * All the functions are re-entrant and thread safe unless stated otherwise.
 */

#include "common.h"
#include "counter.h"
#include "srvutils.h"

#define GLOBAL_COUNTER_LOCK { assert(pthread_mutex_lock(&global_counter_lk) == 0); }
#define GLOBAL_COUNTER_UNLOCK { pthread_mutex_unlock(&global_counter_lk); }

/* prototypes of internals */
counter_t *counterbyid(int counterid);
void counter_realloc(void);
counter_t *check_freelist(void);

/* array of counters */
int numcounters = 0;
int counterspace = 1;
counter_t **counters;
counter_queue_t *freecounters = NULL;

pthread_mutex_t global_counter_lk = PTHREAD_MUTEX_INITIALIZER;

/*
 *  counter_realloc	- doubles the space reservation for counters
 *  			  not thread safe, the caller MUST hold GLOBAL_COUNTER_LOCK
 */
void
counter_realloc(void)
{
	size_t countersize;

	/* counters must be initialized first */
	if (counterspace == 0)
		counterspace = 1;

	logstr(GLOG_DEBUG, "doubling the space for counters from %d to %d", counterspace, counterspace * 2);

	countersize = counterspace * sizeof(counter_t *);

	/* double the size of the array */
	counters = realloc(counters, countersize * 2);
	counterspace *= 2;
}


/*
 * counterbyid	- returns pointer to the counter referred by id
 */
counter_t *
counterbyid(int cid)
{
	counter_t *counter;

	/*
	 * we must lock the mutex because counters array might be replaced 
	 */
	GLOBAL_COUNTER_LOCK;

	counter = counters[cid];

	GLOBAL_COUNTER_UNLOCK;

	return counter;
}

/*
 * get_counter    - returns a new counter
 */
int
counter_create(const char *name, const char *description)
{
	int i;
	counter_t *counter;

	GLOBAL_COUNTER_LOCK;
	/* is this the first call? */
	if (0 == numcounters)
		counters = calloc(counterspace, sizeof(counter_t *));

	/* first check the list of free counters */

	counter = check_freelist();
	if (counter) {
		/* found one, so let's use it */
		i = counter->id;
	} else {
		/* must create a new counter */
		i = numcounters;
		++numcounters;
		counter = Malloc(sizeof(counter_t));
		memset(counter, 0, sizeof(counter_t));
		pthread_mutex_init(&counter->mx, NULL);

		counter->id = i;

		if (numcounters > counterspace) {
			/* there is no space left in the array */
			counter_realloc();
		}

		counters[i] = counter;
	}

	GLOBAL_COUNTER_UNLOCK;

	counter->name = name;
	counter->description = description;
	counter->active = true;


	return i;
}

/* 
 *  release_queue        - add the counter to the counterqueue. Clean up.
 */
int
counter_release(int cid)
{
	int ret;
	counter_t *counter;
	counter_queue_t *f;

	counter = counterbyid(cid);

	ret = pthread_mutex_lock(&counter->mx);
	assert(ret == 0);
	counter->active = false;
	counter->publish = false;
	counter->name = NULL;
	counter->description = NULL;
	counter->value = 0;
	ret = pthread_mutex_unlock(&counter->mx);
	assert(ret == 0);

	/* insert the counter to the list of free counters */
	GLOBAL_COUNTER_LOCK;
	f = Malloc(sizeof(counter_queue_t));
	f->counter = counter;
	f->next = freecounters;
	freecounters = f;
	GLOBAL_COUNTER_UNLOCK;

	return 0;
}


/*
 *  check_freelist        - tries to fetch a message from the counterqueue
 *  			    must hold GLOBAL_COUNTER_LOCK;
 */
counter_t *
check_freelist(void)
{
	counter_t *counter = NULL;
	counter_queue_t *next;

	if (freecounters) {
		/* found */
		counter = freecounters->counter;
		next = freecounters->next;
		Free(freecounters);
		freecounters = next;
	}

	return counter;
}


int64_t
counter_read(int cid)
{
	counter_t *counter;
	int64_t value;
	int ret;

	counter = counterbyid(cid);
	if (!counter)
		return 0;

	ret = pthread_mutex_lock(&counter->mx);
	assert(ret == 0);
	value = counter->value;
	pthread_mutex_unlock(&counter->mx);

	return value;
}

int64_t
counter_increment(int cid)
{
	counter_t *counter;
	int64_t value;
	int ret;

	counter = counterbyid(cid);
	if (!counter)
		return 0;

	ret = pthread_mutex_lock(&counter->mx);
	assert(ret == 0);
	counter->value++;
	value = counter->value;
	pthread_mutex_unlock(&counter->mx);

	return value;
}

int64_t
counter_decrement(int cid)
{
	counter_t *counter;
	int64_t value;
	int ret;

	counter = counterbyid(cid);
	if (!counter)
		return 0;

	ret = pthread_mutex_lock(&counter->mx);
	assert(ret == 0);
	counter->value--;
	value = counter->value;
	pthread_mutex_unlock(&counter->mx);

	return value;
}

int64_t
counter_restart(int cid)
{
	counter_t *counter;
	int64_t value;
	int ret;

	counter = counterbyid(cid);
	if (!counter)
		return 0;

	ret = pthread_mutex_lock(&counter->mx);
	assert(ret == 0);
	value = counter->value;
	counter->value = 0;
	pthread_mutex_unlock(&counter->mx);

	return value;
}

int64_t
counter_set(int cid, int64_t newvalue)
{
	counter_t *counter;
	int64_t value;
	int ret;

	counter = counterbyid(cid);
	if (!counter)
		return 0;

	ret = pthread_mutex_lock(&counter->mx);
	assert(ret == 0);
	value = counter->value;
	counter->value = newvalue;
	pthread_mutex_unlock(&counter->mx);

	return value;
}
