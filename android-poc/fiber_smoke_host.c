#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

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
}

int main(void)
{
    if (psx_fiber_convert_thread() == NULL) {
        fprintf(stderr, "FAIL: convert_thread returned NULL\n");
        return 10;
    }

    FiberSmokeState state = {0};
    state.caller = psx_fiber_current();
    if (state.caller == NULL) {
        fprintf(stderr, "FAIL: current fiber is NULL\n");
        return 11;
    }

    psx_fiber_t fiber = psx_fiber_create(64u * 1024u, fiber_smoke_entry, &state);
    if (fiber == NULL) {
        fprintf(stderr, "FAIL: create returned NULL\n");
        return 12;
    }

    for (int expected = 1; expected <= 3; ++expected) {
        psx_fiber_switch(fiber);
        if (state.failed != 0) {
            fprintf(stderr, "FAIL: stack marker changed at stage %d\n", state.failed);
            psx_fiber_destroy(fiber);
            return 20 + state.failed;
        }
        if (state.stage != expected) {
            fprintf(stderr, "FAIL: expected stage %d, got %d\n", expected, state.stage);
            psx_fiber_destroy(fiber);
            return 30 + expected;
        }
    }

    psx_fiber_destroy(fiber);
    puts("PASS: AArch64 fiber switching and stack preservation under QEMU");
    return 0;
}
