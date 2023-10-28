/*
 * Copyright (c) 2023 Dmitry Mikhirev <dmitry@mikhirev.ru>
 *
 * Copyright (c) 2008
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

#include "common.h"
#include "srvutils.h"
#include "utils.h"
#include "worker.h"
#include "helper_dns.h"


static bool
valid_dn(const char *dn)
{
	const char *ptr = dn;

	while (*ptr != '\0') {
		if (!((*ptr >= 'a' && *ptr <= 'z') ||
				(*ptr >= 'A' && *ptr <= 'Z') ||
				(*ptr >= '0' && *ptr <= '9')))
		{
			return false;
		}
		for (; *ptr != '\0' && *ptr != '.'; ptr++) {
			if (!((*ptr >= 'a' && *ptr <= 'z') ||
					(*ptr >= 'A' && *ptr <= 'Z') ||
					(*ptr >= '0' && *ptr <= '9') ||
					('-'  == *ptr && *(ptr+1) != '.' && *(ptr+1) != '\0')))
			{
				return false;
			}
		}
		if ('.' == *ptr) {
			if (*(ptr+1) == '\0') {
				return false;
			} else {
				ptr++;
			}
		}
	}

	return true;
}


int
helo(thread_pool_t *info, thread_ctx_t *thread_ctx, edict_t *edict)
{
	chkresult_t *result;
	struct hostent *host, *reversehost;
	int family;
	grey_tuple_t *request;
	const char *helostr;
	const char *client_address;
	char addrstrbuf[INET6_ADDRSTRLEN];
	const char *ptr;
	mseconds_t timelimit;

	request = (grey_tuple_t *)edict->job;
	helostr = request->helo_name;
	client_address = request->client_address;
	assert(helostr);
	assert(client_address);

	if (strchr(client_address, ':')) {
		family = AF_INET6;
	} else {
		family = AF_INET;
	}

	result = (chkresult_t *)Malloc(sizeof(chkresult_t));
	memset(result, 0, sizeof(*result));
	result->judgment = J_UNDEFINED;
	result->checkname = "helo";

	timelimit = edict->timelimit;

	/* check the validity of helo string */
	if (!valid_dn(helostr)) {
		logstr(GLOG_DEBUG, "Syntactically suspicious helo name");
		/* one for invalid helo string AND one for not matching the client ip */
		result->weight += 2; /* FIXME */
		/* no point in doing dns queries as invalid helo wouldn't match the client ip */
		goto FINISH;
	}

	/* check if helo resolves to client ip */
	host = Gethostbyname(helostr, family, timelimit);
	if (host) {
		ptr = inet_ntop(family, host->h_addr_list[0], addrstrbuf, INET6_ADDRSTRLEN);
		if (NULL == ptr) {
			/* this should never happen */
			logstr(GLOG_ERROR, "helo_name resolved to an invalid ip");
			goto FINISH;
		}
		logstr(GLOG_INSANE, "client_address (%s), helo (%s)",
			client_address, addrstrbuf); /* FIXME */
		if (strcmp(addrstrbuf, client_address)) {
			logstr(GLOG_DEBUG, "helo name (%s) does not resolve to client address (%s)",
				helostr, client_address);
			result->weight += 1; /* FIXME */
		}
	} else {
		logstr(GLOG_DEBUG, "helo_name not resolvable");
		result->weight += 1; /* FIXME */
	}

	/* check if client's PTR record match helo */
	reversehost = Gethostbyaddr_str(client_address, timelimit);
        if (reversehost) {
                logstr(GLOG_INSANE, "client_address (%s) has a PTR record (%s)",
                        client_address, reversehost->h_name);
		if (strcmp(reversehost->h_name, helostr)) {
			logstr(GLOG_DEBUG, "PTR for client_address (%s) differs from helo_name (%s)",
				reversehost->h_name, helostr);
			result->weight += 1; /* FIXME */
		}
	} else {
		logstr(GLOG_DEBUG, "client_address (%s) does not have a PTR record",
				client_address);
		result->weight += 1; /* FIXME */
	}

      FINISH:
	if (result->weight > 0)
		result->judgment = J_SUSPICIOUS;
	send_result(edict, result);
	logstr(GLOG_DEBUG, "helo returning");
	request_unlink(request);

	return 0;
}

void
helo_init(pool_limits_t *limits)
{
	thread_pool_t *pool;

	/* initialize the thread pool */
	logstr(GLOG_INFO, "initializing helo thread pool");
	pool = create_thread_pool("helo", &helo, limits, NULL);
	if (pool == NULL)
		daemon_fatal("create_thread_pool");

	register_check(pool, false);
}
