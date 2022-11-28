/* See LICENSE file for copyright and license details. */
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../util.h"

mode_t
getumask(void)
{
	mode_t mask = umask(0);
	umask(mask);
	return mask;
}

mode_t
parsemode(const char *str, mode_t mode, mode_t mask)
{
	char *end;
	const char *p = str;
	int octal, op;
	mode_t who, perm, clear;

	octal = strtol(str, &end, 8);
	if (*end == '\0') {
		if (octal < 0 || octal > 07777)
			eprintf("%s: invalid mode\n", str);
		return octal;
	}
next:
	/* first, determine which bits we will be modifying */
	for (who = 0; *p; p++) {
		switch (*p) {
		/* masks */
		case 'u':
			who |= S_IRWXU|S_ISUID;
			continue;
		case 'g':
			who |= S_IRWXG|S_ISGID;
			continue;
		case 'o':
			who |= S_IRWXO;
			continue;
		case 'a':
			who |= S_IRWXU|S_ISUID|S_IRWXG|S_ISGID|S_IRWXO;
			continue;
		}
		break;
	}
	if (who) {
		clear = who;
	} else {
		clear = S_ISUID|S_ISGID|S_ISVTX|S_IRWXU|S_IRWXG|S_IRWXO;
		who = ~mask;
	}
	while (*p) {
		switch (*p) {
		/* opers */
		case '=':
		case '+':
		case '-':
			op = (int)*p;
			break;
		default:
			eprintf("%s: invalid mode\n", str);
		}

		perm = 0;
		switch (*++p) {
		/* copy */
		case 'u':
			if (mode & S_IRUSR)
				perm |= S_IRUSR|S_IRGRP|S_IROTH;
			if (mode & S_IWUSR)
				perm |= S_IWUSR|S_IWGRP|S_IWOTH;
			if (mode & S_IXUSR)
				perm |= S_IXUSR|S_IXGRP|S_IXOTH;
			if (mode & S_ISUID)
				perm |= S_ISUID|S_ISGID;
			p++;
			break;
		case 'g':
			if (mode & S_IRGRP)
				perm |= S_IRUSR|S_IRGRP|S_IROTH;
			if (mode & S_IWGRP)
				perm |= S_IWUSR|S_IWGRP|S_IWOTH;
			if (mode & S_IXGRP)
				perm |= S_IXUSR|S_IXGRP|S_IXOTH;
			if (mode & S_ISGID)
				perm |= S_ISUID|S_ISGID;
			p++;
			break;
		case 'o':
			if (mode & S_IROTH)
				perm |= S_IRUSR|S_IRGRP|S_IROTH;
			if (mode & S_IWOTH)
				perm |= S_IWUSR|S_IWGRP|S_IWOTH;
			if (mode & S_IXOTH)
				perm |= S_IXUSR|S_IXGRP|S_IXOTH;
			p++;
			break;
		default:
			for (; *p; p++) {
				switch (*p) {
				/* modes */
				case 'r':
					perm |= S_IRUSR|S_IRGRP|S_IROTH;
					break;
				case 'w':
					perm |= S_IWUSR|S_IWGRP|S_IWOTH;
					break;
				case 'x':
					perm |= S_IXUSR|S_IXGRP|S_IXOTH;
					break;
				case 'X':
					if (S_ISDIR(mode) || mode & (S_IXUSR|S_IXGRP|S_IXOTH))
						perm |= S_IXUSR|S_IXGRP|S_IXOTH;
					break;
				case 's':
					perm |= S_ISUID|S_ISGID;
					break;
				case 't':
					perm |= S_ISVTX;
					break;
				default:
					goto apply;
				}
			}
		}

apply:
		/* apply */
		switch (op) {
		case '=':
			mode &= ~clear;
			/* fallthrough */
		case '+':
			mode |= perm & who;
			break;
		case '-':
			mode &= ~(perm & who);
			break;
		}
		/* if we hit a comma, move on to the next clause */
		if (*p == ',') {
			p++;
			goto next;
		}
	}
	return mode & ~S_IFMT;
}
