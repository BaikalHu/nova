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
#include <string.h>
#include <stdio.h>

#include <config.h>
#include <common.h>
#include <devfs.h>
#include <init.h>
#include <bug.h>

struct devfs_file
    {
    dlist_t                  node;
    const struct devfs_ops * ops;
    uintptr_t                data;
    const char             * name;
    };

static dlist_t devfs_files = DLIST_INIT (devfs_files);

static struct devfs_file * __get_devfile (const char * name)
    {
    dlist_t           * itr;
    struct devfs_file * file;

    if (strchr (name, '/') != NULL)
        {
        return NULL;
        }

    dlist_foreach (itr, &devfs_files)
        {
        file = container_of (itr, struct devfs_file, node);

        if (strcmp (name, file->name) == 0)
            {
            return file;
            }
        }

    return NULL;
    }

static int devfs_open (uintptr_t mp_data, uintptr_t * fl_data_ptr,
                       const char * name, int flags)
    {
    struct devfs_file * file = __get_devfile (name);

    if (file == NULL)
        {
        errno_set (ENOENT);
        return -1;
        }

    *fl_data_ptr = (uintptr_t) file;

    return file->ops->open  == NULL ? 0 : file->ops->open (file->data);
    }

static int devfs_close (uintptr_t fl_data)
    {
    struct devfs_file * file = (struct devfs_file *) fl_data;

    return file->ops->close == NULL ? 0 : file->ops->close (file->data);
    }

static int devfs_read (uintptr_t fl_data, char * buff, size_t nbyte)
    {
    struct devfs_file * file = (struct devfs_file *) fl_data;

    if (file->ops->read == NULL)
        {
        errno_set (ENOTSUP);
        return -1;
        }

    return file->ops->read (file->data, buff, nbyte);
    }

static int devfs_write (uintptr_t fl_data, const char * buff, size_t nbyte)
    {
    struct devfs_file * file = (struct devfs_file *) fl_data;

    if (file->ops->write == NULL)
        {
        errno_set (ENOTSUP);
        return -1;
        }

    return file->ops->write (file->data, buff, nbyte);
    }

static int devfs_lseek (uintptr_t fl_data, int off, int whence)
    {
    struct devfs_file * file = (struct devfs_file *) fl_data;

    if (file->ops->lseek == NULL)
        {
        errno_set (ENOTSUP);
        return -1;
        }

    return file->ops->lseek (file->data, off, whence);
    }

static int devfs_ioctl (uintptr_t fl_data, int request, va_list valist)
    {
    struct devfs_file * file = (struct devfs_file *) fl_data;

    if (file->ops->ioctl == NULL)
        {
        errno_set (ENOTSUP);
        return -1;
        }

    return file->ops->ioctl (file->data, request, valist);
    }

static int devfs_sync (uintptr_t fl_data)
    {
    struct devfs_file * file = (struct devfs_file *) fl_data;

    if (file->ops->sync == NULL)
        {
        errno_set (ENOTSUP);
        return -1;
        }

    return file->ops->sync (file->data);
    }

static int devfs_stat (uintptr_t mp_data, const char * name, struct stat * stat)
    {
    struct devfs_file * file = __get_devfile (name);

    if (file == NULL)
        {
        errno_set (EBADF);
        return -1;
        }

    memset (stat, 0, sizeof (struct stat));

    if (file->ops->read != NULL)
        {
        stat->st_mode |= 0444;
        }

    if (file->ops->write != NULL)
        {
        stat->st_mode |= 0222;
        }

    if (file->ops->lseek != NULL)
        {
        stat->st_mode |= S_IFBLK;
        }
    else
        {
        stat->st_mode |= S_IFCHR;
        }

    if (file->ops->stat != NULL)
        {
        return file->ops->stat (file->data, stat);
        }

    return 0;
    }

static int devfs_opendir (uintptr_t mp_data, uintptr_t * dr_data_ptr, const char * path)
    {
    uint32_t * offset;

    if (unlikely (*path != '\0'))
        {
        errno_set (ENOENT);
        return -1;
        }

    offset = (uint32_t *) malloc (sizeof (uint32_t));

    if (offset == NULL)
        {
        return -1;
        }

    *offset      = 0;
    *dr_data_ptr = (uintptr_t) offset;

    return 0;
    }

static int devfs_readdir (uintptr_t mp_data, uintptr_t dr_data, struct dirent * de)
    {
    dlist_t           * itr;
    uint32_t            i = 0;
    struct devfs_file * file;
    uint32_t          * offset = (uint32_t *) dr_data;

    (void) mp_data;

    dlist_foreach (itr, &devfs_files)
        {
        if (i++ == *offset)
            {
            file = container_of (itr, struct devfs_file, node);

            strncpy (de->de_name, file->name, CONFIG_MAX_FILE_NAME_LEN);

            de->de_type = "file";
            de->de_size = file->ops->size == NULL ? 0 : file->ops->size (file->data);

            *offset = *offset + 1;

            return 0;
            }
        }

    return -1;
    }

static int devfs_closedir (uintptr_t mp_data, uintptr_t dr_data)
    {
    uint32_t * offset = (uint32_t *) dr_data;

    free (offset);

    return 0;
    }

static int devfs_mount (uintptr_t * mp_data_ptr, va_list valist)
    {
    static bool devfs_mounted = false;

    if (devfs_mounted)
        {
        return 0;
        }

    dlist_init (&devfs_files);

    return 0;
    }

static const struct f_ops devfs_fops =
    {
    devfs_open,
    devfs_close,
    devfs_read,
    devfs_write,
    devfs_lseek,
    devfs_ioctl,
    devfs_sync,
    };

static const struct m_ops devfs_mops =
    {
    devfs_stat,
    NULL,
    NULL,
    devfs_opendir,
    devfs_readdir,
    devfs_closedir,
    NULL,
    devfs_mount,
    NULL
    };

static struct file_system devfs =
    {
    "devfs",
    &devfs_fops,
    &devfs_mops,
    };

/**
 * devfs_add_file - device filesystem add file
 * @name: the file name in the device filesystem mount point
 * @ops:  the device file operation methods
 * @data: the private data of the device file (arguments to operation routines)
 *
 * return : 0 on success, negtive value on error
 */

int devfs_add_file (const char * name, const struct devfs_ops * ops, uintptr_t data)
    {
    struct devfs_file * file;

    if (name == NULL || ops == NULL || strchr (name, '/') != NULL)
        {
        return -1;
        }

    /* CONFIG_MAX_FILE_NAME_LEN - 1 for the id and '\0' */

    if (strlen (name) >= CONFIG_MAX_FILE_NAME_LEN - 1)
        {
        return -1;
        }

    if ((file = (struct devfs_file *) malloc (sizeof (struct devfs_file))) == NULL)
        {
        return -1;
        }

    file->ops  = ops;
    file->data = data;
    file->name = name;

    dlist_add_tail (&devfs_files, &file->node);

    return 0;
    }

/**
 * devfs_init - device filesystem initialization routine
 *
 * return : 0 on success, negtive value on error
 */

static int devfs_init (void)
    {
    if (vfs_fs_register (&devfs) != 0)
        {
        WARN ("fail to register devfs!");
        return -1;
        }

    if (vfs_mount ("devfs", "/dev/") != 0)
        {
        WARN ("fail to mount devfs!");
        return -1;
        }

    return 0;
    }

MODULE_INIT (bus, devfs_init);

