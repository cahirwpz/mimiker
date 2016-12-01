#ifndef __CDEV_H__
#define __CDEV_H__

#include <uio.h>

typedef struct cdev cdev_t;

#define CDEV_NAME_MAX 30

typedef int d_open_t(cdev_t *dev, int flags);
typedef int d_close_t(cdev_t *dev);
typedef int d_read_t(cdev_t *dev, uio_t *uio, int flags);
typedef int d_write_t(cdev_t *dev, uio_t *uio, int flags);

/* Character device switch table */
typedef struct cdevsw {
  d_open_t *d_open;
  d_close_t *d_close;
  d_read_t *d_read;
  d_write_t *d_write;
} cdevsw_t;

typedef struct cdev {
  /* This implementation of cdev structure is just a stub. Other fields that
     might be added: flags, user id, group id, reference count, pointer to
     parent device, list of children devices, driver-specific data, mountpoint,
     access/create timestamps. */
  char cdev_name[CDEV_NAME_MAX];
  struct cdevsw cdev_sw;
  TAILQ_ENTRY(cdev) cdev_all; /* a link on all devices queue */
} cdev_t;

/* List of all character devices present in the system */
typedef TAILQ_HEAD(, cdev) cdev_list_t;
extern cdev_list_t all_cdevs;

/* Initializes the character devices subsystem */
void cdev_init();

/* Allocates and initializes a new character device */
cdev_t *make_dev(cdevsw_t cdevsw, const char *name);

/* Initializes null and zero devices */
void dev_null_init();

#endif /* __CDEV_H__ */
