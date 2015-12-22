#ifndef SOUYUE_RECMD_MODELS_PRIMARY_ELECTION_OPTIONS_H
#define SOUYUE_RECMD_MODELS_PRIMARY_ELECTION_OPTIONS_H

#include <stdint.h>
#include <string>
#include "utils/status.h"

namespace souyue {
  namespace recmd {
    struct ModelOptions {
      // GRPC服务端口，默认：6200
      int rpc_port;
      // 服务监控端口，默认：16200
      int monitor_port;
      // 数据保存目录, 默认当前目录
      std::string    work_path;
      // 表名, 默认："level_table"
      std::string   table_name;
      // 新闻数据保留时间, 单位秒, 默认：1天
      int32_t   item_hold_time;
      // 用户状态保留时间，单位秒, 默认：3周
      int32_t   user_hold_time;
      // 层级表，最大层级数，默认：3
      int32_t  max_table_level;
      // 新增数据可接受的过期时间
      // 超过过期时间则丢弃, 默认：2天
      int32_t new_item_max_age;
      // 默认推荐时间, 默认：1天
      int32_t interval_recommendation;
     // 用户阅读，推荐记录过期时长, 默认：1天
      int32_t profile_expired_time; 
      // 置顶数据过期时间，默认：1天
      int32_t top_item_max_age;
      // 返回候选集最大个数,默认：5000
      int32_t max_candidate_set_size;
      // 返回候选视频最大个数,默认：10
      int32_t max_candidate_video_size;
      // 返回候选视频最大个数, 默认：10
      int32_t max_candidate_region_size;
      // 周期性flush，格式：NN/Gap
      //  NN: 有效Gap范围内的数值
      //  Gap：mon 月(1-31), week 周(0-6)，day 天(0-23), hour: 时(0-59)
      // 样例: 23/day, 表示每天的23点进行flush操作
      // 默认：23/day
      std::string  flush_timer;
      // 服务类型，0 表示推荐，1 表示圈子订阅 默认：0
      int32_t service_type;

      ModelOptions();
      // 从配置文件加载Option
      static Status fromConf(const std::string& conf, ModelOptions& opts);
    };
  } // namespace recmd 
} // namespace souyue
#endif // #define SOUYUE_RECMD_MODELS_PRIMARY_ELECTION_OPTIONS_H

