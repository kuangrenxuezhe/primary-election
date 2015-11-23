#ifndef RSYS_NEWS_UTIL_H
#define RSYS_NEWS_UTIL_H

#include <string>
#include <ctime>
#include <openssl/md5.h>

namespace rsys {
  namespace news {
    inline std::string timeToString(time_t ctime) {
      char timebuf[20];
      struct tm *stm = std::localtime(&ctime);
      std::strftime(timebuf, 20, "%Y/%m/%d %H:%M:%S", stm);
      return std::string(timebuf);
    }

    // 计算时间差，单位毫秒
    inline int32_t timeDiff(const struct timeval& start, const struct timeval& end) {
      return ((end.tv_sec*1000000UL+end.tv_usec) - (start.tv_sec*1000000UL+start.tv_usec))/1000;
    }

    // 生成59位的MD5值
    inline uint64_t makeID(const char* str, size_t len) {
      MD5_CTX ctx;
      uint8_t digest[16];

      MD5_Init(&ctx);
      MD5_Update(&ctx, str, len);
      MD5_Final(digest, &ctx);
      return (*(uint64_t*)digest)&0x7FFFFFFFFFFFFFFUL;
    }

    inline uint64_t makeID(const std::string& str) {
      return makeID(str.c_str(), str.length());
    }
  } // namespace news
} // namespace rsys
#endif // #define RSYS_NEWS_UTIL_H

