Mimiker-related scripts
---

Device hints
---

Provide hints for device resources using FDT (Flat Device Tree) syntax.
Supported keywords:

    interrupts - device's IRQ
    iomem - list of iomem ranges
    ioport - list of ioport ranges

For an exemplary file please take a look at `drv/device_hints.dts`.

Generating generate device hints from DTS file:

1. Install requirements

    `pip3 install -r requirements.txt`

2. Generate C array

    `./gen_device_hints.sh`


