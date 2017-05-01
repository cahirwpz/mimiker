#include <bus.h>
#include <stdc.h>
#include <thread.h>
#include <sched.h>
#include <vnode.h>
#include <devfs.h>

RESOURCE_DECLARE(ioports);

#define PS2_DATA 0x60
#define PS2_STATUS 0x64
#define PS2_COMMAND 0x64

#define PS2_STATUS_OUT_BUF_STATUS                                              \
  0x01 /* Output buffer status, 0 = empty, 1 = full */
#define PS2_STATUS_IN_BUF_STATUS                                               \
  0x02 /* Input buffer status, 0 = empty, 1 = full */

#define PS2_KBD_ACK 0xfa
#define PS2_KBD_SELF_TEST_OK 0xaa

#define PS2_KBD_CMD_RESET 0xff
#define PS2_KBD_CMD_SCAN_ENABLE 0xf4
#define PS2_KBD_CMD_SCAN_DISABLE 0xf5
#define PS2_KBD_CMD_SET_SCANCODE_SET 0xf0

/* NOTE: These blocking wait helper functions can't use an interrupt, as the
   PS/2 controller does not generate interrupts for these events. However, this
   is not a major problem, since pretty much always the controller is
   immediately ready to proceed, so the we don't loop in practice. */
static void ps2_ctrl_wait_before_read() {
  uint8_t status;
  do {
    status = bus_space_read_1(ioports, PS2_STATUS);
  } while (!(status & PS2_STATUS_OUT_BUF_STATUS));
}
static void ps2_ctrl_wait_before_write() {
  uint8_t status;
  do {
    status = bus_space_read_1(ioports, PS2_STATUS);
  } while (status & PS2_STATUS_IN_BUF_STATUS);
}

void ps2_ctrl_command_write(uint8_t command) {
  ps2_ctrl_wait_before_write();
  bus_space_write_1(ioports, PS2_COMMAND, command);
}

uint8_t ps2_ctrl_command_write_response(uint8_t command) {
  ps2_ctrl_wait_before_write();
  bus_space_write_1(ioports, PS2_COMMAND, command);
  ps2_ctrl_wait_before_read();
  return bus_space_read_1(ioports, PS2_DATA);
}

uint8_t ps2_ctrl_read_data() {
  ps2_ctrl_wait_before_read();
  return bus_space_read_1(ioports, PS2_DATA);
}

void ps2_dev1_send_byte(uint8_t byte) {
  ps2_ctrl_wait_before_write();
  bus_space_write_1(ioports, PS2_DATA, byte);
}

uint8_t ps2_dev1_read_byte() {
  ps2_ctrl_wait_before_read();
  return bus_space_read_1(ioports, PS2_DATA);
}

#define SCANCODE_BUFFER_SIZE 128
/* TODO: This should be a cyclic buffer. */
static uint8_t scancode_buffer[SCANCODE_BUFFER_SIZE];
static unsigned scancode_buffer_n = 0;
static mtx_t scancode_buffer_mtx;

static void kbd_reader_thread(void *arg) {
  while (1) {
    /* TODO: Some locking mechanism will be necessary if we want to sent extra
       commands to the ps/2 controller and/or other ps/2 devices, because this
       thread would interfere with commands/responses. */
    /* TODO: This should be driven with an interrupt. */
    uint8_t code, code2 = 0;
    /* See if there is data to read. */
    uint8_t status = bus_space_read_1(ioports, PS2_STATUS);
    if(!(status & PS2_STATUS_IN_BUF_STATUS)){
      /* No data is available. Yield to save CPU time. */
      sched_yield();
      continue;
    }
    code = ps2_dev1_read_byte();
    int is_extended = (code == 0xe0); /* Extended scancode. */

    if (code == 0) /* Device buffer overflow. */
      continue;

    if (is_extended)
      code2 = ps2_dev1_read_byte();

    mtx_scoped_lock(&scancode_buffer_mtx);
    /* Ensure there are 2 bytes of space in the buffer. */
    if (scancode_buffer_n >= SCANCODE_BUFFER_SIZE - 1)
      continue;

    /* Instead of convoluted logic for processing extended scancodes differently
       than simple scancodes, just store both bytes even when the other one is
       bogus. */
    scancode_buffer[scancode_buffer_n + 0] = code;
    scancode_buffer[scancode_buffer_n + 1] = code2;

    scancode_buffer_n += is_extended ? 2 : 1;
  }
}

static int dev_scancode_read(vnode_t *v, uio_t *uio) {
  uio->uio_offset = 0; /* This device does not support offsets. */
  mtx_scoped_lock(&scancode_buffer_mtx);
  if (scancode_buffer_n == 0)
    return 0;
  int error = uiomove_frombuf(scancode_buffer, scancode_buffer_n, uio);
  if (error)
    return error;
  scancode_buffer_n = 0;
  return 0;
}

vnodeops_t dev_scancode_ops = {
  .v_lookup = vnode_op_notsup,
  .v_readdir = vnode_op_notsup,
  .v_open = vnode_open_generic,
  .v_write = vnode_op_notsup,
  .v_read = dev_scancode_read,
};

/* For now, this is the only keyboard driver we'll want to have, so the
   interface is not very flexible. */
void kbd_init() {
  uint8_t response;
  /* Reset keyboard and perform a self-test. */
  ps2_dev1_send_byte(PS2_KBD_CMD_RESET);
  response = ps2_dev1_read_byte();
  assert(response == PS2_KBD_ACK);
  response = ps2_dev1_read_byte();
  if (response != PS2_KBD_SELF_TEST_OK) {
    /* This may indicate a configuration error. */
    log("Keyboard self-test failed.");
    return;
  }

  /* Enable scancode table #1 */
  ps2_dev1_send_byte(PS2_KBD_CMD_SET_SCANCODE_SET);
  ps2_dev1_send_byte(1);
  response = ps2_dev1_read_byte();
  assert(response == PS2_KBD_ACK);

  /* Prepare buffer thread. */
  mtx_init(&scancode_buffer_mtx, MTX_DEF);
  thread_t *kbd_thread = thread_create("kbd reader", kbd_reader_thread, NULL);

  /* Start scanning. */
  ps2_dev1_send_byte(PS2_KBD_CMD_SCAN_ENABLE);
  response = ps2_dev1_read_byte();
  assert(response == PS2_KBD_ACK);

  sched_add(kbd_thread);

  /* Prepare /dev/scancode interface. */
  vnode_t *dev_scancode_device = vnode_new(V_DEV, &dev_scancode_ops);
  devfs_install("scancode", dev_scancode_device);
}
