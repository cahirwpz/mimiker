#include <string.h>

int gethostname(char *name, size_t namelen)
{
	strlcpy(name, "localhost", namelen);
	return 0;
}
