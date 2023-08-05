#include<sys/poll.h>
#include<sys/time.h>
#include<sys/event.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>

int	poll(struct pollfd *fds, nfds_t nfds, int timeout) {
    int kq;
    int ret;
    struct kevent *events;
    struct timespec timeout_ts;
    int nevents = 0;

    if (nfds > (unsigned int)sysconf(_SC_OPEN_MAX)) {
        errno = EINVAL;
        return -1;
    }

    if (timeout >= 0) {
        timeout_ts.tv_sec = timeout / 1000;
        timeout_ts.tv_nsec = (timeout % 1000) * 1000000;
    }
    /* if timeout == -1, then poll should wait indefinitely */
    else if (timeout < -1) {
        errno = EINVAL;
        return -1;
    }

    kq = kqueue();
    if (kq < 0)
        return -1;

    events = malloc(nfds * sizeof(struct kevent));
    if (!events) {
        errno = ENOMEM;
        ret = -1;
        goto close_kq;
    }

    for (nfds_t i = 0; i < nfds; i++) {
        if (fds[i].events & POLLIN)
            EV_SET(&events[nevents++], fds[i].fd, EVFILT_READ, EV_ADD, 0, 0, &fds[i]);
        if (fds[i].events & POLLOUT)
            EV_SET(&events[nevents++], fds[i].fd, EVFILT_WRITE, EV_ADD, 0, 0, &fds[i]);
    }

    ret = kevent(kq, events, nevents, events, nevents, timeout == -1 ? NULL : &timeout_ts);
    if (ret == -1)
        goto end;

    for (int i = 0; i < ret; i++) {
        struct pollfd *pfd = (struct pollfd*)events[i].udata;

        if (events[i].flags & EV_ERROR) {
            errno = events[i].data;
            if (errno == EBADF)
                pfd->revents |= POLLNVAL;
            else
                pfd->revents |= POLLERR;
        }
        else if (events[i].filter == EVFILT_READ) 
            pfd->revents |= POLLIN;
        else if (events[i].filter == EVFILT_WRITE)
            pfd->revents |= POLLOUT;
    }

end:
    free(events);
close_kq:
    close(kq);
    return ret;
}
