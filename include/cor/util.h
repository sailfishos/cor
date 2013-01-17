#ifndef _COR_UTIL_H_
#define _COR_UTIL_H_

#include <string.h>
#include <errno.h>

/* MIN */
#include <sys/param.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(arr[0]))
#endif

struct name_id
{
     int id;
     char const *name;
};

static inline int id_from_name(struct name_id const *from, char const *name)
{
     int res = -1;
     while (from->name) {
          if (!strcmp(name, from->name)) {
               res = from->id;
               break;
          }
          ++from;
     }
     return res;
}

static inline int name_index(char const *name, char const *names[], size_t len)
{
     int res = -1, pos = 0;
     while (pos < (int)len) {
          if (!strcmp(name, names[pos])) {
               res = pos;
               break;
          }
          ++pos;
     }
     return res;
}

/** perform memcpy of maximum src_len bytes from src into dst + off no
 * more then len bytes.
 *
 * \return actual number of copied bytes or negative on error
 */
static inline int memcpy_offset
(char *dst, size_t len, size_t off, char const *src, size_t src_len)
{
    if (!dst || !src || off >= src_len)
        return -EINVAL;

    size_t actual_len = MIN(src_len - off, len + off);
    if (actual_len)
        memcpy(dst, &src[off], actual_len);
    return actual_len;
}

#endif // _COR_UTIL_H_
