/*
 * Copyright (c) 2006, 2007, 2008
 *               Eino Tuominen <eino@utu.fi>
 *               Antti Siira <antti@utu.fi>
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
 * check_dnsbl.c implements all dns-based checks:
 * 	dnsbl, rhsbl and dnswl
 */

#include "common.h"
#include "check_dnsbl.h"
#include "srvutils.h"
#include "utils.h"
#include "addrutils.h"
#include "worker.h"
#include "helper_dns.h"

/* the cleanup routine */
int
cleanup_dnsblc(void *state)
{
	ares_channel *channel;

	channel = (ares_channel *)state;
	ares_destroy(*channel);
	Free(channel);
	return 0;
}

int
add_dnsbl(dnsbl_t **current, const char *name, int weight)
{
	dnsbl_t *new;

	logstr(GLOG_DEBUG, "adding dnsbl: %s, weight %d", name, weight);

	new = Malloc(sizeof(dnsbl_t));
	memset(new, 0, sizeof(dnsbl_t));

	/*
	 * this is not threadsafe, but we do not need an exact result, we can
	 * afford errors here
	 * EDIT: time has passed, and I noticed that the comment
	 * is not very clear. The race condition the comment above
	 * refers to is in tolerate_dnsbl(). 
	 */
	new->tolerancecounter = ERRORTOLERANCE;

	new->name = strdup(name);
	new->next = *current;
	new->weight = weight;
	*current = new;
	return 1;
}

int
query_clearance(dnsbl_t *dnsbl)
{
	int retvalue;

	if (dnsbl->tolerancecounter > 0) {
		retvalue = TRUE;
	} else {
		retvalue = FALSE;
	}
	return retvalue;
}

int
tolerate_dnsbl(dnsbl_t *dnsbl)
{
	/* increment counter if needed */
	if (dnsbl->tolerancecounter < ERRORTOLERANCE) {
		logstr(GLOG_INFO, "incrementing tolerance counter for dnsbl %s", dnsbl->name);
		dnsbl->tolerancecounter++;
	}
	return 0;
}

int
increment_dnsbl_tolerance_counters(dnsbl_t *dnsbl)
{
	while (dnsbl) {
		tolerate_dnsbl(dnsbl);
		dnsbl = dnsbl->next;
	}

	return 0;
}


static void
#if ARES_VERSION_MAJOR > 0 && ARES_VERSION_MINOR > 4
addrinfo_callback(void *arg, int status, int timeouts, struct hostent *host)
#else
addrinfo_callback(void *arg, int status, struct hostent *host)
#endif
{
	chkresult_t *result;
	callback_arg_t *cba;

	cba = (callback_arg_t *)arg;

	if (status == ARES_SUCCESS) {
		/*
		 * DNSWL is definive, so it shortcuts after one match, so we don't
		 * need to send result here.
		 */
		if (cba->check_info->type != TYPE_DNSWL) {
			result = Malloc(sizeof(chkresult_t));
			memset(result, 0, sizeof(*result));
			result->judgment = J_SUSPICIOUS;
			result->weight = cba->dnsbl->weight;
			result->wait = true;
			result->checkname = cba->dnsbl->name;
			send_result(cba->edict, result);
		} else {
			*cba->dnslname = cba->dnsbl->name;
			*cba->done = true;
		}
		stat_dnsbl_match(cba->dnsbl->name);
		logstr(GLOG_DEBUG, "dns-match: %s for %s", cba->dnsbl->name, cba->querystr);
	}
	if (*cba->timeout) {
		logstr(GLOG_DEBUG, "dns-timeout: %s for %s", cba->dnsbl->name, cba->querystr);
		/* decrement tolerancecounter */
		cba->dnsbl->tolerancecounter--;
	}
	Free(cba);
}


