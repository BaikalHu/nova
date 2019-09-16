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

#include <memtry.h>
#include <errno.h>

#ifdef CONFIG_SYSCALL
#include <syscall.h>
#endif

#include <arch/interface.h>

/* externs */

/**
 * mem_try - try to copy from <src> to <dst> 1 (1 << <order>) bytes unit
 * @dst:   the address read or write
 * @src:   the value to write, or where the read data return
 * @order: try to read or write (1 << order) bytes
 *
 * return: 0 on success, negtive value on error
 */

int mem_try (void * dst, void * src, int order)
    {
    if (unlikely (order > MEMTRY_MAX_ORDER))
        {
        errno_set (ERRNO_MEMTRY_ILLEGAL_ORDER);
        return -1;
        }

    errno_set (ERRNO_MEMTRY_NO_ACCESS);

    if (mem_try_arch (dst, src, order) != 0)
        {
        return -1;
        }

    errno_set (0);

    return 0;
    }

#ifdef CONFIG_SYSCALL
static const uintptr_t syscall_entries_memtry [] =
    {
    (uintptr_t) mem_try,
    };

const struct syscall_table syscall_table_memtry =
    {
    ARRAY_SIZE (syscall_entries_memtry),
    syscall_entries_memtry
    };
#endif

