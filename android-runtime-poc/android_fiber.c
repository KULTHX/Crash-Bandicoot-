/*
 * Android AArch64 cooperative fiber backend for PSXRecomp.
 *
 * Bionic's NDK headers do not expose the POSIX getcontext/makecontext/
 * swapcontext API used by the desktop implementation. This backend provides
 * the existing psx_fiber.h contract using the AAPCS64 callee-saved context.
 */
#include "psx_fiber.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#if !defined(__ANDROID__) || !defined(__aarch64__)
#error "Android fiber backend requires Android AArch64."
#endif

typedef struct __attribute__((aligned(16))) {
    uint64_t x19_x30[12];
    uintptr_t sp;
    uint64_t d8_d15[8];
} psx_fiber_context;

_Static_assert(offsetof(psx_fiber_context, sp) == 96, "PSX fiber SP offset");
_Static_assert(offsetof(psx_fiber_context, d8_d15) == 104, "PSX fiber FP offset");

typedef struct psx_fiber_impl {
    psx_fiber_context context;
    void *stack;
    psx_fiber_entry entry;
    void *arg;
} psx_fiber_impl;

static _Thread_local psx_fiber_impl *s_current_fiber;

extern void psx_fiber_swap_context(psx_fiber_context *from,
                                  const psx_fiber_context *to);
extern void psx_fiber_start_trampoline(void);

void psx_fiber_run_from_assembly(void *opaque)
{
    psx_fiber_impl *fiber = (psx_fiber_impl *)opaque;
    if (fiber == NULL || fiber->entry == NULL) abort();
    fiber->entry(fiber->arg);
    abort();
}

psx_fiber_t psx_fiber_convert_thread(void)
{
    if (s_current_fiber == NULL) {
        s_current_fiber = (psx_fiber_impl *)calloc(1, sizeof(*s_current_fiber));
    }
    return (psx_fiber_t)s_current_fiber;
}

psx_fiber_t psx_fiber_current(void)
{
    return (psx_fiber_t)s_current_fiber;
}

psx_fiber_t psx_fiber_create(size_t stack_size, psx_fiber_entry entry, void *arg)
{
    if (entry == NULL) return NULL;
    if (stack_size < 16u * 1024u) stack_size = 16u * 1024u;

    psx_fiber_impl *fiber = (psx_fiber_impl *)calloc(1, sizeof(*fiber));
    if (fiber == NULL) return NULL;
    fiber->stack = malloc(stack_size);
    if (fiber->stack == NULL) {
        free(fiber);
        return NULL;
    }

    fiber->entry = entry;
    fiber->arg = arg;

    uintptr_t stack_top = (uintptr_t)fiber->stack + stack_size;
    stack_top &= ~(uintptr_t)0x0Fu;
    fiber->context.sp = stack_top;

    /* x19 survives the context switch and passes the fiber to the bootstrap. */
    fiber->context.x19_x30[0] = (uint64_t)(uintptr_t)fiber;
    fiber->context.x19_x30[11] =
        (uint64_t)(uintptr_t)&psx_fiber_start_trampoline;

    return (psx_fiber_t)fiber;
}

void psx_fiber_switch(psx_fiber_t target)
{
    psx_fiber_impl *to = (psx_fiber_impl *)target;
    psx_fiber_impl *from = s_current_fiber;
    if (from == NULL || to == NULL || from == to) return;

    s_current_fiber = to;
    psx_fiber_swap_context(&from->context, &to->context);
    /* The resumed fiber is made current by the fiber switching back to it. */
}

void psx_fiber_destroy(psx_fiber_t opaque)
{
    psx_fiber_impl *fiber = (psx_fiber_impl *)opaque;
    if (fiber == NULL || fiber == s_current_fiber) return;
    free(fiber->stack);
    free(fiber);
}
