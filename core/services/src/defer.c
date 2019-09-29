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

#include <stdlib.h>
#include <limits.h>

#include <common.h>
#include <list.h>
#include <defer.h>
#include <irq.h>
#include <task.h>
#include <sem.h>
#include <sync.h>
#include <init.h>
#include <warn.h>
#include <bug.h>

#ifdef CONFIG_PROFILE
#include <profile.h>
#endif

/* locals */

static dlist_t          __deferred_jobs = DLIST_INIT (__deferred_jobs);
static sem_t            __deferred_sem;

static uint64_t         __defer_stack [CONFIG_DEFERRED_STACK_SIZE / sizeof (uint64_t)];

static task_t           __defer [1];

/* globals */

struct deferred_isr_q   deferred_isr_q = {0};

/* typedefs */

static __always_inline bool __deferred_isr_q_full (void)
    {
    return (deferred_isr_q.tail_idx - deferred_isr_q.head_idx) == DEFERRED_IRQ_JOB_SLOTS;
    }

/*
 * deferred_isr_q_idx - get deferred isr queue insert index
 *
 * return: the index or -1 on error
 */

__weak unsigned int deferred_isr_q_idx (void)
    {
    unsigned int  idx;
    unsigned long key = int_lock ();

    if (unlikely (__deferred_isr_q_full ()))
        {
        idx = (unsigned int) -1;
        }
    else
        {
        idx = deferred_isr_q.tail_idx++;
        }

    int_restore (key);

    return idx;
    }

static int deferred_isr_q_put (deferred_job_t * job)
    {
    unsigned int idx = deferred_isr_q_idx ();

    WARN_ON (idx == (unsigned int) -1,
             errno = ERRNO_DEFERRED_QUEUE_FULL; return -1,
             "deferred_isr_q full!");

    idx &= DEFERRED_IRQ_JOB_MASK;

    deferred_isr_q.jobs [idx]  = job;

    return 0;
    }

static deferred_job_t * deferred_isr_q_get (void)
    {
    deferred_job_t * deferred_isr_job;
    unsigned int     idx = deferred_isr_q.head_idx;

    if (idx == deferred_isr_q.tail_idx)
        {
        return NULL;
        }

    deferred_isr_job = deferred_isr_q.jobs [idx & DEFERRED_IRQ_JOB_MASK];

    mb ();

    deferred_isr_q.head_idx = idx + 1;

    mb ();

    return deferred_isr_job;
    }

/**
 * do_deferred - do deferred job
 * @job:  the event id
 *
 * return: 0 on success, negtive value on error
 */

int do_deferred (deferred_job_t * job)
    {
    WARN_ON (job == NULL, errno = ERRNO_DEFERRED_ILLEGAL_JOB; return -1,
             "Invalid job!");

    /* job already in queue */

    if (unlikely (!atomic_uint_set_eq (&job->busy, 0, 1)))
        {
        return -1;
        }

    if (int_context ())
        {
        if (deferred_isr_q_put (job) != 0)
            {
            return -1;
            }
        }
    else
        {

        /* maybe invoked in critical context, do not use mutex */

        task_lock ();

        dlist_add_tail (&__deferred_jobs, &job->node);

        task_unlock ();
        }

    return sem_post (&__deferred_sem);
    }

static __always_inline void __do_deferred (deferred_job_t * job)
    {
    void (* rtn) (struct deferred_job *);

    rtn = job->job;

    atomic_uint_set (&job->busy, 0);

    rtn (job);
    }

static __noreturn int deferred_task (uintptr_t dummy)
    {
    dlist_t        * dlist;
    deferred_job_t * job;

    (void) dummy;

    while (1)
        {
        while (sem_wait (&__deferred_sem) != 0);

        while (1)
            {
            job = deferred_isr_q_get ();

            if (likely (job == NULL))
                {
                break;
                }

            __do_deferred (job);
            }

        while (!dlist_empty (&__deferred_jobs))
            {
            dlist = __deferred_jobs.next;

            task_lock ();

            dlist_del (dlist);

            task_unlock ();

            job = container_of (dlist, deferred_job_t, node);

            __do_deferred (job);
            }
        }
    }

/**
 * defer_lib_init - defer initialization routine
 *
 * return: 0 on success, negtive value on error
 */

static int defer_lib_init (void)
    {
    BUG_ON (sem_init (&__deferred_sem, 0) != 0,
            "Fail to initialize __deferred_sem!");

    BUG_ON (task_init (__defer, (char *) __defer_stack, CONFIG_DEFERRED_NAME, 0,
                       TASK_OPTION_SYSTEM, CONFIG_DEFERRED_STACK_SIZE,
                       deferred_task, 0) != 0,
            "Fail to initialize deferred task!");

    BUG_ON (task_resume (__defer) != 0,
            "Fail to resume deffered task!");

    return 0;
    }

MODULE_INIT (user, defer_lib_init);

