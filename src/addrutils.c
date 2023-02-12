#include "common.h"
#include "srvutils.h"
#include "addrutils.h"

static int
reverse_inet4_addr(char *ipstr)
{
	unsigned int ipa, tmp;
	int i;
	int ret;
	struct in_addr inaddr;
	const char *ptr;
	char tmpstr[INET_ADDRSTRLEN];
	size_t iplen;

	if ((iplen = strlen(ipstr)) >= INET_ADDRSTRLEN) {
		return -1;
	}
	ret = inet_pton(AF_INET, ipstr, &inaddr);
	switch (ret) {
	case -1:
		gerror("reverse_inet_addr: inet_pton");
		return -1;
		break;
	case 0:
		logstr(GLOG_ERROR, "not a valid ip address: %s", ipstr);
		return -1;
		break;
	}

	/* case default */
	ipa = inaddr.s_addr;

	tmp = 0;

	for (i = 0; i < 4; i++) {
		tmp = tmp << 8;
		tmp |= ipa & 0xff;
		ipa = ipa >> 8;
	}

	/*
	 * this tmpstr hack here is because at least FreeBSD seems to handle
	 * buffer lengths differently from Linux and Solaris. Specifically,
	 * with inet_ntop(AF_INET, &tmp, ipstr, iplen) one gets a truncated
	 * address in ipstr in FreeBSD.
	 */
	ptr = inet_ntop(AF_INET, &tmp, tmpstr, INET_ADDRSTRLEN);
	if (!ptr) {
		gerror("inet_ntop");
		return -1;
	}
	assert(strlen(tmpstr) == iplen);
	strncpy(ipstr, tmpstr, iplen);

	return 0;
}

static int
reverse_inet6_addr(char *ipstr)
{
	int i;
	int ret;
	struct in6_addr inaddr;
	const char *ptr;

	if (strlen(ipstr) >= INET6_ADDRSTRLEN) {
		return -1;
	}
	ret = inet_pton(AF_INET6, ipstr, &inaddr);
	switch (ret) {
	case -1:
		gerror("reverse_inet_addr: inet_pton");
		return -1;
		break;
	case 0:
		logstr(GLOG_ERROR, "not a valid ip address: %s", ipstr);
		return -1;
		break;
	}

	/* case default */
	snprintf(ipstr, REVERSED_INET6_ADDRSTRLEN,
			"%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x."
			"%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x",
			inaddr.s6_addr[15] & 0xf, inaddr.s6_addr[15] >> 4,
			inaddr.s6_addr[14] & 0xf, inaddr.s6_addr[14] >> 4,
			inaddr.s6_addr[13] & 0xf, inaddr.s6_addr[13] >> 4,
			inaddr.s6_addr[12] & 0xf, inaddr.s6_addr[12] >> 4,
			inaddr.s6_addr[11] & 0xf, inaddr.s6_addr[11] >> 4,
			inaddr.s6_addr[10] & 0xf, inaddr.s6_addr[10] >> 4,
			inaddr.s6_addr[9] & 0xf, inaddr.s6_addr[9] >> 4,
			inaddr.s6_addr[8] & 0xf, inaddr.s6_addr[8] >> 4,
			inaddr.s6_addr[7] & 0xf, inaddr.s6_addr[7] >> 4,
			inaddr.s6_addr[6] & 0xf, inaddr.s6_addr[6] >> 4,
			inaddr.s6_addr[5] & 0xf, inaddr.s6_addr[5] >> 4,
			inaddr.s6_addr[4] & 0xf, inaddr.s6_addr[4] >> 4,
			inaddr.s6_addr[3] & 0xf, inaddr.s6_addr[3] >> 4,
			inaddr.s6_addr[2] & 0xf, inaddr.s6_addr[2] >> 4,
			inaddr.s6_addr[1] & 0xf, inaddr.s6_addr[1] >> 4,
			inaddr.s6_addr[0] & 0xf, inaddr.s6_addr[0] >> 4);

	return 0;
}

/*
 * reverse_inet_addr	- reverse ipaddress string for dnsbl query
 *                        e.g. 1.2.3.4 -> 4.3.2.1
 */
int
reverse_inet_addr(char *ipstr)
{
	if (strchr(ipstr, ':')) {
		return reverse_inet6_addr(ipstr);
	}
	return reverse_inet4_addr(ipstr);
}


static char *
grey_mask_v4(char *ipstr)
{
	int ret;
	in_addr_t ip, net, mask;
	const char *ptr = NULL;
	char masked[INET_ADDRSTRLEN] = { '\0' };
	struct in_addr inaddr;

	/*
	 * apply checkmask to the ip
	 */
	if (strlen(ipstr) >= INET_ADDRSTRLEN) {
		return NULL;
	}

	ret = inet_pton(AF_INET, ipstr, &inaddr);
	switch (ret) {
	case -1:
		gerror("test_tuple: inet_pton");
		return NULL;
		break;
	case 0:
		return NULL;
		break;
	}

	/* case default */
	ip = inaddr.s_addr;

	/* this is 0xffffffff ^ (2 ** (32 - mask - 1) - 1) */
	mask = 0xffffffff ^ ((1 << (32 - ctx->config.grey_mask)) - 1);

	/* ip is in network order */
	net = ip & htonl(mask);

	ptr = inet_ntop(AF_INET, &net, masked, INET_ADDRSTRLEN);
	if (!ptr) {
		logstr(GLOG_ERROR, "test_tuple: inet_ntop: %s", strerror(errno));
		return NULL;
	}
	return strdup(masked);
}

static char *
grey_mask_v6(char *ipstr)
{
	int ret, i;
	struct in6_addr inaddr;
	uint32_t mask_part;
	const char *ptr = NULL;
	char masked[INET6_ADDRSTRLEN] = { '\0' };

	if (strlen(ipstr) >= INET6_ADDRSTRLEN) {
		return NULL;
	}

	ret = inet_pton(AF_INET6, ipstr, &inaddr);
	switch (ret) {
	case -1:
		gerror("test_tuple: inet_pton");
		return NULL;
		break;
	case 0:
		return NULL;
		break;
	}

	for (i = 0; i < (int)(sizeof(inaddr.s6_addr)/sizeof(inaddr.s6_addr[0])); i++) {
		if (i < ctx->config.grey_mask6 >> 3) {
			continue;
		} else if (i > ctx->config.grey_mask6 >> 3) {
			mask_part =0;
		} else {
			//mask_part = 0xffffffff ^ ((1 << ((i + 1) * 32 - ctx->config.grey_mask6)) - 1);
			mask_part = 0xff ^ ((1 << (8 - (ctx->config.grey_mask6 & 0x7))) - 1);
		}
		inaddr.s6_addr[i] &= mask_part;
	}

	ptr = inet_ntop(AF_INET6, &inaddr, masked, INET6_ADDRSTRLEN);
	if (!ptr) {
		logstr(GLOG_ERROR, "test_tuple: inet_ntop: %s", strerror(errno));
		return NULL;
	}
	return strdup(masked);
}

char *
grey_mask(char *ipstr)
{
	char *masked;

	/*
	 * apply checkmask to the ip
	 */
	masked = grey_mask_v4(ipstr);
	if (masked == NULL) {
		masked = grey_mask_v6(ipstr);
	}
	if (masked == NULL) {
		logstr(GLOG_NOTICE, "invalid ipaddress: %s", ipstr);
	}

	return masked;
}
