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

#include <stddef.h>

#include <config.h>
#include <compiler.h>
#include <hal_int.h>
#include <errno.h>

/* globals */

unsigned int int_cnt = 0;

__section__ (stack) __unused__ char irq_stack [CONFIG_IRQ_STACK_SIZE];

/* statics */

static struct
    {
    hal_int_handler_t handler;
    uintptr_t         arg;
    } hal_int_vector [CONFIG_NR_IRQS];

static const hal_int_methods_t * hal_int_methods = NULL;

/**
 * hal_int_dispatch - run interrupt routine for an irq
 * @irq: the irq number
 *
 * return: NA
 */

void hal_int_dispatch (unsigned int irq)
    {
    if (unlikely (irq >= CONFIG_NR_IRQS))
        {
        return;
        }

    if (unlikely (hal_int_vector [irq].handler == NULL))
        {
        return;
        }

    hal_int_vector [irq].handler (hal_int_vector [irq].arg);
    }

/**
 * hal_int_connect - connect a routine to a hardware interrupt
 * @irq:     the irq number to attach to
 * @handler: irq handler to be installed
 * @arg:     argument for the handler
 *
 * return: 0 on success, negtive value on error
 */

int hal_int_connect (unsigned int irq, hal_int_handler_t handler, uintptr_t arg)
    {
    if (unlikely (irq >= CONFIG_NR_IRQS))
        {
        errno_set (ERRNO_HAL_INTC_ILLEGAL_IRQN);
        return -1;
        }

    if (unlikely (hal_int_vector [irq].handler != NULL))
        {
        errno_set (ERRNO_HAL_INTC_ILLEGAL_OPERATION);
        return -1;
        }

    hal_int_vector [irq].arg     = arg;
    hal_int_vector [irq].handler = handler;

    return 0;
    }

/**
 * hal_int_disconnect - disconnect a routine to a hardware interrupt
 * @irq: the irq number to be attached
 *
 * return: 0 on success, negtive value on error
 */

int hal_int_disconnect (unsigned int irq)
    {
    if (unlikely (irq >= CONFIG_NR_IRQS))
        {
        errno_set (ERRNO_HAL_INTC_ILLEGAL_IRQN);
        return -1;
        }

    hal_int_vector [irq].handler = NULL;

    return 0;
    }

/**
 * hal_int_setprio - set the priority of a specific irq
 * @irq:  the irq number
 * @prio: the priority of the irq
 *
 * return: 0 on success, negtive value on error
 */

int hal_int_setprio (unsigned int irq, unsigned int prio)
    {
    if (unlikely (hal_int_methods == NULL || hal_int_methods->setprio == NULL))
        {
        errno_set (ERRNO_HAL_INTC_ILLEGAL_OPERATION);
        return -1;
        }

    return hal_int_methods->setprio (irq, prio);
    }

/**
 * hal_int_enable - enable a specific irq
 * @irq:  the irq number to be enabled
 *
 * return: 0 on success, negtive value on error
 */

int hal_int_enable (unsigned int irq)
    {
    if (unlikely (hal_int_methods == NULL || hal_int_methods->enable == NULL))
        {
        errno_set (ERRNO_HAL_INTC_ILLEGAL_OPERATION);
        return -1;
        }

    // TODO: devfs register

    return hal_int_methods->enable (irq);
    }

/**
 * hal_int_disable - disable a specific irq
 * @irq: the irq number to be disabled
 *
 * return: 0 on success, negtive value on error
 */

int hal_int_disable (unsigned int irq)
    {
    if (unlikely (hal_int_methods == NULL || hal_int_methods->disable == NULL))
        {
        errno_set (ERRNO_HAL_INTC_ILLEGAL_OPERATION);
        return -1;
        }

    return hal_int_methods->disable (irq);
    }

/**
 * hal_int_handler - interrupt handler
 * @irq: the irq number to be disabled
 *
 * return: 0 on success, negtive value on error
 */

void hal_int_handler (void)
    {
    if (unlikely (hal_int_methods == NULL || hal_int_methods->handler == NULL))
        {
        return;
        }

    hal_int_methods->handler ();
    }

/**
 * hal_int_register - register an interrupt controler
 * @methods:   the interrupt controler methods
 *
 * return: 0 on success, negtive value on error
 */

int hal_int_register (const hal_int_methods_t * methods)
    {
    if (unlikely (methods == NULL || methods->enable == NULL))
        {
        errno_set (ERRNO_HAL_INTC_ILLEGAL_OPERATION);
        return -1;
        }

    hal_int_methods = methods;

    // TODO: devfs register

    return 0;
    }

