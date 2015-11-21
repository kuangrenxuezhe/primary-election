#ifndef RSYS_NEWS_OPTIONS_H
#define RSYS_NEWS_OPTIONS_H

#include <stdint.h>
#include <string>
#include "util/status.h"

namespace rsys {
  namespace news {
    struct Options {
      // 数据保存目录, 默认当前目录
      std::string work_path;
      // 表名, 默认："level_table"
      std::string table_name;
      // 新闻数据保留时间, 单位秒, 默认：1天
      int32_t item_hold_time;
      // 用户状态保留时间，单位秒, 默认：3周
      int32_t user_hold_time;
      // 层级表，最大层级数，默认：3
      int32_t max_table_level;
      // 新增数据可接受的过期时间
      // 超过过期时间则丢弃, 默认：2天
      int32_t new_item_max_age;
      // LOG文件过期天数, 单位秒，默认：7天
      int32_t log_expired_days; 
      // 周期性flush，格式：NN/Gap
      //  NN: 有效Gap范围内的数值
      //  Gap：mon 月(1-31), week 周(0-6)，day 天(0-23), hour: 时(0-59)
      // 样例: 23/day, 表示每天的23点进行flush操作
      // 默认：23/day
      std::string flush_timer;

      Options();
      // 从配置文件加载Option
      static Status fromConf(const std::string& conf, Options& opts);
    };
  } // namespace news
} // namespace rsys
#endif // #define RSYS_NEWS_OPTIONS_H

