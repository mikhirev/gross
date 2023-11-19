/*
 * Copyright (c) 2007,2008
 *               Eino Tuominen <eino@utu.fi>
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

#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <signal.h>

#ifdef HAVE_STDBOOL_H
# include <stdbool.h>
#endif

#include <stdatomic.h>

typedef int mseconds_t;

typedef struct thread_pool_s
{
	int work_queue_id;
	const char *name;	/* name of the pool for logging purposes */
	void *arg;		/* pool specific arguments, if needed */
} thread_pool_t;

typedef struct
{
	int count;
	pthread_mutex_t mx;
} reference_count_t;

#ifndef HAVE_BOOL
# ifndef bool
#  ifndef __bool_true_false_are_defined
#   define __bool_true_false_are_defined       1
typedef int bool;

#  define true 1
#  define false 0
#  endif /* __bool_true_false_are_defined */
# endif	/* bool */
#endif /* HAVE_BOOL */

typedef struct edict_s
{
	void *job;
	int resultmq;
	atomic_bool obsolete;
	reference_count_t reference;
	mseconds_t timelimit;
} edict_t;

typedef struct watchdog_s
{
	struct timespec last_seen;
	pthread_t tid;
	struct watchdog_s *next;	/* linked list */
} watchdog_t;

typedef struct
{
	void *state;
	int (*cleanup) ();
	watchdog_t watchdog;
} thread_ctx_t;

#define IDLETIME (mseconds_t)1000	/* max loop wait time */

typedef struct
{
	int max_thread;
	mseconds_t watchdog_time;
	bool watchdog;
} pool_limits_t;

typedef struct pool_ctx_s
{
	pthread_mutex_t *mx;
	int (*routine) (thread_pool_t *, thread_ctx_t *, edict_t *);
	thread_pool_t *info;	/* pool specific info */
	int count_thread;	/* number of threads in the pool */
	int count_idle;		/* idling threads */
	float ewma_idle;	/* moving average of count_idle */
	struct timespec last_idle_check;
	int max_thread;		/* maximum threads in the pool */
	int idle_time;		/* how many seconds to wait new jobs */
	watchdog_t *wdlist;	/* watchdog list */
	int watchdog_time;	/* watchdog timer, 0 is disabled */
} pool_ctx_t;

/* message queue wrap for edicts */
typedef struct edict_message_s
{
	edict_t *edict;
} edict_message_t;

#define LAMBDA 0.1
#define EWMA(ewma, observation) (ewma = (LAMBDA * observation + (1 - LAMBDA) * ewma))

int submit_job(thread_pool_t *pool, edict_t *edict);
thread_pool_t *create_thread_pool(const char *name, int (*routine) (thread_pool_t *, thread_ctx_t *,
	edict_t *), pool_limits_t *limits, void *arg);
edict_t *edict_get(bool forget);
void send_result(edict_t *edict, void *result);
void edict_unlink(edict_t *edict);

#endif /* THREAD_POOL_H */
