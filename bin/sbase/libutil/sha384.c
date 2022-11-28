/* public domain sha384 implementation based on fips180-3 */
#include <stdint.h>
#include "../sha384.h"

extern void sha512_sum_n(void *, uint8_t *, int n);

void
sha384_init(void *ctx)
{
	struct sha384 *s = ctx;
	s->len = 0;
	s->h[0] = 0xcbbb9d5dc1059ed8ULL;
	s->h[1] = 0x629a292a367cd507ULL;
	s->h[2] = 0x9159015a3070dd17ULL;
	s->h[3] = 0x152fecd8f70e5939ULL;
	s->h[4] = 0x67332667ffc00b31ULL;
	s->h[5] = 0x8eb44a8768581511ULL;
	s->h[6] = 0xdb0c2e0d64f98fa7ULL;
	s->h[7] = 0x47b5481dbefa4fa4ULL;
}

void
sha384_sum(void *ctx, uint8_t md[SHA384_DIGEST_LENGTH])
{
	sha512_sum_n(ctx, md, 6);
}
