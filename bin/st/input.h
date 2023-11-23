#ifndef INPUT_H
#define INPUT_H

int input_init(const char *ev_dev);
void input_read(void);
int input_getfd(void);

#endif