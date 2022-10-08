#include <sys/select.h>
#include <sys/time.h>
#include <sys/event.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

int pselect(int nfds, fd_set *readfds, fd_set *writefds,
            fd_set *exceptfds, const struct timespec *timeout,
            const sigset_t *sigmask) {
    return 0;
}

int select(int nfds, fd_set *restrict readfds, fd_set *restrict writefds,
           fd_set *restrict exceptfds, struct timeval *restrict timeout) {
    int kq;
    int ret;
    struct timespec timeout_ts;
    struct kevent *events;
    int nevents = 0;

    if (nfds < 0) {
        errno = EINVAL;
        return -1;
    }

    if (timeout != NULL) {
        if (timeout->tv_sec < 0 || timeout->tv_usec < 0 || timeout->tv_usec >= 1000000) {
            errno = EINVAL;
            return -1;
        }

        tv2ts(timeout, &timeout_ts);
    }

    kq = kqueue();
    if (kq < 0)
        return -1;

    events = malloc(2 * nfds * sizeof(struct kevent));
    if (!events) {
        errno = ENOMEM;
        ret = -1;
        goto close_kq;
    }

    for (int i = 0; i < nfds; i++) {
        if (readfds != NULL && FD_ISSET(i, readfds))
            EV_SET(&events[nevents++], i, EVFILT_READ, EV_ADD, 0, 0, 0);
        if (writefds != NULL && FD_ISSET(i, writefds))
            EV_SET(&events[nevents++], i, EVFILT_WRITE, EV_ADD, 0, 0, 0);
    }

    if (readfds != NULL)
        FD_ZERO(readfds);
    if (writefds != NULL)
        FD_ZERO(writefds);
    
    ret = kevent(kq, events, nevents, events, nevents, timeout == NULL ? NULL : &timeout_ts);
    if (ret == -1)
        goto end;

    for (int i = 0; i < ret; i++) {
        if (events[i].flags == EV_ERROR) {
            errno = events[i].data;
            ret = -1;
            goto end;
        }
        if (events[i].filter == EVFILT_READ)
            FD_SET(events[i].ident, readfds);
        if (events[i].filter == EVFILT_WRITE)
            FD_SET(events[i].ident, writefds);
    }

end:
    free(events);
close_kq:
    close(kq);
    return ret;
}
