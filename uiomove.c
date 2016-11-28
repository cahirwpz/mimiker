#include <uio.h>
#include <systm.h>
#include <stdc.h>
#include <vm_map.h>

int main() {
  int res = 0;

  char buffer1[100];
  char buffer2[100];
  const char *text = "Example string with data to be used for i/o operations.";

  /* Initialize buffers. */
  memset(buffer1, '=', sizeof(buffer1));
  memset(buffer2, '=', sizeof(buffer2));

  uio_t uio;
  iovec_t iov[3];

  /* Perform WRITE to buffer1, using text as data. */
  uio.uio_op = UIO_WRITE;
  uio.uio_vmspace = get_kernel_vm_map();
  iov[0].iov_base = (char *)text;
  iov[0].iov_len = 8;
  iov[1].iov_base = (char *)text + 20;
  iov[1].iov_len = 5;
  iov[2].iov_base = (char *)text + 44;
  iov[2].iov_len = 12;
  uio.uio_iovcnt = 3;
  uio.uio_iov = &iov[0];
  uio.uio_offset = 5;
  uio.uio_resid = 8 + 5 + 12;

  res = uiomove(buffer1, sizeof(buffer1), &uio);
  assert(res == 0);
  res = strcmp(buffer1, "=====Example data operations.");
  assert(res == 0);

  /* Now, perform a READ from text, using buffer2 as data. */
  uio.uio_op = UIO_READ;
  uio.uio_vmspace = get_kernel_vm_map();
  iov[0].iov_base = buffer2;
  iov[0].iov_len = 8;
  iov[1].iov_base = buffer2 + 12;
  iov[1].iov_len = 7;
  iov[2].iov_base = buffer2 + 27;
  iov[2].iov_len = 10;
  uio.uio_iovcnt = 3;
  uio.uio_iov = &iov[0];
  uio.uio_offset = 0;
  uio.uio_resid = 8 + 7 + 10;

  res = uiomove((char *)text, strlen(text), &uio);
  assert(res == 0);
  buffer2[37] = 0; /* Manually null-terminate */
  res = strcmp(buffer2, "Example ====string ========with data ");
  assert(res == 0);

  return 0;
}
