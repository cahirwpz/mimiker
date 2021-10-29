#include <sbi/riscv_asm.h>
#include <sbi/riscv_encoding.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_const.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_string.h>
#include <sbi/sbi_types.h>
#include <sbi_utils/ipi/aclint_mswi.h>
#include <sbi_utils/irqchip/plic.h>
#include <sbi_utils/serial/uart8250.h>
#include <sbi_utils/timer/aclint_mtimer.h>

#include "litex.h"

/*
 * PLIC parameters.
 */
#define VEX_PLIC_ADDR		0xf1000000
#define VEX_PLIC_NUM_SOURCES	16
#define VEX_HART_COUNT		1

/*
 * CLINT parameters.
 */
#define VEX_CLINT_ADDR		0xf0010000
#define VEX_ACLINT_MTIMER_FREQ	1000000
#define VEX_ACLINT_MSWI_ADDR	(VEX_CLINT_ADDR + CLINT_MSWI_OFFSET)
#define VEX_ACLINT_MTIMER_ADDR	(VEX_CLINT_ADDR + CLINT_MTIMER_OFFSET)

static struct plic_data plic = {
	.addr = VEX_PLIC_ADDR,
	.num_src = VEX_PLIC_NUM_SOURCES,
};

static struct aclint_mswi_data mswi = {
	.addr = VEX_ACLINT_MSWI_ADDR,
	.size = ACLINT_MSWI_SIZE,
	.first_hartid = 0,
	.hart_count = VEX_HART_COUNT,
};

static struct aclint_mtimer_data mtimer = {
	.mtime_freq = VEX_ACLINT_MTIMER_FREQ,
	.mtime_addr = VEX_ACLINT_MTIMER_ADDR + ACLINT_DEFAULT_MTIME_OFFSET,
	.mtime_size = ACLINT_DEFAULT_MTIME_SIZE,
	.mtimecmp_addr = VEX_ACLINT_MTIMER_ADDR +
			 ACLINT_DEFAULT_MTIMECMP_OFFSET,
	.mtimecmp_size = ACLINT_DEFAULT_MTIMECMP_SIZE,
	.first_hartid = 0,
	.hart_count = VEX_HART_COUNT,
	.has_64bit_mmio = TRUE,
};

static struct sbi_console_device console = {
	.console_putc = vex_putc,
	.console_getc = vex_getc,
};

static int vex_early_init(bool cold_boot)
{
	return 0;
}

static int vex_final_init(bool cold_boot)
{
	return 0;
}

static int vex_console_init(void)
{
	const char *name = "LiteX UART";
	size_t size = MIN(sbi_strlen(name), sizeof(console.name) - 1);
	sbi_memcpy(console.name, name, size);
	sbi_console_set_device(&console);
	return 0;
}

static int vex_irqchip_init(bool cold_boot)
{
	u32 hartid = current_hartid();
	int ret;

	if (cold_boot) {
		ret = plic_cold_irqchip_init(&plic);
		if (ret)
			return ret;
	}

	return plic_warm_irqchip_init(&plic, 2 * hartid, 2 * hartid + 1);
}

static int vex_ipi_init(bool cold_boot)
{
	int ret;

	if (cold_boot) {
		ret = aclint_mswi_cold_init(&mswi);
		if (ret)
			return ret;
	}

	return aclint_mswi_warm_init();
}

static int vex_timer_init(bool cold_boot)
{
	int ret;

	if (cold_boot) {
		ret = aclint_mtimer_cold_init(&mtimer, NULL);
		if (ret)
			return ret;
	}

	return aclint_mtimer_warm_init();
}

/*
 * VexRiscv platform description.
 */
const struct sbi_platform_operations platform_ops = {
	.early_init 	= vex_early_init,
	.final_init	= vex_final_init,
	.console_init	= vex_console_init,
	.irqchip_init	= vex_irqchip_init,
	.ipi_init	= vex_ipi_init,
	.timer_init	= vex_timer_init,
};

const struct sbi_platform platform = {
	.opensbi_version	= OPENSBI_VERSION,
	.platform_version	= SBI_PLATFORM_VERSION(0x0, 0x01),
	.name			= "LiteX VexRiscv",
	.features		= SBI_PLATFORM_DEFAULT_FEATURES,
	.hart_count		= VEX_HART_COUNT,
	.platform_ops_addr	= (unsigned long)&platform_ops,
};
