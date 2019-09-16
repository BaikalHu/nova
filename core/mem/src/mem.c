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

#include <config.h>
#include <mem.h>
#include <heap.h>
#include <init.h>

#ifdef CONFIG_CMDER
#include <cmder.h>
#endif

void * malloc (size_t size)
    {
    return heap_alloc (kernel_heap, size);
    }

void * realloc (void * ptr, size_t size)
    {
    return (void *) heap_realloc (kernel_heap, (char *) ptr, size);
    }

void free (void * ptr)
    {
    heap_free (kernel_heap, ptr);
    }

void * memalign (size_t alignment,  size_t size)
    {
    return heap_alloc_align (kernel_heap, alignment, size);
    }

#ifdef CONFIG_CMDER

extern void __cmd_dump_heap (cmder_t * cmder, heap_t * heap, bool show_chunk);

static int __cmd_meminfo (cmder_t * cmder, int argc, char * argv [])
    {
    const struct phys_mem * spm = system_phys_mem;
    bool                    show_chunk = false;
    int                     i;

    for (i = 1; i < argc; i++)
        {
        if (strncmp (argv [i], "--chunk", 7) == 0)
            {
            show_chunk = true;
            }
        }

    cmder_printf (cmder, "all physcial memory blocks:\n\n");

    cmder_printf (cmder, "address    end        length    \n");
    cmder_printf (cmder, "---------- ---------- ----------\n");

    while (spm->end)
        {
        cmder_printf (cmder, "%10p %10p %10p\n", spm->start, spm->end - 1,
                      spm->end - spm->start);
        spm++;
        }

    cmder_printf (cmder, "\n[kernel_heap] statistics\n\n");

    (void) __cmd_dump_heap (cmder, kernel_heap, show_chunk);

    return 0;
    }

CMDER_CMD_DEF ("meminfo", "show memory information, usage: 'meminfo [--chunk]'",
               __cmd_meminfo);
#endif
