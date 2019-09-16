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

#ifndef __ERRNO_H__
#define __ERRNO_H__

#include <stdint.h>

#include <sys/errno.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* macros */

#define ERRNO_UNKOWN            ((uint32_t) 0xfffffffful)

/*
 * the errno format:
 *
 * +-----+-----+
 * | mid | err |
 * +-----+-----+
 *    |     |
 *    |     \
 *    \      `-- 8-bit error number
 *     `-------- 8-bit module id
 */

#define ERRNO_JOIN(mid, err)    ((((uint32_t) mid & 0xff) << 8) | \
                                 (((uint32_t) err & 0xff) << 0))

/* externs */

extern void     errno_set       (uint32_t);
extern uint32_t errno_get       (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __ERRNO_H__ */

