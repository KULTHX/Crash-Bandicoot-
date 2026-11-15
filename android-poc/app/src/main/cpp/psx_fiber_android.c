/*
 * Android AArch64 fiber backend for the PSXRecomp psx_fiber API.
 *
 * Android's Bionic exposes ucontext types but does not expose the
 * getcontext/makecontext/swapcontext function family to NDK applications.
 * This backend preserves the AAPCS64 callee-saved register set and stack
 * pointer directly, without relying on those unavailable functions.
 *
 * This file is a local Android port/proof of concept. It does not modify
 * the upstream PSXRecomp source tree.
 */
#include "psx_fiber.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#if !defined(__aarch64__) || (!defined(__ANDROID__) && !defined(PSX_FIBER_ALLOW_HOST_TEST))
#error "This fiber backend requires Android AArch64 (or the explicit AArch64 host test build)."
#endif

typedef struct __attribute__((aligned(16))) psx_fiber_context {
    /* x19..x30, in ascending register order. */
    uint64_t x19_x30[12];
    /* Current stack pointer. */
    uintptr_t sp;
    /* AAPCS64 callee-saved floating-point registers d8..d15. */
    uint64_t d8_d15[8];
} psx_fiber_context;

_Static_assert(offsetof(psx_fiber_context, x19_x30) == 0,
               "Unexpected GPR context offset");
_Static_assert(offsetof(psx_fiber_context, sp) == 96,
               "Unexpected SP context offset");
_Static_assert(offsetof(psx_fiber_context, d8_d15) == 104,
               "Unexpected FP context offset");

typedef struct psx_fiber_impl {
    psx_fiber_context context;
    void *stack;
    psx_fiber_entry entry;
    void *arg;
} psx_fiber_impl;

static _Thread_local psx_fiber_impl *s_current_fiber = NULL;

extern void psx_fiber_swap_context(psx_fiber_context *from,
                                  const psx_fiber_context *to);
extern void psx_fiber_start_trampoline(void);

/* Called by the assembly trampoline after the first switch into a new fiber. */
void psx_fiber_run_from_assembly(void *opaque)
{
    psx_fiber_impl *fiber = (psx_fiber_impl *)opaque;
    if (fiber == NULL || fiber->entry == NULL) {
        abort();
    }

    fiber->entry(fiber->arg);

    /* A PS1 BIOS/game fiber must switch away rather than return. */
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

psx_fiber_t psx_fiber_create(size_t stack_size,
                             psx_fiber_entry entry,
                             void *arg)
{
    if (entry == NULL) {
        return NULL;
    }

    /* Keep a conservative floor; callers may request a larger PS1 stack. */
    if (stack_size < 16u * 1024u) {
        stack_size = 16u * 1024u;
    }

    psx_fiber_impl *fiber = (psx_fiber_impl *)calloc(1, sizeof(*fiber));
    if (fiber == NULL) {
        return NULL;
    }

    fiber->stack = malloc(stack_size);
    if (fiber->stack == NULL) {
        free(fiber);
        return NULL;
    }

    fiber->entry = entry;
    fiber->arg = arg;

    /* AAPCS64 requires SP to be 16-byte aligned at public call boundaries. */
    uintptr_t stack_top = (uintptr_t)fiber->stack + stack_size;
    stack_top &= ~(uintptr_t)0x0Fu;
    fiber->context.sp = stack_top;

    /* x19 is callee-saved and carries the fiber pointer into the trampoline. */
    fiber->context.x19_x30[0] = (uint64_t)(uintptr_t)fiber;
    /* Restoring a new context returns directly into the bootstrap trampoline. */
    fiber->context.x19_x30[11] =
        (uint64_t)(uintptr_t)&psx_fiber_start_trampoline;

    return (psx_fiber_t)fiber;
}

void psx_fiber_switch(psx_fiber_t target)
{
    psx_fiber_impl *to = (psx_fiber_impl *)target;
    psx_fiber_impl *from = s_current_fiber;

    if (from == NULL || to == NULL || from == to) {
        return;
    }

    s_current_fiber = to;
    psx_fiber_swap_context(&from->context, &to->context);
    /* The fiber that switched back sets s_current_fiber before restoring us. */
}

void psx_fiber_destroy(psx_fiber_t opaque)
{
    psx_fiber_impl *fiber = (psx_fiber_impl *)opaque;
    if (fiber == NULL || fiber == s_current_fiber) {
        return;
    }

    free(fiber->stack);
    free(fiber);
}
