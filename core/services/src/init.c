/*
 * Copyright (c) [2019] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * LiteOS NOVA is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan PSL v1.
 * You may obtain a copy of Mulan PSL v1 at:
 *
 *      http://license.coscl.org.cn/MulanPSL
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v1 for more details.
 */

#include <config.h>

#include <task.h>
#include <init.h>
#include <irq.h>
#include <errno.h>
#include <kprintf.h>
#include <warn.h>

#include <arch/interface.h>

/* externs */

extern int main (void);

extern char __section_start__       (init_cpu)  [];
extern char __section_end__         (init_user) [];

/**
 * kernel_init - kernel initialization routine
 *
 * return : 0 on success, negtive value on error
 */

__noreturn void kernel_init (void)
    {
    init_pfn * init;

    for (init  = (init_pfn *) __section_start__ (init_cpu);
         init != (init_pfn *) __section_end__   (init_user);
         init++)
        {
        if ((*init) () != 0)
            {
            WARN ("Kernel initialization fail, last routine is %p, errno = %p\n",
                  *init, errno);
            }
        }

    task_sched_start ();

    /* should never be here */

    while (1)
        {
        int_lock ();
        }
    }

/**
 * user_init - user entry (main) task initialization routine
 *
 * return: 0 on success, negtive value on error
 */

static int user_init (void)
    {
    task_id tid;

    tid = task_spawn ("main", CONFIG_MAIN_TASK_PRIO, CONFIG_MAIN_TASK_OPTIONS,
                      CONFIG_MAIN_TASK_STACK_SIZE, (int (*) (uintptr_t)) main, 0);

    WARN_ON (tid == NULL, return -1, "Fail to create user task <main>!");

    return 0;
    }

MODULE_INIT (user, user_init);
