#ifndef SOUYUE_RECMD_MODELS_PRIMARY_ELECTION_OPTIONS_H
#define SOUYUE_RECMD_MODELS_PRIMARY_ELECTION_OPTIONS_H

#include <stdint.h>
#include <string>
#include "db/db/options.h"
#include "utils/status.h"

namespace souyue {
  namespace recmd {
    struct ModelOptions: public Options {
      // GRPC服务端口，默认：6200
      int rpc_port;
      // 服务监控端口，默认：16200
      int monitor_port;
      // 表名, 默认："level_table"
      std::string   table_name;
      // 默认推荐时间, 默认：1天
      int32_t interval_recommendation;
     // 用户阅读，推荐记录过期时长, 默认：1天
      int32_t profile_expired_time; 
      // 返回候选集最大个数,默认：5000
      int32_t max_candidate_set_size;
      // 返回候选视频最大个数,默认：10
      int32_t max_candidate_video_size;
      // 返回候选视频最大个数, 默认：10
      int32_t max_candidate_region_size;
      // 周期性flush, 默认：23/day
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

