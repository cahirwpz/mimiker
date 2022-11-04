/* public domain sha512 implementation based on fips180-3 */

#include "sha512.h"

#define sha384  sha512  /*struct*/

enum { SHA384_DIGEST_LENGTH = 48 };

/* reset state */
void sha384_init(void *ctx);
/* process message */
#define sha384_update  sha512_update
/* get message digest */
/* state is ruined after sum, keep a copy if multiple sum is needed */
/* part of the message might be left in s, zero it if secrecy is needed */
void sha384_sum(void *ctx, uint8_t md[SHA384_DIGEST_LENGTH]);
