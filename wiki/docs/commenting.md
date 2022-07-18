# How do we comment our code?

- We use [Markdown](https://guides.github.com/pdfs/markdown-cheatsheet-online.pdf)
  syntax inside C-style comments.

## Tags

Tags are meant to help us mark important things in our code.
When you are about to write a comment that requires special
attention, please add an adequate tag to mark it.

Wildcards used below:

- `?a`: author's GitHub nick
- `?s`: name of source from which code has been taken
- `?t`: term to be described

### List of tags

- `FIXME(?a)`: info about needed fixes, that we are aware about,
   but we leave it be
- `TODO(?a)`: info about some unimplemented features for
   the time being, less obliging than GitHub Issue
- `XXX(?a)`: explanation for some non-obvious part of code,
   perhaps a hack
- `START OF ?s CODE` and `END OF ?s CODE`: marks foreign part of code,
   you **must** leave info about where is it taken from along
   with specific link
- `INFO(?t)`: describes given term, must occur exactly once in the code

### Tags usage examples

- `FIXME(?a)`

```c=
/* Find the first free number for that device.
 * FIXME Not the best solution, but will do for now. */
do {
    snprintf(buf, sizeof(buf), "event%d", unit);
    ret = devfs_makedev(evdev_input_dir, buf, &evdev_vnodeops, evdev, NULL);
    unit++;
} while (ret == EEXIST);
```

[Full code](https://mimiker.ii.uni.wroc.pl/source/xref/mimiker/sys/drv/evdev.c?r=60656d5d#510)

- `TODO(?a)`

```c=
char *ttyname(int fd) {
   /* TODO(fzdob): to implement */
   errno = ENOTTY;
   return NULL;
 }
```

```c=
static void cbus_uart_init(console_t *dev __unused) {
    /* TODO(pj) This resource allocation should be done in parent of
     * cbus_uart device. Unfortunately now we don't have fully working device
     * infrastructure. It should be changed after done with DEVCLASS. */
    vaddr_t handle = kmem_map_contig(MALTA_FPGA_BASE, PAGESIZE, PMAP_NOCACHE);
    cbus_uart->r_bus_handle = handle + MALTA_CBUS_UART_OFFSET;
  
    set(LCR, LCR_DLAB);
    out(DLM, 0);
    out(DLL, 1); /* 115200 */
    clr(LCR, LCR_DLAB);
  
    out(IER, 0);
    out(FCR, 0);
    out(LCR, LCR_8BITS); /* 8-bit data, no parity */
  }
```

```c=
static intr_filter_t mips_timer_intr(void *data) {
    device_t *dev = data;
    mips_timer_state_t *state = dev->state;
    /* TODO(cahir): can we tell scheduler that clock ticked more than once? */
    (void)set_next_tick(state);
    tm_trigger(&state->timer);
    return IF_FILTERED;
  }
```

```c=
/* TODO(cahir): revisit this after off_t is changed to int64_t */
    if ((unsigned long)pos > LONG_MAX) {
      errno = EOVERFLOW;
      return -1L;
    }
```

- `XXX(?a)`

```c=
/* XXX: It's still possible for periods to be lost.
* For example disabling interrupts for the whole period
* without calling pit_gettime will lose period_ticks.
* It is also possible that time suddenly jumps by period_ticks
* due to the fact that pit_update_time() can't detect an overflow if
* the current counter value is greater than the previous one, while
* pit_intr() can thanks to the noticed_overflow flag. */
pit_update_time(pit);
if (!pit->noticed_overflow)
 pit_incr_ticks(pit, pit->period_ticks);
tm_trigger(&pit->timer);
```

[Context for the one below](https://mimiker.ii.uni.wroc.pl/source/xref/mimiker/lib/libc/stdio/vfscanf.c?r=10da8877#919)

```c
#if 1 /* XXX another disgusting compatibility hack */
```

- `START OF ?s CODE` / `END OF ?s CODE`

[Example](https://mimiker.ii.uni.wroc.pl/source/xref/mimiker/sys/kern/tty.c?r=7a5a999c#26)

- `INFO(?a)`

<!-- TODO(hadarai) add INFO example -->
