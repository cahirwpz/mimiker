#include <cdev.h>
#include <malloc.h>
#include <string.h>
#include <mutex.h>

cdev_list_t all_cdevs;
static mtx_t all_cdev_mtx;

static MALLOC_DEFINE(cdev_pool, "cdevs pool");

void cdev_init() {
  TAILQ_INIT(&all_cdevs);

  kmalloc_init(cdev_pool);
  kmalloc_add_arena(cdev_pool, pm_alloc(1)->vaddr, PAGESIZE);

  mtx_init(&all_cdev_mtx);

  /* Initialize some core devices */
  dev_null_init();
}

cdev_t *make_dev(cdevsw_t cdevsw, const char *name) {
  cdev_t *cdev = kmalloc(cdev_pool, sizeof(cdev_t), M_ZERO);
  strlcpy(cdev->cdev_name, name, CDEV_NAME_MAX);
  cdev->cdev_sw = cdevsw;

  /* TODO: This function may get called before the threads system is
     initialized. Calling mtx_lock before threads started crashes, because
     thread_self() is not ready yet! See issue #156. Once we fix this, enable
     mtx_lock and mtx_unlock below. */

  /* mtx_lock(&all_cdev_mtx); */
  TAILQ_INSERT_TAIL(&all_cdevs, cdev, cdev_all);
  /* mtx_unlock(&all_cdev_mtx); */
  return NULL;
}
