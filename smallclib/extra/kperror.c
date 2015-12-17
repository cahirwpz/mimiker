#include <stdio.h>
#include <string.h>
#include <errno.h>

/* Print an error message on standard error. */
void kperror (const char *s)
{
    char *msg;

    if (s != NULL){
        kputs (s);
        kputs (": ");
    }

    msg = strerror (errno);

    if (msg != NULL)
        kputs (msg);

    kputs ("\n");

    return;
}/* perror */
