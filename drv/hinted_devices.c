#include <device.h>
#include "device_hints.h"

const char* construct_device_path(device_t *dev) {
    // TODO
    return "/";
}

void bus_enumerate_hinted_children(device_t *bus) {
    const char* bus_path = construct_device_path(bus);

    // TODO: C ain't no python
    for hint in filter(lambda hint: hint.path.startswith(bus_path), hints) {
        bus_hinted_child(bus, hint);
    }
}


static void generic_hinted_child(device_t *bus, devhint_t *devhint) {
    // TODO: include devhint_t
    // TODO: probably a wrong file for that
    device_t child;
    int start, count;
    int order;

    /* skeleton of a FreeBSD solution for isa_hintedd_child; needs to be adjusted */

    // TODO: do we have BUS_ADD_CHILD equivalent?
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

    return;
}

