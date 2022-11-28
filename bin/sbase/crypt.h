/* See LICENSE file for copyright and license details. */
struct crypt_ops {
	void (*init)(void *);
	void (*update)(void *, const void *, unsigned long);
	void (*sum)(void *, uint8_t *);
	void *s;
};

int cryptcheck(int, char **, struct crypt_ops *, uint8_t *, size_t);
int cryptmain(int, char **, struct crypt_ops *, uint8_t *, size_t);
int cryptsum(struct crypt_ops *, int, const char *, uint8_t *);
void mdprint(const uint8_t *, const char *, size_t);
