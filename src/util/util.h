#ifndef RSYS_NEWS_UTIL_H
#define RSYS_NEWS_UTIL_H

#include <string>
#include <ctime>

namespace rsys {
  namespace news {
    inline std::string timeToString(time_t ctime) {
      char timebuf[20];
      struct tm *stm = std::localtime(&ctime);
      std::strftime(timebuf, 20, "%Y/%m/%d %H:%M:%S", stm);
      return std::string(timebuf);
    }

    inline int32_t timeDiff(const struct timeval& start, const struct timeval& end) {
      return ((end.tv_sec*1000000UL+end.tv_usec) - (start.tv_sec*1000000UL+start.tv_usec))/1000;
    }
  } // namespace news
} // namespace rsys
#endif // #define RSYS_NEWS_UTIL_H

