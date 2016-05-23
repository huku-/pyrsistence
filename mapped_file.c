/* pyrsistence - A Python extension for External Memory Data Structures (EMDs)
 * huku <huku@grhack.net>
 *
 * mapped_file.c - Simple API for memory mapped files.
 *
 * My original intention was to expose an API for memory mapped files similar to
 * this for `FILE' objects on most modern operating systems. However, the various
 * incompatibilities in the representation of `off_t', `fpos_t' and other data
 * types between the various operating systems, rendered this task impossible.
 * For example, at first, I assumed that `sizeof(off_t) >= sizeof(size_t)' on
 * most (all?) POSIX systems, but this was not the case on Microsoft Windows.
 * Using `fpos_t' instead of `off_t' is not a solution; the man pages on UNIX
 * systems clearly state that `fpos_t' is not necessarily a primitive type.
 *
 * In the following, I decided use `ssize_t' instead of `off_t' and `size_t'
 * instead of `fpos_t'.
 *
 * Bewarned that this code is still experimental and various integer related
 * bugs may be present. Let me know if you happen to hit any!
 */
#include "includes.h"
#include "marshaller.h"
#include "common.h"
#include "util.h"
#include "rbtree.h"
#include "mapped_file.h"


/* Holes in the memory mapped buffer are ordered by size in a red-black tree. */
static int hole_cmp(const void *a, const void *b)
{
    hole_t *ha = (hole_t *)a;
    hole_t *hb = (hole_t *)b;
    int ret = 0;

    if(ha->size < hb->size)
        ret = -1;
    if(ha->size > hb->size)
        ret = 1;
    return ret;
}


/* Allocate, initialize and return a new mapped file object. */
static mapped_file_t *mapped_file_alloc(const char *filename)
{
    mapped_file_t *mf;

    if((mf = PyMem_MALLOC(sizeof(mapped_file_t))) == NULL)
        goto _err1;

    memset(mf, 0, sizeof(mapped_file_t));

    if((mf->holes = rbtree_alloc(hole_cmp)) == NULL)
        goto _err2;

    if((mf->filename = PyMem_MALLOC(strlen(filename) + 1)) == NULL)
        goto _err3;

    strcpy(mf->filename, filename);
    return mf;

_err3:
    PyMem_FREE(mf->holes);

_err2:
    PyMem_FREE(mf);

_err1:
    return NULL;
}


/* Deallocate a mapped file object. */
static void mapped_file_free(mapped_file_t *mf)
{
    PyMem_FREE(mf->filename);
    rbtree_free(mf->holes);
    PyMem_FREE(mf);
}


/* Make sure it's safe to access `size' bytes starting from position `pos'. */
static int mapped_file_check_range(mapped_file_t *mf, size_t pos,
        size_t size)
{
    return (size <= SSIZE_MAX && pos <= SSIZE_MAX && pos + size <= mf->size);
}


/* Make sure we can write at least `size' bytes starting from position `pos' in
 * mapped file `mf'. Mapped file may be resized to fulfil the request.
 */
static int mapped_file_ensure_range(mapped_file_t *mf, size_t pos,
        size_t size)
{
    size_t mf_size = mf->size;
    int ret = -1;

    if(size > SSIZE_MAX || pos > SSIZE_MAX)
        goto _err;

    if(mf_size < pos + size)
    {
        while(mf_size < pos + size)
        {
            if(mf_size + (mf_size >> 1) < mf_size)
                goto _err;
            mf_size += mf_size >> 1;
        }

        if(mapped_file_truncate(mf, mf_size) != 0)
            goto _err;
    }

    ret = 0;

_err:
    return ret;
}


/* Equivalent to `read()' for memory mapped files. */
ssize_t mapped_file_read(mapped_file_t *mf, void *buf, size_t size)
{
    size_t mf_pos = mf->pos;
    ssize_t ret = -1;

    if(mapped_file_check_range(mf, mf_pos, size) == 0)
        goto _err;

    memcpy(buf, (char *)mf->address + mf_pos, size);
    mf->pos += size;
    ret = size;

_err:
    return ret;
}


