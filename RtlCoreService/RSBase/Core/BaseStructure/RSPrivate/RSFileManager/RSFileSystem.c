//
//  RSFileSystem.c
//  RSCoreFoundation
//
//  Created by RetVal on 8/12/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include "RSFileSystem.h"
#include <sys/mman.h>   /*mmap*/
#include <sys/fcntl.h>  /*open*/
static BOOL __rs_mmap(int fd, void ** buf, size_t *size)
{
    *buf = mmap(nil, *size, PROT_READ, MAP_PRIVATE, fd, 0);
    return (*buf) ? YES : NO;
}

static size_t __rs_read(const char *path, void **buf, int *fd)
{
    *fd = open(path, O_RDONLY);
    if (*fd > 0)
    {
        size_t size = 0;
        if (__rs_mmap(*fd, buf, &size))
            return size;
        close(*fd);
        *fd = -1;
        return EOF;
    }
    return EOF;
}

static size_t __rs_write(int fd, const void *buf, size_t size)
{
    return write(fd, buf, size);
}