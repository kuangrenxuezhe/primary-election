#ifndef RSYS_NEWS_BUFFER_H
#define RSYS_NEWS_BUFFER_H

#include <stddef.h>

namespace rsys {
  namespace news {

    struct buffer_ {
      int maxlen;
      char buffer[1];
    };
    typedef struct buffer_ buffer_t;

    inline buffer_t* newBuffer(int size) {
      int align_size = ((size/1024)+1)*1024;
      int struct_size = offsetof(buffer_t, maxlen);

      buffer_t* buffer = (buffer_t *)new char[align_size+struct_size];
      buffer->maxlen = align_size;
      return buffer;
    }

    inline void freeBuffer(void* buffer) {
      if (NULL != buffer)
        delete[] (char*)buffer;
    }

    inline buffer_t* renewBuffer(buffer_t* buffer, int new_size)
    {
      freeBuffer(buffer);
      return newBuffer(new_size);
    }
  }; // namespace news
}; // namespace rsys

#endif // #define RSYS_NEWS_BUFFER_H