/* Equivalent to `write()' for memory mapped files. */
ssize_t mapped_file_write(mapped_file_t *mf, void *buf, size_t size)
{
    size_t mf_pos = mf->pos;
    ssize_t ret = -1;

    if(mapped_file_ensure_range(mf, mf_pos, size) != 0)
        goto _err;

    memcpy((char *)mf->address + mf_pos, buf, size);

    mf_pos += size;
    if(mf_pos > mf->eof)
        mf->eof = mf_pos;
    mf->pos = mf_pos;

    ret = size;

_err:
    return ret;
}


/* Safe `memset()' of a mapped file's contents. Sets `size' bytes starting from
 * position `pos'.
 */
int mapped_file_memset(mapped_file_t *mf, int c, size_t size)
{
    size_t mf_pos = mf->pos;
    int ret = -1;

    if(mapped_file_ensure_range(mf, mf_pos, size) != 0)
        goto _err;

    memset((char *)mf->address + mf_pos, c, size);

    mf_pos += size;
    if(mf_pos > mf->eof)
        mf->eof = mf_pos;
    mf->pos = mf_pos;

    ret = 0;

_err:
    return ret;
}


/* Equivalent to `fseek()' for memory mapped files. */
int mapped_file_seek(mapped_file_t *mf, ssize_t off, int whence)
{
    int ret = -1;

    if(whence == SEEK_CUR)
        off += mf->pos;
    else if(whence == SEEK_END)
        off += mf->size;

    /* Remember that `mf->size <= SSIZE_MAX' in `map_file()'. */
    if(off >= 0 && (size_t)off <= mf->size)
    {
        mf->pos = (size_t)off;
        ret = 0;
    }

    return ret;
}


/* Equivalent to `ftell()' for memory mapped files. */
size_t mapped_file_tell(mapped_file_t *mf)
{
    return mf->pos;
}


/* Get mapped buffer size of mapped file `mf'. */
size_t mapped_file_get_size(mapped_file_t *mf)
{
    return mf->size;
}


/* Get current EOF position of mapped file `mf'. */
size_t mapped_file_get_eof(mapped_file_t *mf)
{
    return mf->eof;
}


/* Return the position of a chunk of size `size' in memory mapped file `mf'.
 * It's safe to seek there and write `size' bytes. On error -1 is returned.
 */
ssize_t mapped_file_allocate_chunk(mapped_file_t *mf, size_t size)
{
    hole_t hole;
    rbnode_t *node;
    size_t pos;

    ssize_t ret = -1;


    if(size > SSIZE_MAX)
        goto _err;

    size = HOLE_SIZE(size);

    hole.pos = 0;
    hole.size = size;
    node = rbtree_find_node(mf->holes, &hole);

    if(node == NULL)
    {
        pos = mf->eof;
        if(mapped_file_seek(mf, pos, SEEK_SET) != 0)
            goto _err;

        if(mapped_file_write(mf, &size, sizeof(size_t)) != sizeof(size_t))
            goto _err;

        size -= sizeof(size_t);
        if(mapped_file_memset(mf, 0, size) != 0)
            goto _err;
    }
    else
    {
        pos = ((hole_t *)node->data)->pos;
        PyMem_FREE(node->data);
        rbtree_delete_node(mf->holes, node);
    }

    ret = (ssize_t)(pos + sizeof(size_t));

_err:
    return ret;
}


/* Mark the chunk at position `pos' in mapped file `mf' as free. Chunk must have
 * been allocated using `mapped_file_allocate_chunk()'.
 */
void mapped_file_free_chunk(mapped_file_t *mf, size_t pos)
{
    hole_t *hole;
    size_t size;

    pos -= sizeof(size_t);
    if(mapped_file_seek(mf, pos, SEEK_SET) != 0)
        goto _err;

    if(mapped_file_read(mf, &size, sizeof(size_t)) != sizeof(size_t))
        goto _err;

    if(mapped_file_check_range(mf, pos, size) == 0)
        goto _err;

    if((hole = PyMem_MALLOC(sizeof(hole_t))) == NULL)
        goto _err;

    hole->pos = pos;
    hole->size = size;
    rbtree_insert_node(mf->holes, hole);

_err:
    return;
}


/* Allocate a chunk of appropriate size from mapped file `mf' and marshal Python
 * string object `obj' in it.
 */
