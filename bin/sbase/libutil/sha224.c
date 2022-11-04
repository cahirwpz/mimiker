/* public domain sha224 implementation based on fips180-3 */
#include <stdint.h>
#include "../sha224.h"

extern void sha256_sum_n(void *, uint8_t *, int n);

void
sha224_init(void *ctx)
{
	struct sha224 *s = ctx;
	s->len = 0;
	s->h[0] = 0xc1059ed8;
	s->h[1] = 0x367cd507;
	s->h[2] = 0x3070dd17;
	s->h[3] = 0xf70e5939;
	s->h[4] = 0xffc00b31;
	s->h[5] = 0x68581511;
	s->h[6] = 0x64f98fa7;
	s->h[7] = 0xbefa4fa4;
}

void
sha224_sum(void *ctx, uint8_t md[SHA224_DIGEST_LENGTH])
{
	sha256_sum_n(ctx, md, 8);
}
