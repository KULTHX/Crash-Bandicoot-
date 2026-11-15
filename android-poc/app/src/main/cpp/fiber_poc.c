#include <jni.h>
#include <stdint.h>

#include "psx_fiber.h"

typedef struct FiberSmokeState {
    psx_fiber_t caller;
    volatile int stage;
    volatile int failed;
} FiberSmokeState;

static void fiber_smoke_entry(void *opaque)
{
    FiberSmokeState *state = (FiberSmokeState *)opaque;
    volatile uint64_t stack_marker = UINT64_C(0xC3A55A7E1234ABCD);

    state->stage = 1;
    psx_fiber_switch(state->caller);
    if (stack_marker != UINT64_C(0xC3A55A7E1234ABCD)) {
        state->failed = 1;
        state->stage = -1;
        psx_fiber_switch(state->caller);
    }

    state->stage = 2;
    psx_fiber_switch(state->caller);
    if (stack_marker != UINT64_C(0xC3A55A7E1234ABCD)) {
        state->failed = 2;
        state->stage = -2;
        psx_fiber_switch(state->caller);
    }

    state->stage = 3;
    psx_fiber_switch(state->caller);

    /* The caller intentionally destroys this suspended test fiber. */
}

JNIEXPORT jint JNICALL
Java_net_kulthx_nizarcrash3poc_MainActivity_nativeRunFiberSmokeTest(
    JNIEnv *env,
    jobject instance)
{
    (void)env;
    (void)instance;

    if (psx_fiber_convert_thread() == NULL) {
        return -10;
    }

    FiberSmokeState state = {0};
    state.caller = psx_fiber_current();
    if (state.caller == NULL) {
        return -11;
    }

    psx_fiber_t fiber = psx_fiber_create(64u * 1024u, fiber_smoke_entry, &state);
    if (fiber == NULL) {
        return -12;
    }

    for (int expected = 1; expected <= 3; ++expected) {
        psx_fiber_switch(fiber);
        if (state.failed != 0) {
            psx_fiber_destroy(fiber);
            return -20 - state.failed;
        }
        if (state.stage != expected) {
            psx_fiber_destroy(fiber);
            return -30 - expected;
        }
    }

    psx_fiber_destroy(fiber);
    return 0;
}
