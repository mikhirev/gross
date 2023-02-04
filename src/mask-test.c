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

#include "common.h"
#include "srvutils.h"
#include "addrutils.h"

#define MAX_ADDR_LEN 1024

typedef struct {
  char *orig_addr;
  char *masked_addr;
  int mask;
} test_vector;

test_vector test_vectors[] = { {"1.2.3.4", "1.0.0.0", 8},
			       {"1.2.3.4", "1.2.0.0", 16},
			       {"1.2.3.4", "1.2.3.0", 24},
			       {"1.2.3.4", "1.2.3.4", 32},
			       {"210.123.110.220", "208.0.0.0", 6},
			       {"210.123.110.220", "210.96.0.0", 11},
			       {"210.123.110.220", "210.123.96.0", 19},
			       {"210.123.110.220", "210.123.110.220", 30},
			       {"2600:abcd::f03c:91ff:fe50:d2", "2600::", 16},
			       {"2600:abcd::f03c:91ff:fe50:d2", "2600:abcd::", 32},
			       {"2600:abcd::f03c:91ff:fe50:d2", "2600:abcd::", 64},
			       {"2600:abcd::f03c:91ff:fe50:d2", "2600:abcd:0:0:8000::", 65},
			       {"2600:abcd::f03c:91ff:fe50:d2", "2600:abcd::f03c:8000:0:0", 83},
			       {"2600:abcd::f03c:91ff:fe50:d2", "2600:abcd::f03c:91ff:fe50:d0", 124},
			       {"1.2.3.4.5", NULL, 24},
			       {"1.2.3", NULL, 24},
			       {"asdf::", NULL, 64},
			       {"1234:1234:1234:1234:1234:1234:1234:12345", NULL, 42},
			       {"", NULL, 0},
			       {NULL, NULL, 0} };

int
main(int argc, char **argv)
{
	int res;
	int error_count = 0;
	test_vector *test;
	gross_ctx_t myctx = { 0x00 };
	char *masked;

#define FAIL(...) \
	if (argc > 2) \
		printf(__VA_ARGS__); \
	else \
		printf("Fail.\n"); \
	error_count++;

	ctx = &myctx;
        memset(ctx, 0, sizeof(gross_ctx_t));

	printf("Check: Apply mask to IP address\n");
	
	for (test=test_vectors ; test->orig_addr ; test++) {
		printf("  Checking '%s/%d'...  ", test->orig_addr, test->mask);
		fflush(stdout);

		ctx->config.grey_mask = test->mask;
		ctx->config.grey_mask6 = test->mask;
		masked = grey_mask(test->orig_addr);

		if (masked == NULL) {
			if (test->masked_addr != NULL) {
				FAIL("\nERROR: Failed to apply mask for '%s/%d' (reference is %s).\n",
				     test->orig_addr, test->mask, test->masked_addr);
			} else {
				printf("Done.\n");
			}
		} else if (test->masked_addr == NULL) {
			FAIL("\nERROR: Applying mask for '%s/%d' returned %s, expected failure..\n",
			       test->orig_addr, test->mask, masked);
		} else if (strcmp(test->masked_addr, masked) != 0 ) {
			FAIL("\nERROR: masked string: '%s'\n\treference: '%s'.\n",
			     masked, test->masked_addr);
		} else {
			printf("Done.\n");
		}
		free(masked);
	}

	return error_count > 0;
}
