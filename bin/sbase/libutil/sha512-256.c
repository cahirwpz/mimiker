/* public domain sha512/256 implementation based on fips180-3 */
#include <stdint.h>
#include "../sha512-256.h"

extern void sha512_sum_n(void *, uint8_t *, int n);

void
sha512_256_init(void *ctx)
{
	struct sha512_256 *s = ctx;
	s->len = 0;
	s->h[0] = 0x22312194fc2bf72cULL;
	s->h[1] = 0x9f555fa3c84c64c2ULL;
	s->h[2] = 0x2393b86b6f53b151ULL;
	s->h[3] = 0x963877195940eabdULL;
	s->h[4] = 0x96283ee2a88effe3ULL;
	s->h[5] = 0xbe5e1e2553863992ULL;
	s->h[6] = 0x2b0199fc2c85b8aaULL;
	s->h[7] = 0x0eb72ddc81c52ca2ULL;
}

void
sha512_256_sum(void *ctx, uint8_t md[SHA512_256_DIGEST_LENGTH])
{
	sha512_sum_n(ctx, md, 4);
}
