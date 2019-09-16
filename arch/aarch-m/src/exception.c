/*
 * Copyright (c) [2019] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * LiteOS NOVA is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan PSL v1.
 * You may obtain a copy of Mulan PSL v1 at:
 *
 * 	http://license.coscl.org.cn/MulanPSL
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v1 for more details.
 */

#include <config.h>
#include <defer.h>
#include <task.h>
#include <kprintf.h>
#include <bug.h>
#include <critical.h>
#include <memtry.h>

#include <arch/exc.h>
#include <arch/spr.h>
#include <arch/trace.h>

extern char mem_try_arch [];
extern char mem_try_arch_end [];

static void __exc_task_delete (struct deferred_job * job)
    {
    task_id task = container_of (job, task_t, job);

    (void) task_delete (task);
    }

/*
 * exc_trace - show the stack trace info for an exception
 * exc_info: the exception information struct
 *
 * return: NA
 */

static void exc_trace (exc_info_t * exc_info)
    {
    uint32_t sp = (uint32_t) (exc_info->esp + 1);

    if (exc_info->esp->epsr & 0x200)
        {
        sp += 4;
        }

    back_trace (sp, exc_info->esp->lr, exc_info->esp->pc);
    }

void arch_exc_handler (exc_info_t * context)
    {
    uint32_t         pc = context->esp->pc;
    deferred_job_t * job;
    uint32_t       * sp = (uint32_t *) (context->esp + 1);
#ifdef CONFIG_AARCH_M_MAINLINE
    const char     * actived;
#endif

#ifdef CONFIG_AARCH_M_MAINLINE
    context->shcsr = scb->shcsr;

    context->cfsr  = 0;
    context->hfsr  = 0;
    context->mmfar = 0;
    context->bfar  = 0;

    /* get and clean dfsr */

    context->dfsr  = scb->dfsr;

    scb->dfsr      = context->dfsr;

    if (context->shcsr &
        (SHCSR_MEMFAULTPENDED | SHCSR_BUSFAULTPENDED | SHCSR_USGFAULTPENDED |
         SHCSR_MEMFAULTACT    | SHCSR_BUSFAULTACT    | SHCSR_USGFAULTACT))
        {
        context->cfsr = scb->cfsr;

        scb->cfsr     = context->cfsr;     /* clean the register */

        if (context->cfsr & CFSR_MMARVALID)
            {
            context->mmfar = scb->mmfar;
            }

        if (context->cfsr & CFSR_BFARVALID)
            {
            context->bfar  = scb->bfar;
            }
        }

    if (__get_ipsr () == EXC_VEC_HARDFAULT)
        {
        context->hfsr = scb->hfsr;
        scb->hfsr     = context->hfsr;
        }
#endif

    if ((pc > (uint32_t) mem_try_arch) && (pc <= (uint32_t) mem_try_arch_end))
        {
        context->esp->r0 = (uint32_t) -1;
        context->esp->pc = (uint32_t) mem_try_arch_end;
        return;
        }

    kprintf ("\nencountered exception, the exception context:\n");
    kprintf ("r0  = %p    r1  = %p\n", context->esp->r0, context->esp->r1);
    kprintf ("r2  = %p    r3  = %p\n", context->esp->r2, context->esp->r3);
    kprintf ("r4  = %p    r5  = %p\n", context->r4,      context->r5);
    kprintf ("r6  = %p    r7  = %p\n", context->r6,      context->r7);
    kprintf ("r8  = %p    r9  = %p\n", context->r8,      context->r9);
    kprintf ("r10 = %p    r11 = %p\n", context->r10,     context->r11);
    kprintf ("r12 = %p    sp  = %p\n", context->esp->ip, sp);
    kprintf ("lr  = %p    pc  = %p\n", context->esp->lr, context->esp->pc);

#ifdef CONFIG_AARCH_M_MAINLINE
    switch (context->shcsr & (SHCSR_SVCALLACT   | SHCSR_USGFAULTACT |
                              SHCSR_BUSFAULTACT | SHCSR_MEMFAULTACT))
        {
        case SHCSR_SVCALLACT:
            actived = "SVCALL";
            break;
        case SHCSR_USGFAULTACT:
            actived =  "USG";
            break;
        case SHCSR_BUSFAULTACT:
            actived =  "BUS";
            break;
        case SHCSR_MEMFAULTACT:
            actived =  "MEM";
            break;
        default:
            /* should be hardfault */
            actived =  "HARD";
            break;
        }

    kprintf ("\nshcsr = %p, ACTIVED: %s, PENDED: SVCALL (%d), BUS (%d), MEM (%d), USG (%d)\n\n",
             context->shcsr, actived,
             !!(context->shcsr & SHCSR_SVCALLPENDED),
             !!(context->shcsr & SHCSR_BUSFAULTPENDED),
             !!(context->shcsr & SHCSR_MEMFAULTPENDED),
             !!(context->shcsr & SHCSR_USGFAULTPENDED));

    kprintf ("cfsr  = %p\n", context->cfsr);
    kprintf ("hfsr  = %p\n", context->hfsr);
    kprintf ("dfsr  = %p\n", context->dfsr);
    kprintf ("mmfar = %p\n", context->mmfar);
    kprintf ("bfar  = %p\n", context->bfar);
#endif

    // TODO: hook, signal

    BUG_ON ((context->exclr & 0xf) == 1);   // exception happen in handler mode

    BUG_ON (in_critical ());                // exception happen in critical

    BUG_ON (task_suspend (current) != 0);   // kernel task fail

#ifdef CONFIG_AARCH_M_MAINLINE

    /* prevent mem_try in hardfault cause lockup in back_trace */

    scb->ccr |= CCR_BFHFNMIGN;

    exc_trace (context);

    scb->ccr -= CCR_BFHFNMIGN;
#else
    exc_trace (context);
#endif

    kprintf ("task \"%s\" deleted!\n\n", current->name);

    job = &current->job;

    job->job = __exc_task_delete;

    do_deferred (job);
    }

