#ifndef _RISCV_BOARD_H_
#define _RISCV_BOARD_H_

/* Initialize kernel environment. */
void *board_stack(paddr_t dtb_pa, void *dtb_va);

/* Prepare and perform kernel MI initialization. */
void __noreturn board_init(void);

#endif /* !_RISCV_BOARD_H_ */
