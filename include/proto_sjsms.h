/*
 * Copyright (c) 2006,2007
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

#ifndef PROTO_SJSMS_H
#define PROTO_SJSMS_H

#include "common.h"
#include "worker.h"

#define WITH_HELO 0

typedef struct
{
	uint16_t msglen;
	uint16_t sender;
	uint16_t recipient;
	uint16_t client_address;
#if WITH_HELO
	uint16_t helo_name;
#endif
	char message[MAXLINELEN];
} grey_req_t;

#define MSGTYPE_QUERY    ((uint16_t) 0)
#define MSGTYPE_LOGMSG   ((uint16_t) 1)
#define MSGTYPE_QUERY_V2 ((uint16_t) 2)

typedef struct
{
	uint16_t msgtype;
	uint16_t msglen;
	char message[MAXLINELEN];
} sjsms_msg_t;

int fold(grey_req_t *request, const char *sender, const char *recipient, const char *caddr, const char *helo);
int sendquery(int fd, struct sockaddr_in *gserv, grey_req_t *request);
int sendquerystr(int fd, struct sockaddr_in *gserv, const char *querystr);
int senderrormsg(int fd, struct sockaddr_in *gserv, const char *fmt, ...);
int sjsms_to_host_order(sjsms_msg_t *message);
int recvquery(sjsms_msg_t *message, grey_req_t *request);
char *recvquerystr(sjsms_msg_t *message);
char *buildquerystr(const char *sender, const char *rcpt, const char *caddr, const char *helo);

#endif
