/* public domain sha512/256 implementation based on fips180-3 */

#include "sha512.h"

#define sha512_256  sha512  /*struct*/

enum { SHA512_256_DIGEST_LENGTH = 32 };

/* reset state */
void sha512_256_init(void *ctx);
/* process message */
#define sha512_256_update  sha512_update
/* get message digest */
/* state is ruined after sum, keep a copy if multiple sum is needed */
/* part of the message might be left in s, zero it if secrecy is needed */
void sha512_256_sum(void *ctx, uint8_t md[SHA512_256_DIGEST_LENGTH]);
