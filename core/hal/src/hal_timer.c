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

#include <string.h>

#include <common.h>
#include <errno.h>
#include <hal_timer.h>

/* locals */

static dlist_t hal_timers = DLIST_INIT (hal_timers);

/**
 * hal_timer_enable - enable a timer
 * @timer: the timer to be enabled
 * @mode:  the timer mode
 * @count: the timer max rollover count
 *
 * return: 0 on success, negtive value on error
 */

int hal_timer_enable (hal_timer_t * timer, uint8_t mode, unsigned long count)
    {
    if (unlikely (timer == NULL))
        {
        errno_set (ERRNO_HAL_TIMER_ILLEGAL_ID);
        return -1;
        }

    if (unlikely (count > timer->max_count))
        {
        errno_set (ERRNO_HAL_TIMER_ILLEGAL_RANGE);
        return -1;
        }

    timer->mode = mode;

    return timer->methods->enable (timer, count);
    }

/**
 * hal_timer_disable - disable a timer
 * @timer:     the timer to be disabled
 *
 * return: 0 on success, negtive value on error
 */

int hal_timer_disable (hal_timer_t * timer)
    {
    if (unlikely (timer == NULL))
        {
        errno_set (ERRNO_HAL_TIMER_ILLEGAL_ID);
        return -1;
        }

    if (unlikely (timer->methods->disable == NULL))
        {
        errno_set (ERRNO_HAL_TIMER_ILLEGAL_OPERATION);
        return -1;
        }

    return timer->methods->disable (timer);
    }

/**
 * hal_timer_connect - connect a callback routine to a timer
 * @timer: the timer to be connected
 * @pfn:   the callback routine
 * @arg:   the argument to the callback routine
 *
 * return: 0 on success, negtive value on error
 */

int hal_timer_connect (hal_timer_t * timer, void (* pfn) (uintptr_t),
                       uintptr_t arg)
    {
    if (unlikely (timer == NULL))
        {
        errno_set (ERRNO_HAL_TIMER_ILLEGAL_ID);
        return -1;
        }

    timer->handler = pfn;
    timer->arg     = arg;

    return 0;
    }

/**
 * hal_timer_counter - get the current counter of a timer
 * @timer: the timer to read
 *
 * return: the counter for now (timestamp)
 */

unsigned long hal_timer_counter (hal_timer_t * timer)
    {
    if (unlikely (timer == NULL))
        {
        errno_set (ERRNO_HAL_TIMER_ILLEGAL_ID);
        return 0;
        }

    return timer->methods->counter (timer);
    }

/**
 * hal_timer_counter - get the period (max counter) of a timer
 * @timer: the timer to read
 *
 * return: the max counter of the given timer
 */

unsigned long hal_timer_period (hal_timer_t * timer)
    {
    if (unlikely (timer == NULL))
        {
        errno_set (ERRNO_HAL_TIMER_ILLEGAL_ID);
        return 0;
        }

    return timer->methods->period (timer);
    }

/**
 * hal_timer_postpone - postpone next timer interrupt
 * @timer: the timer
 * @count: the count
 *
 * return: 0 on success, negtive value on error
 */

int hal_timer_postpone (hal_timer_t * timer, unsigned long count)
    {
    if (unlikely (timer == NULL))
        {
        errno_set (ERRNO_HAL_TIMER_ILLEGAL_ID);
        return -1;
        }

    if (unlikely (timer->methods->postpone == NULL))
        {
        errno_set (ERRNO_HAL_TIMER_ILLEGAL_OPERATION);
        return -1;
        }

    timer->methods->postpone (timer, count);

    return 0;
    }

/**
 * hal_timer_prepone - prepone next timer interrupt
 * @timer: the timer
 * @count: the count
 *
 * return: 0 on success, negtive value on error
 */

int hal_timer_prepone (hal_timer_t * timer, unsigned long count)
    {
    if (unlikely (timer == NULL))
        {
        errno_set (ERRNO_HAL_TIMER_ILLEGAL_ID);
        return -1;
        }

    if (unlikely (timer->methods->prepone == NULL))
        {
        errno_set (ERRNO_HAL_TIMER_ILLEGAL_OPERATION);
        return -1;
        }

    timer->methods->prepone (timer, count);

    return 0;
    }

/**
 * hal_timer_register - register a timer to the hal
 * @timer: the timer to register
 *
 * return: 0 on success, negtive value on error
 */

int hal_timer_register (hal_timer_t * timer)
    {
    if (unlikely (timer == NULL))
        {
        errno_set (ERRNO_HAL_TIMER_ILLEGAL_ID);
        return -1;
        }

    if (unlikely (timer->methods == NULL          ||
                  timer->methods->enable  == NULL ||
                  timer->methods->counter == NULL ||
                  timer->methods->period  == NULL))
        {
        errno_set (ERRNO_HAL_TIMER_ILLEGAL_CONFIG);
        return -1;
        }

    dlist_add_tail (&hal_timers, &timer->node);

    return 0;
    }

/**
 * hal_timer_get - get (allocate) a timer by name
 * @name: the timer name
 *
 * return: 0 on success, negtive value on error
 */

hal_timer_t * hal_timer_get (const char * name)
    {
    dlist_t     * itr;

    dlist_foreach (itr, &hal_timers)
        {
        hal_timer_t * timer = container_of (itr, hal_timer_t, node);

        if (strncmp (name, timer->name, HAL_TIMER_MAX_NAME_LEN))
            {
            continue;
            }

        if (timer->busy)
            {
            continue;
            }

        timer->busy = true;

        return timer;
        }

    errno_set (ERRNO_HAL_TIMER_NOT_ENOUGH_TIMER);

    return NULL;
    }

/**
 * hal_timer_feat_get - get the timer features
 * @timer:     the timer handler
 * @max_count: return value of the max counter
 * @freq:      return value of the freq
 *
 * return: 0 on success, negtive value on error
 */

int hal_timer_feat_get (hal_timer_t * timer, uint32_t * max_count,
                        uint32_t * freq)
    {
    if (unlikely (timer == NULL))
        {
        errno_set (ERRNO_HAL_TIMER_ILLEGAL_ID);
        return -1;
        }

    if (max_count != NULL)
        {
        *max_count = timer->max_count;
        }

    if (freq != NULL)
        {
        *freq = timer->freq;
        }

    return 0;
    }

