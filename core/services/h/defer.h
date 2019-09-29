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

#ifndef __DEFER_H__
#define __DEFER_H__

#include <stdint.h>

#include <list.h>
#include <atomic.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* macros */

#define DEFERRED_IRQ_JOB_SLOTS              (CONFIG_NR_DEFERRED_IRQ_JOB_SLOTS)
#define DEFERRED_IRQ_JOB_MASK               (DEFERRED_IRQ_JOB_SLOTS - 1)

#define ERRNO_DEFERRED_QUEUE_FULL           ERRNO_JOIN (MID_DEFER, 1)
#define ERRNO_DEFERRED_ILLEGAL_JOB          ERRNO_JOIN (MID_DEFER, 2)
#define ERRNO_DEFERRED_ILLEGAL_OPERATION    ERRNO_JOIN (MID_DEFER, 3)

/* typedefs */

typedef struct deferred_job
    {
    dlist_t               node;
    void               (* job) (struct deferred_job *);
    atomic_uint           busy;
    } deferred_job_t;

struct deferred_isr_q
    {
    volatile unsigned int head_idx;
    volatile unsigned int tail_idx;
    deferred_job_t      * jobs [DEFERRED_IRQ_JOB_SLOTS];
    };

/* externs */

extern struct deferred_isr_q  deferred_isr_q;

extern int do_deferred (deferred_job_t *);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DEFER_H__ */

