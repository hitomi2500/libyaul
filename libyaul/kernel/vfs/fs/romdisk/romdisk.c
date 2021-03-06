/*
 * Copyright (c) 2001-2003, 2012-2019
 * See LICENSE for details.
 *
 * Dan Potter
 * Lawrence Sebald
 */

#include <sys/types.h>

#include <stdlib.h>
#include <errno.h>

#include "romdisk.h"

#include <internal.h>

#define ROMFS_MAGIC             "-rom1fs-"
#define MAX_RD_FILES            16

#define TYPE_HARD_LINK          0
#define TYPE_DIRECTORY          1
#define TYPE_REGULAR_FILE       2
#define TYPE_SYMBOLIC_LINK      3
#define TYPE_BLOCK_DEVICE       4
#define TYPE_CHAR_DEVICE        5

#define IS_TYPE(x)              ((x) & 0x03)
#define TYPE_GET(x)             ((x) & 0x0F)

#define HEADER

struct rd_file_handle;

TAILQ_HEAD(rd_file, rd_file_handle);

struct rd_file_handle {
        struct rd_file *rdh;
        uint32_t index;         /* ROMFS image index */
        bool dir;               /* If a directory */
        int32_t ptr;            /* Current read position in bytes */
        size_t len;             /* Length of file in bytes */
        void *mnt;              /* Which mount instance are we using? */

        TAILQ_ENTRY(rd_file_handle) handles;
};

struct romdisk_hdr {
        char magic[8];          /* Should be "-rom1fs-" */
        uint32_t full_size;     /* Full size of the file system */
        uint32_t checksum;      /* Checksum */
        char volume_name[16];   /* Volume name (zero-terminated) */
};

/* File header info; note that this header plus filename must be a
 * multiple of 16 bytes, and the following file data must also be a
 * multiple of 16 bytes */
struct romdisk_file {
        uint32_t next_header;   /* Offset of next header */
        uint32_t spec_info;     /* Spec info */
        uint32_t size;          /* Data size */
        uint32_t checksum;      /* File checksum */
        char filename[16];      /* File name (zero-terminated) */
};

struct rd_image {
        const uint8_t *image;   /* The actual image */
        const struct romdisk_hdr *hdr; /* Pointer to the header */
        uint32_t files;         /* Offset in the image to the files area */
};

static uint32_t romdisk_find(struct rd_image *, const char *, bool);
static uint32_t romdisk_find_object(struct rd_image *, const char *, size_t, bool,
    uint32_t);

/* XXX Provisional */
static struct rd_file_handle *romdisk_fd_alloc(void);
static void romdisk_fd_free(struct rd_file_handle *);
/* XXX Provisional */
static struct rd_file fhs;

void
romdisk_init(void)
{
        TAILQ_INIT(&fhs);
}

void *
romdisk_mount(const char *mnt_point __unused,
    const uint8_t *image)
{
        struct romdisk_hdr *hdr;
        struct rd_image *mnt;

        if (strncmp((char *)image, ROMFS_MAGIC, 8)) {
                return NULL;
        }

        hdr = (struct romdisk_hdr *)image;
        if ((mnt = (struct rd_image *)_internal_malloc(sizeof(struct rd_image))) == NULL) {
                return NULL;
        }

        mnt->image = image;
        mnt->hdr = hdr;
        mnt->files = sizeof(struct romdisk_hdr) +
            ((strlen(hdr->volume_name) >> 4) << 4);

        return mnt;
}

void *
romdisk_open(void *p, const char *fn)
{
        struct rd_image *mnt;
        struct rd_file_handle *fh;

        const struct romdisk_file *f_hdr;
        uint32_t f_idx;
        bool directory;

        mnt = (struct rd_image *)p;
        directory = false;
        if ((f_idx = romdisk_find(mnt, fn, directory)) == 0) {
                /* errno = ENOENT; */
                return NULL;
        }

        f_hdr = (const struct romdisk_file *)(mnt->image + f_idx);

        if ((fh = romdisk_fd_alloc()) == NULL) {
                return NULL;
        }

        fh->rdh = &fhs;
        fh->index = f_idx + mnt->files;
        fh->dir = directory;
        fh->ptr = 0;
        fh->len = f_hdr->size;
        fh->mnt = mnt;

        return fh;
}

void
romdisk_close(void *fh)
{
        romdisk_fd_free(fh);
}

ssize_t
romdisk_read(void *p, void *buf, size_t bytes)
{
        struct rd_file_handle *fh;
        struct rd_image *mnt;
        uint8_t *ofs;

        fh = (struct rd_file_handle *)p;
        mnt = (struct rd_image *)fh->mnt;

        /* Sanity checks */
        if ((fh == NULL) || (fh->index == 0)) {
                /* Not a valid file descriptor or is not open for
                 * reading */
                /* errno = EBADF; */
                return -1;
        }

        if (fh->dir) {
                /* errno = EISDIR; */
                return -1;
        }

        /* Is there enough left? */
        if ((fh->ptr + bytes) > fh->len) {
                bytes = fh->len - fh->ptr;
        }

        ofs = (uint8_t *)(mnt->image + fh->index + fh->ptr);
        memcpy(buf, ofs, bytes);
        fh->ptr += bytes;

        return bytes;
}

void *
romdisk_direct(void *p)
{
        struct rd_file_handle *fh;
        struct rd_image *mnt;

        fh = (struct rd_file_handle *)p;
        mnt = (struct rd_image *)fh->mnt;

        /* Sanity checks */
        if ((fh == NULL) || (fh->index == 0)) {
                /* Not a valid file descriptor or is not open for
                 * reading */
                /* errno = EBADF; */
                return NULL;
        }

        if (fh->dir) {
                /* errno = EISDIR; */
                return NULL;
        }

        return (void *)(mnt->image + fh->index);
}

