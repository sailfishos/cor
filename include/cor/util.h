#ifndef _COR_UTIL_H_
#define _COR_UTIL_H_

#include <string.h>

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

#endif // _COR_UTIL_H_
