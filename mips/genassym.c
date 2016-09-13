#include <assym.h>
#include <thread.h>

ASSYM(TD_USERCTX, offsetof(thread_t, td_userctx));
ASSYM(TD_FRAME, offsetof(thread_t, td_frame));
ASSYM(TD_STATE, offsetof(thread_t, td_state));
