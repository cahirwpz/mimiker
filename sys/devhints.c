#include <device.h>
#include <stdc.h>
#include <bus.h>
#include <fdt.h>
#include <klog.h>

extern uint8_t __dtb_start[];

/*
void construct_device_path(device_t *dev, char* buff, size_t buff_size) {
    // TODO: https://github.com/cahirwpz/mimiker/pull/491/ introduces name and unit fields for device
    if (dev == NULL) return;
    snprintf((buff), buff_size, "%s%d/%s", dev->name, dev->unit, buff);
    construct_device_path(dev->parent, buff, buff_size);
}
*/

void generic_hinted_child(device_t *bus, const void *fdt, int nodeoffset) {
    /* skeleton of a FreeBSD solution for isa_hinted_child; needs to be adjusted */

    char child_path[100];
    fdt_get_path(fdt, nodeoffset, child_path, PATHBUF_LEN);
    klog("full path: %s", child_path);

    int proplen;
    const fdt_property_t *prop_ptr;
    prop_ptr = fdt_getprop(fdt, nodeoffset, "interrupts", &proplen);
    klog("Interrupts: %s, len: %d", prop_ptr->data, proplen);

    prop_ptr = fdt_getprop(fdt, nodeoffset, "reg", &proplen);
    klog("Regs: %s, len: %d", prop_ptr->data, proplen);

    // TODO: do we have BUS_ADD_CHILD equivalent?
    /*
    device_t child;
    int start, count;
    int order;

    child = BUS_ADD_CHILD(parent, order, name, unit);
    if (child == 0)
        return;

    resource_int_value(name, unit, "port", &start);
    resource_int_value(name, unit, "portsize", &count);
    if (start > 0 || count > 0)
        bus_set_resource(child, SYS_RES_IOPORT, 0, start, count);

    start = 0;
    count = 0;
    resource_int_value(name, unit, "maddr", &start);
    resource_int_value(name, unit, "msize", &count);

    if (start > 0 || count > 0)
        bus_set_resource(child, SYS_RES_MEMORY, 0, start, count);
    if (resource_int_value(name, unit, "irq", &start) == 0 && start > 0)
        bus_set_resource(child, SYS_RES_IRQ, 0, start, 1);
    if (resource_int_value(name, unit, "drq", &start) == 0 && start >= 0)
        bus_set_resource(child, SYS_RES_DRQ, 0, start, 1);

    */
    return;
}

void bus_enumerate_hinted_children(device_t *bus) {
    const void *fdt = (const void *)__dtb_start;
    // TODO: path should be constructed here, but name and unit are included in
    // https://github.com/cahirwpz/mimiker/pull/491/
    // char bus_path_buff[PATHBUF_LEN];
    // construct_device_path(bus, bus_path_buff, 256);
    const char *path = "/pci@0/isa@1000";

    int bus_nodeoffset = fdt_path_offset(fdt, path);
    int child_nodeoffset;

    fdt_for_each_subnode(child_nodeoffset, fdt, bus_nodeoffset) {
        bus_hinted_child(bus, fdt, child_nodeoffset);
    }
}
