/* public domain sha512 implementation based on fips180-3 */

struct sha512 {
	uint64_t len;     /* processed message length */
	uint64_t h[8];    /* hash state */
	uint8_t buf[128]; /* message block buffer */
};

enum { SHA512_DIGEST_LENGTH = 64 };

/* reset state */
void sha512_init(void *ctx);
/* process message */
void sha512_update(void *ctx, const void *m, unsigned long len);
/* get message digest */
/* state is ruined after sum, keep a copy if multiple sum is needed */
/* part of the message might be left in s, zero it if secrecy is needed */
void sha512_sum(void *ctx, uint8_t md[SHA512_DIGEST_LENGTH]);
