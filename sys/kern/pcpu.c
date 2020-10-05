#include <sys/pcpu.h>
#include <sys/thread.h>

pcpu_t _pcpu_data[1] = {{
  .curthread = &thread0,
}};