int
dnsblc(thread_pool_t *info, thread_ctx_t *thread_ctx, edict_t *edict)
{
	ares_channel *channel;
	int nfds, count, ret;
	bool done = false;
	int timeout = 0;
	fd_set readers, writers;
	struct timeval tv;
	struct timespec ts, start, now, timeleft;
	char buffer[MAXQUERYSTRLEN];
	char *query;
	char qstr[REVERSED_INET6_ADDRSTRLEN];
	char *sender;
	char *ptr;
	const char *orig_qstr;
	dnsbl_t *dnsbl;
	callback_arg_t *callback_arg;
	const char *dnslname;
	int timeused;
	chkresult_t *result = NULL;
	grey_tuple_t *request = NULL;
	dns_check_info_t *check_info;

	logstr(GLOG_DEBUG, "dnsblc called: timelimit %d", edict->timelimit);

	/* fetch check_info */
	assert(info);
	assert(info->arg);
	check_info = (dns_check_info_t *)info->arg;

	/* initialize if we are not yet initialized */
	if (NULL == thread_ctx->state) {
		channel = Malloc(sizeof(*channel));
		if (ares_init(channel) != ARES_SUCCESS) {
			gerror("ares_init");
			goto FINISH;
		}
		thread_ctx->state = channel;
		thread_ctx->cleanup = &cleanup_dnsblc;
	} else {
		channel = (ares_channel *)thread_ctx->state;
	}

	request = (grey_tuple_t *)edict->job;
	assert(request);
	result = (chkresult_t *)Malloc(sizeof(chkresult_t));
	memset(result, 0, sizeof(*result));
	result->checkname = "dnsbl"; /* the default is only used in a GLOG_INSANE log line */

	if (check_info->type == TYPE_DNSBL || check_info->type == TYPE_DNSWL) {
		/* test the client ip address */
		assert(request->client_address);
		orig_qstr = request->client_address;
		if (strlen(orig_qstr) >= INET6_ADDRSTRLEN) {
			logstr(GLOG_ERROR, "invalid ipaddress: %s", orig_qstr);
			goto FINISH;
		}

		strncpy(qstr, request->client_address, INET6_ADDRSTRLEN);
		ret = reverse_inet_addr(qstr);
		if (ret < 0) {
			goto FINISH;
		}
	} else if (check_info->type == TYPE_RHSBL) {

		/* try to find the last '@' of the sender address */
		sender = strdup(request->sender);
		assert(sender);
		ptr = sender + strlen(sender);	/* end of the address */
		while ((ptr > sender) && (*ptr != '@'))
			ptr--;

		if (ptr > sender) {
			/* found */
			/* skip '@' */
			orig_qstr = strncpy(qstr, ptr + 1, INET6_ADDRSTRLEN);
			Free(sender);
		} else {
			/* no sender domain, no check */
			Free(sender);
			goto FINISH;
		}
	} else {
		logstr(GLOG_ERROR, "invalid check type");
		goto FINISH;
	}

	/* dns base list ;-) */
	dnsbl = check_info->dnsbase;

	/* initiate dnsbl queries */
	while (dnsbl) {
		assert(dnsbl->name);
		/*
		 * make sure the domain name is fully qualified in order
		 * to avoid unnecessary dns lookups
		 */
		snprintf(buffer, MAXQUERYSTRLEN, "%s.%s%s", qstr, dnsbl->name,
			*(dnsbl->name + strlen(dnsbl->name) - 1) != '.' ? "." : "");
		query = strdup(buffer);
		if (query_clearance(dnsbl) == TRUE) {
			logstr(GLOG_INSANE, "initiating dns query: %s", query);
			callback_arg = Malloc(sizeof(callback_arg_t));
			callback_arg->dnsbl = dnsbl;
			callback_arg->done = &done;
			callback_arg->dnslname = &dnslname;
			callback_arg->timeout = &timeout;
			callback_arg->querystr = orig_qstr;
			callback_arg->edict = edict;
			callback_arg->check_info = check_info;
			ares_gethostbyname(*channel, query, PF_INET, &addrinfo_callback, callback_arg);
		} else {
			logstr(GLOG_DEBUG, "skipping dnsbl %s due to timeouts.", dnsbl->name);
		}
		Free(query);
		dnsbl = dnsbl->next;
	}

	clock_gettime(CLOCK_TYPE, &start);

	while (!timeout) {
		do {
			clock_gettime(CLOCK_TYPE, &now);
			timeused = ms_diff(&now, &start);
			if (timeused >= edict->timelimit)
				break;

			mstotimespec(edict->timelimit - timeused, &timeleft);

			FD_ZERO(&readers);
			FD_ZERO(&writers);
			nfds = ares_fds(*channel, &readers, &writers);
			if (nfds == 0)
				break;
			ares_timeout(*channel, NULL, &tv);
			tvtots(&tv, &ts);

			if (ms_diff(&timeleft, &ts) < 0)
				memcpy(&ts, &timeleft, sizeof(timeleft));

			tstotv(&ts, &tv);

			count = select(nfds, &readers, &writers, NULL, &tv);
			ares_process(*channel, &readers, &writers);
		} while (!(done || edict->obsolete));

		clock_gettime(CLOCK_TYPE, &now);
		timeused = ms_diff(&now, &start);

		if (timeused >= edict->timelimit) {
			logstr(GLOG_INSANE, "dnsbl timeout");
			/* the final timeout value */
			timeout = 1;
		}
		if (edict->obsolete || done || nfds == 0)
			break;
	}

	ares_cancel(*channel);
      FINISH:
	if (result) {
		if (done && check_info->type == TYPE_DNSWL) {
			result->judgment = J_PASS;
			result->checkname = dnslname;
		} else {
			result->judgment = J_UNDEFINED;
		}
	}
	send_result(edict, result);

	logstr(GLOG_DEBUG, "dnsblc returning");
	if (request) {
		request_unlink(request);
	}

	return 0;
}

void
dnsbl_init(dns_check_info_t *check_info, pool_limits_t *limits)
{
	thread_pool_t *pool;

	/* initialize the thread pool */
	logstr(GLOG_INFO, "initializing dns checker thread pool '%s'", check_info->name);
	pool = create_thread_pool(check_info->name, &dnsblc, limits, (void *)check_info);
	if (pool == NULL)
		daemon_fatal("create_thread_pool");

	register_check(pool, check_info->definitive);
}