static ssize_t mapped_file_marshal_string_object(mapped_file_t *mf,
        PyObject *obj)
{
    Py_ssize_t size;
    char *data;
    ssize_t pos, ret = -1;

    if(PyString_AsStringAndSize(obj, &data, &size) == -1)
        goto _err;

    if((pos = mapped_file_allocate_chunk(mf, (size_t)size)) < 0)
        goto _err;

    if(pos != mapped_file_tell(mf) &&
            mapped_file_seek(mf, pos, SEEK_SET) != 0)
    {
        mapped_file_free_chunk(mf, pos);
        goto _err;
    }

    if(mapped_file_write(mf, data, size) != size)
    {
        mapped_file_free_chunk(mf, pos);
        goto _err;
    }

    ret = pos;

_err:
    return ret;
}


/* Allocate a chunk of appropriate size from mapped file `mf' and marshal Python
 * object `obj' in it.
 */
ssize_t mapped_file_marshal_object(mapped_file_t *mf, PyObject *obj)
{
    PyObject *str;

    ssize_t pos = -1;

    if((str = marshal(obj)) == NULL)
        goto _err;

    pos = mapped_file_marshal_string_object(mf, str);
    Py_DECREF(str);

_err:
    return pos;
}


/* Return the size of chunk at position `pos' in mapped file `mf'. Chunk must
 * have been allocated using `mapped_file_allocate_chunk()'.
 */
static ssize_t mapped_file_get_chunk_size(mapped_file_t *mf, size_t pos)
{
    size_t size;
    ssize_t ret = -1;

    pos -= sizeof(size_t);
    if(pos != mapped_file_tell(mf) &&
            mapped_file_seek(mf, (ssize_t)pos, SEEK_SET) != 0)
        goto _err;

    if(mapped_file_read(mf, &size, sizeof(size_t)) != sizeof(size_t))
        goto _err;

    ret = (ssize_t)(size - sizeof(size_t));

_err:
    return ret;
}


/* Unmarshal a Python object from position `pos' in memory mapped file `mf'. */
PyObject *mapped_file_unmarshal_object(mapped_file_t *mf, size_t pos)
{
    ssize_t size;
    PyObject *str, *ret = NULL;

    /* The call to `mapped_file_get_chunk_size()' will seek to the correct
     * position in the memory mapped file.
     */
    if((size = mapped_file_get_chunk_size(mf, pos)) < 0)
        goto _err;

    if((str = PyString_FromStringAndSize((char *)mf->address + pos, size)) == NULL)
        goto _err;

    ret = unmarshal(str);
    Py_DECREF(str);

_err:
    return ret;
}


#ifdef _WIN32

/* Truncate file `fd' at `size' bytes and map it in memory. */
static void *map_file(HANDLE fd, LARGE_INTEGER size)
{
    LARGE_INTEGER disk_size, cur_off;
    HANDLE h;
    void *address;


    if(size.QuadPart < 0 || size.QuadPart > SSIZE_MAX)
        goto _err1;

    GetFileSizeEx(fd, &disk_size);

    if(size.QuadPart == 0)
        size = disk_size;

    /* Quoting the MSDN page for `SetFilePointerEx()':
     *
     * "If the file is extended, the contents of the file between the old end of
     * the file and the new end of the file are not guaranteed to be zero; the
     * behavior is defined by the file system."
     *
     * Looks like for NTFS, the extended part of the file is filled with zeros.
     */
    if(SetFilePointerEx(fd, size, &cur_off, FILE_BEGIN) == FALSE)
    {
        serror("map_file: SetFilePointerEx");
        goto _err1;
    }

    if(SetEndOfFile(fd) == FALSE)
    {
        serror("map_file: SetEndOfFile");
        goto _err2;
    }

    if((h = CreateFileMappingA(fd, NULL, PAGE_READWRITE, 0, 0, NULL)) == NULL)
    {
        serror("map_file: CreateFileMappingA");
        goto _err3;
    }

    if((address = MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, 0)) == NULL)
        serror("map_file: MapViewOfFile");

    /* We need not keep a reference to the file mapping object, we can safely
     * close it now. Quoting the MSDN page for `CreateFileMapping()':
     *
     * "Mapped views of a file mapping object maintain internal references to
     * the object, and a file mapping object does not close until all references
     * to it are released. Therefore, to fully close a file mapping object, an
     * application must unmap all mapped views of the file mapping object by
     * calling UnmapViewOfFile and close the file mapping object handle by
     * calling CloseHandle. These functions can be called in any order."
     */
    CloseHandle(h);
    return address;

