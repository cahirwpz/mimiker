#ifndef _AARCH64_FPU_H_
#define _AARCH64_FPU_H_

bool fpu_enabled(void);
void fpu_enable(void);
void fpu_disable(void);
void fpu_save_ctx(user_ctx_t *uctx);
void fpu_load_ctx(user_ctx_t *uctx);

#endif /* !_AARCH64_FPU_H_ */
