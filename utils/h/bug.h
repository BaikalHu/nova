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

#ifndef __BUG_H__
#define __BUG_H__

#include <kconfig.h>

#include <irq.h>
#include <kprintf.h>

#include <arch/trace.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef CONFIG_DEBUG

#define BUG(str)                                                                \
do                                                                              \
    {                                                                           \
                                                                                \
    /* disable interrupt to force kprintf use poll mode */                      \
                                                                                \
    (void) int_lock ();                                                         \
                                                                                \
    kprintf ("\nBUG @ (file : %s, line : %d)\n", __FILE__, __LINE__);           \
                                                                                \
    __bug (str);                                                                \
    } while (0)

#define BUG_ON(cond)                                                            \
do                                                                              \
    {                                                                           \
    if (unlikely (cond))                                                        \
        {                                                                       \
        BUG (__CVTSTR_RAW (cond));                                              \
        }                                                                       \
    } while (0)

#define WARN(str)                                                               \
do                                                                              \
    {                                                                           \
    kprintf ("\nWARN @ (file : %s, line : %d)\n", __FILE__, __LINE__);          \
    kprintf ("\"%s\"\n", str);                                                  \
    call_trace ();                                                              \
    } while (0)

#define WARN_ON(cond)                                                           \
do                                                                              \
    {                                                                           \
    if (unlikely (cond))                                                        \
        {                                                                       \
        WARN (__CVTSTR_RAW (cond));                                             \
        }                                                                       \
    } while (0)

#else

#define BUG(str)                                                                \
do                                                                              \
    {                                                                           \
    __bug (str);                                                                \
    } while (0)

#define BUG_ON(cond)                                                            \
do                                                                              \
    {                                                                           \
    if (unlikely (cond))                                                        \
        {                                                                       \
        BUG (__CVTSTR_RAW (cond));                                              \
        }                                                                       \
    } while (0)

#define WARN(str)                                                               \
do                                                                              \
    {                                                                           \
    __asm__ __volatile__ ("" : : : "memory");                                   \
    } while (0)

#define WARN_ON(cond)                                                           \
do                                                                              \
    {                                                                           \
    if (unlikely (cond))                                                        \
        {                                                                       \
        __asm__ __volatile__ ("" : : : "memory");                               \
        }                                                                       \
    } while (0)
#endif

extern void __bug (const char *);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __BUG_H__ */

