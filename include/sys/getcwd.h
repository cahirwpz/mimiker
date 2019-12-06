#ifndef _SYS_GETCWD_H_
#define _SYS_GETCWD_H_

#include <sys/vnode.h>
#include <sys/mimiker.h>

int getcwd_scandir(vnode_t *lvp, vnode_t *uvp, char **bpp, char *bufp);

#endif // !_SYS_GETCWD_H_
