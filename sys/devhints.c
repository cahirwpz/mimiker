#include <device.h>
#include <stdc.h>
#include <bus.h>
#include <fdt.h>
#include <klog.h>
#include <devclass.h>

extern uint8_t __malta_dtb_start[];

void generic_hinted_child(device_t *bus, const void *fdt, int nodeoffset) {
    int proplen;
    char child_path[PATHBUF_LEN];
    char child_path_as_device[PATHBUF_LEN];
    const fdt_property_t *prop_ptr;
    // TODO: find driver here?
    driver_t *driver = NULL;
    // TODO: name, unit will be assigned by devclass
    device_t *child = make_device(bus, driver, "name", 0x42);
    device_construct_fullpath(child, child_path_as_device, PATHBUF_LEN);

    klog("child path as device: %s", child_path_as_device);

    fdt_get_path(fdt, nodeoffset, child_path, PATHBUF_LEN);
    klog("full fdt path: %s", child_path);

    prop_ptr = fdt_getprop(fdt, nodeoffset, "interrupts", &proplen);
    klog("Interrupts: %s, len: %d", prop_ptr->data, proplen);

    prop_ptr = fdt_getprop(fdt, nodeoffset, "reg", &proplen);
    klog("Regs: %s, len: %d", prop_ptr->data, proplen);

    // TODO: Get resources here from device hints (interrupts, regs)
    // using fdt_getprop and then add them to ivars using
    // bus_set_resource (not implemented yet)

    /*
    resource_int_value(name, unit, "port", &start);
    resource_int_value(name, unit, "portsize", &count);
    bus_set_resource(child, SYS_RES_IOPORT, 0, start, count);

    start = 0;
    count = 0;
    resource_int_value(name, unit, "maddr", &start);
    resource_int_value(name, unit, "msize", &count);

    bus_set_resource(child, SYS_RES_MEMORY, 0, start, count);
    bus_set_resource(child, SYS_RES_IRQ, 0, start, 1);
    bus_set_resource(child, SYS_RES_DRQ, 0, start, 1);
    */
    return;
}

void bus_enumerate_hinted_children(device_t *bus) {
    const void *fdt = (const void *)__malta_dtb_start;
    // char bus_path_buff[PATHBUF_LEN];
    // construct_device_path(bus, bus_path_buff, 256);
    const char *path = "/pci@0/isa@1000";

    int bus_nodeoffset = fdt_path_offset(fdt, path);
    int child_nodeoffset;

    FDT_FOR_EACH_SUBNODE(child_nodeoffset, fdt, bus_nodeoffset) {
        bus_hinted_child(bus, fdt, child_nodeoffset);
    }
}
