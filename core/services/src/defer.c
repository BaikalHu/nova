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
#include <event.h>
#include <sync.h>
#include <init.h>
#include <bug.h>

#ifdef CONFIG_PROFILE
#include <profile.h>
#endif

static dlist_t          __deferred_jobs = DLIST_INIT (__deferred_jobs);
static event_t          __deferred_event;

static uint64_t         __defer_stack [CONFIG_DEFERRED_STACK_SIZE / sizeof (uint64_t)];

/* globals */

task_t                  defer [1];

struct deferred_isr_q   deferred_isr_q = {0};

/* typedefs */

static __always_inline__ bool __deferred_isr_q_full (void)
    {
    return (deferred_isr_q.tail_idx - deferred_isr_q.head_idx) == DEFERRED_IRQ_JOB_SLOTS;
    }

/*
 * deferred_isr_q_idx - get deferred isr queue insert index
 *
 * return: the index or -1 on error
 */

__weak__ unsigned int deferred_isr_q_idx (void)
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

    if (unlikely (idx == (unsigned int) -1))
        {
        errno_set (ERRNO_DEFERRED_QUEUE_FULL);
        return -1;
        }

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
    if (unlikely (job == NULL))
        {
        return -1;
        }

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

    return event_send (&__deferred_event, 1);
    }

static __always_inline__ void __do_deferred (deferred_job_t * job)
    {
    void (* rtn) (struct deferred_job *);

    rtn = job->job;

    atomic_uint_set (&job->busy, 0);

    rtn (job);
    }

static __noreturn__ int deferred_task (uintptr_t dummy)
    {
    dlist_t        * dlist;
    deferred_job_t * job;

    (void) dummy;

    while (1)
        {
        while (event_recv (&__deferred_event, 0xffffffff,
                           EVENT_WAIT_ANY | EVENT_WAIT_CLR, NULL) != 0);

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
    int ret;

    BUG_ON (event_init (&__deferred_event) != 0);

    ret = task_init  (defer, (char *) __defer_stack, "defer", 0,
                      TASK_OPTION_SYSTEM, CONFIG_DEFERRED_STACK_SIZE,
                      deferred_task, 0);

    BUG_ON (ret != 0);

    BUG_ON (task_resume (defer) != 0);

    return 0;
    }

MODULE_INIT (user, defer_lib_init);

