/* public domain sha256 implementation based on fips180-3 */

struct sha256 {
	uint64_t len;    /* processed message length */
	uint32_t h[8];   /* hash state */
	uint8_t buf[64]; /* message block buffer */
};

enum { SHA256_DIGEST_LENGTH = 32 };

/* reset state */
void sha256_init(void *ctx);
/* process message */
void sha256_update(void *ctx, const void *m, unsigned long len);
/* get message digest */
/* state is ruined after sum, keep a copy if multiple sum is needed */
/* part of the message might be left in s, zero it if secrecy is needed */
void sha256_sum(void *ctx, uint8_t md[SHA256_DIGEST_LENGTH]);
