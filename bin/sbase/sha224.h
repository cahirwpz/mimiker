/* public domain sha224 implementation based on fips180-3 */

#include "sha256.h"

#define sha224  sha256  /*struct*/

enum { SHA224_DIGEST_LENGTH = 28 };

/* reset state */
void sha224_init(void *ctx);
/* process message */
#define sha224_update  sha256_update
/* get message digest */
/* state is ruined after sum, keep a copy if multiple sum is needed */
/* part of the message might be left in s, zero it if secrecy is needed */
void sha224_sum(void *ctx, uint8_t md[SHA224_DIGEST_LENGTH]);