off_t
romdisk_seek(void *p, off_t offset, int whence)
{
        struct rd_file_handle *fh;

        fh = (struct rd_file_handle *)p;

        /* Sanity checks */
        if ((fh == NULL) || (fh->index == 0)) {
                /* Not a valid file descriptor or is not open for
                 * reading */
                /* errno = EBADF; */
                return -1;
        }

        if (fh->dir) {
                /* errno = EISDIR; */
                return -1;
        }

        /* Update current position according to arguments */
        switch(whence) {
        case SEEK_SET:
                fh->ptr = offset;
                break;
        case SEEK_CUR:
                fh->ptr += offset;
                break;
        case SEEK_END:
                fh->ptr = fh->len + offset;
                break;
        default:
                /* The whence argument to fseek() was not 'SEEK_SET',
                 * 'SEEK_END', or 'SEEK_CUR' */
                /* errno = EINVAL; */
                return -1;
        }

        /* Check bounds */
        if(fh->ptr < 0)
                fh->ptr = 0;

        if(fh->ptr > (int32_t)fh->len)
                fh->ptr = fh->len;

        return fh->ptr;
}

off_t
romdisk_tell(void *p)
{
        struct rd_file_handle *fh;

        fh = (struct rd_file_handle *)p;
        /* Sanity checks */
        if ((fh == NULL) || (fh->index == 0)) {
                /* Not a valid file descriptor or is not open for
                 * reading */
                /* errno = EBADF; */
                return -1;
        }

        if (fh->dir) {
                /* errno = EISDIR; */
                return -1;
        }

        return fh->ptr;
}

size_t
romdisk_total(void *p)
{
        struct rd_file_handle *fh;

        fh = (struct rd_file_handle *)p;

        /* Sanity checks */
        if ((fh == NULL) || (fh->index == 0)) {
                /* Not a valid file descriptor or is not open for
                 * reading */
                /* errno = EBADF; */
                return -1;
        }

        if (fh->dir) {
                /* errno = EISDIR; */
                return -1;
        }

        return fh->len;
}

/* Given a file name and a starting ROMDISK directory listing (byte
 * offset), search for the entry in the directory and return the byte
 * offset to its entry */
static uint32_t
romdisk_find_object(struct rd_image *mnt, const char *fn, size_t fn_len, bool directory,
    uint32_t offset)
{
        uint32_t next_ofs;
        uint32_t type;

        const struct romdisk_file *f_hdr;

        do {
                f_hdr = (const struct romdisk_file *)(mnt->image + offset);
                next_ofs = f_hdr->next_header;

                /* Since the file headers begin always at a 16 byte
                 * boundary, the lowest 4 bits would be always zero in
                 * the next filehdr pointer
                 *
                 * These four bits are used for the mode information */

                /* Bits 0..2 specify the type of the file; while bit 4
                 * shows if the file is executable or not */
                type = TYPE_GET(next_ofs);
                next_ofs &= 0xFFFFFFF0;

                /* Check filename */
                if ((strlen(f_hdr->filename) == fn_len) &&
                    ((strncmp(f_hdr->filename, fn, fn_len)) == 0)) {
                        if (directory && (IS_TYPE(type) == TYPE_DIRECTORY))
                                return offset;

                        if (!directory && (IS_TYPE(type) == TYPE_REGULAR_FILE))
                                return offset;

                        return 0;
                }

                /* Not found */
                offset = next_ofs;
        } while (offset);

        return 0;
}

static uint32_t
romdisk_find(struct rd_image *mnt, const char *fn, bool directory)
{
        const char *fn_cur;
        uint32_t ofs;
        int fn_len;
        const struct romdisk_file *f_hdr;

        fn_cur = NULL;
        ofs = mnt->files;

        /* Traverse directories */
        while ((fn_cur = strchr(fn, '/'))) {
                if (fn_cur != fn) {
                        fn_len = fn_cur - fn;
                        if ((ofs = romdisk_find_object(mnt,
                                    fn, fn_len, /* directory = */ true, ofs)) == 0)
                                return 0;

                        f_hdr = (const struct romdisk_file *)(mnt->image + ofs);
                        ofs = f_hdr->spec_info;
                }

                fn = fn_cur + 1;
        }

        /* Locate the file under the resultant directory */
        if (*fn != '\0') {
                fn_len = strlen(fn);
                ofs = romdisk_find_object(mnt, fn, fn_len, directory, ofs);
                return ofs;
        }

        return 0;
}

static struct rd_file_handle *
romdisk_fd_alloc(void)
{
        struct rd_file_handle *fh;
        fh = (struct rd_file_handle *)_internal_malloc(sizeof(struct rd_file_handle));

        if (fh == NULL) {
                return NULL;
        }

        TAILQ_INSERT_TAIL(&fhs, fh, handles);

        return fh;
}

static void
romdisk_fd_free(struct rd_file_handle *fd)
{
        if (fd == NULL) {
                return;
        }

        struct rd_file_handle *fh;

        bool fd_match;
        fd_match = false;

        TAILQ_FOREACH (fh, &fhs, handles) {
                if (fh == fd) {
                        fd_match = true;
                        break;
                }
        }

        if (!fd_match) {
                return;
        }

        TAILQ_REMOVE(&fhs, fh, handles);
        _internal_free(fh);
}
