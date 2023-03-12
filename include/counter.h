/*
 * Copyright (c) 2006,2007 Eino Tuominen <eino@utu.fi>
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

#ifndef COUNTER_H
#define COUNTER_H

typedef struct counter_s
{
	pthread_mutex_t mx;
	int64_t value;
	int id;
	const char *name;
	const char *description;
	bool publish;
	bool active;
} counter_t;

typedef struct counter_queue_s
{
	counter_t *counter;
	struct counter_queue_s *next;
} counter_queue_t;

int counter_create(const char *name, const char *description);
int counter_release(int cid);
int64_t counter_increment(int cid);
int64_t counter_decrement(int cid);
int64_t counter_restart(int cid);
int64_t counter_set(int cid, int64_t value);
int64_t counter_read(int cid);

#endif /* COUNTER_H */
