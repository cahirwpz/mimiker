#ifndef __ARGS_H__
#define __ARGS_H__

int args_get_flag(const char *name, int argc, char *argv[]);
const char *args_get_value(const char *name, int argc, char *argv[]);

#endif // __ARGS_H__
