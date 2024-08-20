#ifndef _MAPPED_FILE_H_
#define _MAPPED_FILE_H_

#include "includes.h"
#include "rbtree.h"


#ifdef _WIN32

#define MF_ACCESS_NORMAL     0
#define MF_ACCESS_RANDOM     0
#define MF_ACCESS_SEQUENTIAL 0

#else /* _WIN32 */

#define MF_ACCESS_NORMAL     MADV_NORMAL
#define MF_ACCESS_RANDOM     MADV_RANDOM
#define MF_ACCESS_SEQUENTIAL MADV_SEQUENTIAL

#endif /* _WIN32 */


/* Macros used by the allocator API. */
#define MASK         (~(sizeof(size_t) - 1))
#define ALIGN(x)     (((x) + sizeof(size_t) - 1) & MASK)
#define HOLE_SIZE(x) (ALIGN(x) + sizeof(size_t))


/* A structure that represents a mapped file. */
typedef struct mapped_file
{
    char *filename;   /* Name of mapped file */
#ifdef _WIN32
    HANDLE fd;        /* Handle to mapped file */
#else
    int fd;           /* File descriptor of mapped file */
#endif
    void *address;    /* Address where file was mapped */
    size_t size;      /* Size of mapped file */
    size_t pos;       /* Current position in mapped buffer */
    size_t eof;       /* Mapped file EOF position */
    rbtree_t *holes;  /* Red-black tree of holes in mapped buffer */
} mapped_file_t;


/* A structure that represents a hole in the mapped buffer. */
typedef struct hole
{
    size_t pos;       /* Position of hole in mapped buffer */
    size_t size;      /* Size of hole */
} hole_t;


ssize_t mapped_file_read(mapped_file_t *, void *, size_t);
ssize_t mapped_file_write(mapped_file_t *, void *, size_t);
int mapped_file_memset(mapped_file_t *, int, size_t);
int mapped_file_seek(mapped_file_t *, ssize_t, int);
size_t mapped_file_tell(mapped_file_t *);
size_t mapped_file_get_size(mapped_file_t *);
size_t mapped_file_get_eof(mapped_file_t *);

ssize_t mapped_file_allocate_chunk(mapped_file_t *, size_t);
void mapped_file_free_chunk(mapped_file_t *, size_t);

ssize_t mapped_file_marshal_object(mapped_file_t *, PyObject *);
PyObject *mapped_file_unmarshal_object(mapped_file_t *, size_t);

mapped_file_t *mapped_file_open(const char *);
mapped_file_t *mapped_file_create(const char *, size_t);
int mapped_file_sync(mapped_file_t *, size_t, size_t);
int mapped_file_set_access(mapped_file_t *, int);
int mapped_file_truncate(mapped_file_t *, size_t);
int mapped_file_rename(mapped_file_t *, const char *);
int mapped_file_unlink(mapped_file_t *);
void mapped_file_close(mapped_file_t *);

#endif /* _MAPPED_FILE_H_ */
