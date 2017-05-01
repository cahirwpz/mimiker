#include <bus.h>

RESOURCE_DECLARE(ioports);

#define PS2_DATA 0x60
#define PS2_STATUS 0x64
#define PS2_COMMAND 0x64

#define PS2_STATUS_OUT_BUF_STATUS 0x01 /* Output buffer status, 0 = empty, 1 = full */
#define PS2_STATUS_IN_BUF_STATUS 0x02 /* Input buffer status, 0 = empty, 1 = full */

/* NOTE: These blocking wait helper functions can't use an interrupt, as the
   PS/2 controller does not generate interrupts for these events. However, this
   is not a major problem, since pretty much always the controller is
   immediately ready to proceed, so the we don't loop in practice. */
static void ps2_ctrl_wait_before_read(){
  uint8_t status;
  do{
    status = bus_space_read_1(ioports, PS2_STATUS);
  }while(!(status & PS2_STATUS_OUT_BUF_STATUS));
}
static void ps2_ctrl_wait_before_write(){
  uint8_t status;
  do{
    status = bus_space_read_1(ioports, PS2_STATUS);
  }while(status & PS2_STATUS_IN_BUF_STATUS);
}

void ps2_ctrl_command_write(uint8_t command){
  ps2_ctrl_wait_before_write();
  bus_space_write_1(ioports, PS2_COMMAND, command);
}

uint8_t ps2_command_write_response(uint8_t command){
  ps2_ctrl_wait_before_write();
  bus_space_write_1(ioports, PS2_COMMAND, command);
  ps2_ctrl_wait_before_read();
  return bus_space_read_1(ioports, PS2_DATA);
}

uint8_t ps2_read_data(){
  return bus_space_read_1(ioports, PS2_DATA);
}

/* For now, this is the only keyboard driver we'll have, so the interface is not
   very flexible. */
void kbd_init(){
  bus_space_write_1(ioports, 0x3c0, 0xff);
  ps2_ctrl_command_write(0x20);
  ps2_ctrl_wait_before_read();
}