_err3:
    SetFilePointerEx(fd, disk_size, NULL, FILE_BEGIN);
    SetEndOfFile(fd);

_err2:
    SetFilePointerEx(fd, cur_off, NULL, FILE_BEGIN);

_err1:
    return NULL;
}


/* Open existing file `filename' and map it in memory. */
mapped_file_t *mapped_file_open(const char *filename)
{
    HANDLE fd;
    LARGE_INTEGER disk_size;
    void *address;
    mapped_file_t *mf;

    if((fd = CreateFileA(filename, GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
            OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
    {
        serror("mapped_file_open: CreateFileA");
        goto _err1;
    }

    GetFileSizeEx(fd, &disk_size);

    if((address = map_file(fd, disk_size)) == NULL)
        goto _err2;

    if((mf = mapped_file_alloc(filename)) == NULL)
        goto _err3;

    mf->fd = fd;
    mf->address = address;
    mf->size = (size_t)disk_size.QuadPart;
    mf->eof = (size_t)disk_size.QuadPart;
    return mf;

_err3:
    UnmapViewOfFile(address);

_err2:
    CloseHandle(fd);

_err1:
    return NULL;
}


/* Create file `filename', truncate it at `size' bytes and map it in memory. */
mapped_file_t *mapped_file_create(const char *filename, size_t size)
{
    HANDLE fd;
    LARGE_INTEGER lsize;
    void *address;
    mapped_file_t *mf;

    if((fd = CreateFileA(filename, GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
        CREATE_ALWAYS, 0, NULL)) == INVALID_HANDLE_VALUE)
    {
        serror("mapped_file_create: CreateFileA");
        goto _err1;
    }

    lsize.QuadPart = size;
    if((address = map_file(fd, lsize)) == NULL)
        goto _err2;

    if((mf = mapped_file_alloc(filename)) == NULL)
        goto _err3;

    mf->fd = fd;
    mf->address = address;
    mf->size = size;
    return mf;

_err3:
    UnmapViewOfFile(address);

_err2:
    CloseHandle(fd);

_err1:
    return NULL;
}


/* Synchronize memory contents to disk. */
int mapped_file_sync(mapped_file_t *mf, size_t pos, size_t size)
{
    int ret = -1;

    if(mapped_file_check_range(mf, pos, size) == 0)
        goto _err;

    ret = FlushViewOfFile((char *)mf->address + pos, size);

_err:
    return ret;
}


/* Give kernel a hint about the type of access we are going to perform. It's a
 * no-op for now; may be implemented in the future using `ReOpenFile()'.
 */
int mapped_file_set_access(mapped_file_t *mf, int advice)
{
    UNREFERENCED_PARAMETER(mf);
    UNREFERENCED_PARAMETER(advice);
    return 0;
}


/* Equivalent to `ftruncate()' for memory mapped files. */
int mapped_file_truncate(mapped_file_t *mf, size_t size)
{
    LARGE_INTEGER lsize;
    void *address;

    void *mf_address = mf->address;
    int ret = -1;

    lsize.QuadPart = size;
    if((address = map_file(mf->fd, lsize)) == NULL)
        goto _err;

    /* Unmap the old mapping only as soon as we have succesfully allocated a new
     * one. Unfortunately, there's no `mremap()' equivalent on Microsoft Windows.
     */
    UnmapViewOfFile(mf_address);

    mf->address = address;
    mf->size = size;
    if(mf->pos > size)
        mf->pos = size;
    if(mf->eof > size)
        mf->eof = size;

    ret = 0;

_err:
    return ret;
}


/* Equivalent to `rename()' for memory mapped files. */
int mapped_file_rename(mapped_file_t *mf, const char *filename)
{
    char *filename_p;

    int ret = -1;

    if((filename_p = PyMem_MALLOC(strlen(filename) + 1)) == NULL)
        goto _err;

    strcpy(filename_p, filename);

    if(MoveFileExA(mf->filename, filename,
            MOVEFILE_WRITE_THROUGH | MOVEFILE_REPLACE_EXISTING) == 0)
    {
        serror("mapped_file_rename: MoveFileExA");
        PyMem_FREE(filename_p);
        goto _err;
    }

    PyMem_FREE(mf->filename);
    mf->filename = filename_p;

    ret = 0;

_err:
    return ret;
}


/* Instructs Microsoft Windows to delete the file once it's closed. We can't
 * directly delete the file in any other way and the whole process should be
 * transparent to the user.
 */
int mapped_file_unlink(mapped_file_t *mf)
{
    HANDLE fd;

    int ret = -1;

    if((fd = ReOpenFile(mf->fd,  GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_FLAG_DELETE_ON_CLOSE)) == INVALID_HANDLE_VALUE)
    {
        serror("mapped_file_unlink: ReOpenFile");
        goto _err;
    }

    CloseHandle(mf->fd);
    mf->fd = fd;

    ret = 0;

_err:
    return ret;
}


/* Equivalent to `close()' for memory mapped files. It's not safe to access `mf'
 * after a call to `mapped_file_close()'.
 */
void mapped_file_close(mapped_file_t *mf)
{
    UnmapViewOfFile(mf->address);
    CloseHandle(mf->fd);
    mapped_file_free(mf);
}


#else

/* Truncate file `fd' at `size' bytes and map it in memory. */
static void *map_file(int fd, size_t size)
{
    struct stat st;

    void *address = NULL;

    if(size > SSIZE_MAX)
        goto _err;

    /* Use `fstat()' to read file's original size. */
    if(fstat(fd, &st) != 0)
    {
        serror("map_file: fstat");
        goto _err;
    }

    if(ftruncate(fd, size) != 0)
    {
        serror("map_file: ftruncate");
        goto _err;
    }

    address = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if(address == MAP_FAILED)
    {
        /* Restore original file size on failure to avoid undefined behaviour
         * when accessing a memory mapped region for a file that has, for
         * example, been shrunk by `ftruncate()'.
         */
        ftruncate(fd, st.st_size);
        serror("map_file: mmap");
        goto _err;
    }

_err:
    return address;
}


/* Open existing file `filename' and map it in memory. */
mapped_file_t *mapped_file_open(const char *filename)
{
    int fd;
    struct stat st;
    void *address;
    mapped_file_t *mf;


    if((fd = open(filename, O_RDWR)) < 0)
    {
        serror("mapped_file_open: fopen");
        goto _err1;
    }

    if(fstat(fd, &st) != 0)
    {
        serror("mapped_file_open: fstat");
        goto _err2;
    }

    if((address = map_file(fd, st.st_size)) == NULL)
        goto _err2;

    if((mf = mapped_file_alloc(filename)) == NULL)
        goto _err3;

    mf->fd = fd;
    mf->address = address;
    mf->size = st.st_size;
    mf->eof = st.st_size;
    return mf;

_err3:
    munmap(address, st.st_size);

_err2:
    close(fd);

_err1:
    return NULL;
}


/* Create file `filename', truncate it at `size' bytes and map it in memory. */
mapped_file_t *mapped_file_create(const char *filename, size_t size)
{
    int fd;
    void *address;
    mapped_file_t *mf;


    if((fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0600)) < 0)
    {
        serror("mapped_file_create: open");
        goto _err1;
    }

    if((address = map_file(fd, size)) == NULL)
        goto _err2;

    if((mf = mapped_file_alloc(filename)) == NULL)
        goto _err3;

    mf->fd = fd;
    mf->address = address;
    mf->size = size;
    return mf;

_err3:
    munmap(address, size);

_err2:
    close(fd);
    unlink(filename);

_err1:
    return NULL;
}


/* Synchronize memory contents to disk. */
int mapped_file_sync(mapped_file_t *mf, size_t pos, size_t size)
{
    int ret = -1;

    if(mapped_file_check_range(mf, pos, size) == 0)
        goto _err;

    ret = msync((char *)mf->address + pos, size, MS_SYNC);

_err:
    return ret;
}


/* Give kernel a hint about the type of access we are going to perform. */
int mapped_file_set_access(mapped_file_t *mf, int advice)
{
    int ret = -1;

    if(advice != MF_ACCESS_NORMAL && advice != MF_ACCESS_RANDOM &&
            advice != MF_ACCESS_SEQUENTIAL)
        goto _err;

    if((ret = madvise(mf->address, mf->size, advice)) < 0)
        serror("mapped_file_set_access: madvise");
_err:
    return ret;
}


/* Equivalent to `ftruncate()' for memory mapped files. */
int mapped_file_truncate(mapped_file_t *mf, size_t size)
{
    long pagesize;

    void *mf_address = mf->address, *address = NULL;
    size_t mf_size = mf->size;
    int mf_fd = mf->fd;


    if(size > SSIZE_MAX)
        goto _err1;

    if(size == mf_size)
        goto _ok;


    /* Truncate the file to the requested size first. If resizing the memory
     * mapping fails, we can easily restore the original file size by calling
     * `ftruncate()' again.
     */
    if(ftruncate(mf_fd, size) != 0)
    {
        serror("mapped_file_truncate: ftruncate");
        goto _err1;
    }

    /* When shrinking the memory mapped file, just unmap part of the mapping. */
    if(size < mf_size)
    {
        /* According to the manual page, this is the portable way of reading the
         * system's page size.
         */
        if((pagesize = sysconf(_SC_PAGESIZE)) == -1)
        {
            serror("mapped_file_truncate: sysconf");
            goto _err2;
        }

        /* Align sizes to the next multiple of the page size, as `munmap()' will
         * unmap any page overlapping with the given argument.
         */
        size = (size + pagesize - 1) & ~(pagesize - 1);
        mf_size = (mf_size + pagesize - 1) & ~(pagesize - 1);

        if(munmap((char *)mf_address + size, mf_size - size) != 0)
        {
            serror("mapped_file_truncate: munmap");
            goto _err2;
        }

        address = mf_address;
    }

    /* When increasing the size of the memory mapped file, we have to re-map the
     * underlying file pages.
     */
    else if(size > mf_size)
    {
#if defined __linux__ || defined __NetBSD__
        /* Linux and NetBSD implement `mremap()'. I haven't tested the code on
         * NetBSD, but this is good news anyway.
         */
        address = mremap(mf_address, mf_size, size, MREMAP_MAYMOVE);

        if(address == MAP_FAILED)
        {
            serror("mapped_file_truncate: mremap");
            goto _err2;
        }
#else
        /* On other OSes, like MacOS X and FreeBSD, we have to take the slow path
         * which involves creating a larger memory mapping and then unmapping the
         * old one.
         */
        address = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, mf_fd, 0);

        if(address == MAP_FAILED)
        {
            serror("mapped_file_truncate: mmap");
            goto _err2;
        }

        munmap(mf_address, mf_size);
#endif
    }

    mf->address = address;
    mf->size = size;
    if(mf->pos > size)
        mf->pos = size;
    if(mf->eof > size)
        mf->eof = size;

_ok:
    return 0;

_err2:
    ftruncate(mf_fd, mf_size);

_err1:
    return -1;
}


/* Equivalent to `rename()' for memory mapped files. */
int mapped_file_rename(mapped_file_t *mf, const char *filename)
{
    char *filename_p;

    int ret = -1;

    if((filename_p = PyMem_MALLOC(strlen(filename) + 1)) == NULL)
        goto _err;

    strcpy(filename_p, filename);

    if(rename(mf->filename, filename) != 0)
    {
        serror("mapped_file_rename: rename");
        PyMem_FREE(filename_p);
        goto _err;
    }

    PyMem_FREE(mf->filename);
    mf->filename = filename_p;

    ret = 0;

_err:
    return ret;
}


/* Equivalent to `unlink()' for memory mapped files. */
int mapped_file_unlink(mapped_file_t *mf)
{
    return unlink(mf->filename);
}


/* Equivalent to `close()' for memory mapped files. It's not safe to access `mf'
 * after a call to `mapped_file_close()'.
 */
void mapped_file_close(mapped_file_t *mf)
{
    munmap(mf->address, mf->size);
    close(mf->fd);
    mapped_file_free(mf);
}

#endif /* _WIN32 */

