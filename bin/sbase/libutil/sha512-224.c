/* public domain sha512/224 implementation based on fips180-3 */
#include <stdint.h>
#include "../sha512-224.h"

extern void sha512_sum_n(void *, uint8_t *, int n);

void
sha512_224_init(void *ctx)
{
	struct sha512_224 *s = ctx;
	s->len = 0;
	s->h[0] = 0x8c3d37c819544da2ULL;
	s->h[1] = 0x73e1996689dcd4d6ULL;
	s->h[2] = 0x1dfab7ae32ff9c82ULL;
	s->h[3] = 0x679dd514582f9fcfULL;
	s->h[4] = 0x0f6d2b697bd44da8ULL;
	s->h[5] = 0x77e36f7304c48942ULL;
	s->h[6] = 0x3f9d85a86a1d36c8ULL;
	s->h[7] = 0x1112e6ad91d692a1ULL;
}

void
sha512_224_sum(void *ctx, uint8_t md[SHA512_224_DIGEST_LENGTH])
{
	sha512_sum_n(ctx, md, 4);
}
