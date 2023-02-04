/*
 * Copyright (c) 2023
 *               Dmitry Mikhirev <dmitry@mikhirev.ru>
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

#include <stdio.h>
#include <string.h>

#include "srvutils.h"

#define MAX_ADDR_LEN 1024

typedef struct {
  char *orig_addr;
  char *rev_addr;
  int res;
} test_vector;

test_vector test_vectors[] = { {"1.2.3.4", "4.3.2.1", 0},
			       {"210.123.110.220", "220.110.123.210", 0},
			       {
				       "2600:abcd::f03c:91ff:fe50:d2",
				       "2.d.0.0.0.5.e.f.f.f.1.9.c.3.0.f.0.0.0.0.0.0.0.0.d.c.b.a.0.0.6.2",
				       0
			       },
			       {
				       "1234:4321:5678:8765:90ab:ba09:cdef:fedc",
				       "c.d.e.f.f.e.d.c.9.0.a.b.b.a.0.9.5.6.7.8.8.7.6.5.1.2.3.4.4.3.2.1",
				       0
			       },
			       {
				       "::", 
				       "0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0",
				       0
			       },
			       {"1.2.3.4.5", "", -1},
			       {"1.2.3", "", -1},
			       {"asdf::", "", -1},
			       {"1234:1234:1234:1234:1234:1234:1234:12345", "", -1},
			       {"", "", -1},
			       {NULL, NULL, 0} };

int reverse_inet_addr(char *ipstr);

int
main(int argc, char **argv)
{
	char addr[MAX_ADDR_LEN];
	int res;
	int error_count = 0;
	test_vector *test;
	gross_ctx_t myctx = { 0x00 };

	ctx = &myctx;
        memset(ctx, 0, sizeof(gross_ctx_t));

	printf("Check: reverse IP address\n");
	
	for (test=test_vectors ; test->orig_addr ; test++) {
		strncpy(addr, test->orig_addr, MAX_ADDR_LEN);
		printf("  Checking '%s'...  ", addr);
		fflush(stdout);

		res = reverse_inet_addr(addr);
		if (res != test->res || (res == 0 && strncmp(addr, test->rev_addr, MAX_ADDR_LEN) != 0)) {
			if (argc>2) {
				if (res != test->res) {
					printf("\nERROR: For address '%s' result is %s.\n",
					       test->orig_addr, res?"FAIL":"OK");
				} else {
					printf("\nERROR: For address '%s' reversed string is '%s', reference is '%s'.\n",
						test->orig_addr, addr, test->rev_addr);
				}
			} else {
				printf("Fail.\n");
			}

			error_count++;
		} else {
			if (argc>2) { 
				if (res == 0) {
					printf("\n\treversed string: '%s'\n\treference: '%s'.\n",
						addr, test->rev_addr);
				} else {
					printf("\n\tresult: %s, reference: %s\n",
						res?"FAIL":"OK", test->res?"FAIL":"OK");
				}
			} else {
				printf("Done.\n");
			}
		}
	}

	return error_count > 0;
}
