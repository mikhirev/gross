/*
 * Copyright (c) 2006, 2007
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

#include <stdarg.h>

#include "proto_sjsms.h"

/* prototypes of internals */
int send_sjsms_msg(int fd, struct sockaddr_in *grserv, sjsms_msg_t *message);

char *
buildquerystr(const char *sender, const char *rcpt, const char *caddr, const char *helo)
{
	char buffer[MAXLINELEN];

	snprintf(buffer, MAXLINELEN - 1, "sender=%s\nrecipient=%s\nclient_address=%s%s%s\n\n",
	    sender, rcpt, caddr, helo ? "\nhelo_name=" : "", helo ? helo : "");

	return strdup(buffer);
}

int
senderrormsg(int fd, struct sockaddr_in *gserv, const char *fmt, ...)
{
	sjsms_msg_t message;
	va_list vap;

	va_start(vap, fmt);
	vsnprintf(message.message, MAXLINELEN, fmt, vap);
	va_end(vap);

	message.msglen = MIN(strlen(message.message) + 1, MAXLINELEN);
	message.msgtype = MSGTYPE_LOGMSG;

	return send_sjsms_msg(fd, gserv, &message);
}


int
sendquery(int fd, struct sockaddr_in *gserv, grey_req_t *request)
{
	sjsms_msg_t message;

	/* check struct grey_req_t */
	message.msglen = MIN(ntohs(request->msglen) + sizeof(grey_req_t) - sizeof(char *), MAXLINELEN);
	message.msgtype = MSGTYPE_QUERY;
	memcpy(&message.message, request, message.msglen);
	return send_sjsms_msg(fd, gserv, &message);
}

int
sendquerystr(int fd, struct sockaddr_in *gserv, const char *querystr)
{
	sjsms_msg_t message;

	message.msglen = MIN(strlen(querystr), MAXLINELEN);
	message.msgtype = MSGTYPE_QUERY_V2;
	memcpy(&message.message, querystr, message.msglen);
	return send_sjsms_msg(fd, gserv, &message);
}

int
recvquery(sjsms_msg_t *message, grey_req_t *request)
{
	memcpy(request, message->message, MIN(message->msglen, MAXLINELEN));
	request->message[MAXLINELEN - 1] = '\0';

	return 1;
}

char *
recvquerystr(sjsms_msg_t *message)
{
	char querystr[MAXLINELEN] = { '\0' };

	snprintf(querystr, MAXLINELEN - 1, "%s", message->message);
	querystr[MAXLINELEN - 1] = '\0';

	return strdup(querystr);
}

int
send_sjsms_msg(int fd, struct sockaddr_in *gserv, sjsms_msg_t *message)
{
	int slen;
	int mlen;

	slen = sizeof(struct sockaddr_in);
	mlen = message->msglen + 2 * sizeof(uint16_t);
	message->msgtype = htons(message->msgtype);
	message->msglen = htons(message->msglen);

	return sendto(fd, message, mlen, 0, (struct sockaddr *)gserv, slen);
}

int
sjsms_to_host_order(sjsms_msg_t *message)
{
	message->msgtype = ntohs(message->msgtype);
	message->msglen = ntohs(message->msglen);

	return 1;
}
