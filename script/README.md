Mimiker-related scripts
---

Device hints
---

Provide hints for device resources using FDT (Flat Device Tree) syntax.
Supported keywords:

 * `interrupts`: device's IRQ
 * `iomem`: list of iomem ranges
 * `ioport`: list of ioport ranges

For an example file please take a look at `mips/malta.dts`.

To build devhints use `gendevhint.py` script as follows:
```
./gendevhint.py example.dts example-devhint.c
```
