#ifndef RSYS_NEWS_ITEM_TABLE_H
#define RSYS_NEWS_ITEM_TABLE_H

#include <stdint.h>
#include <set>
#include <list>

#include "core/options.h"
#include "core/core_type.h"
#include "util/status.h"
#include "util/base_table.h"
#include "util/level_table.h"
#include "proto/service.pb.h"
#include "sparsehash/dense_hash_map"

namespace rsys {
  namespace news {
    struct item_info_ {
      uint64_t item_id; // itemid
      int32_t	publish_time; // 发布时间
      float primary_power; // 初选权重
      int32_t item_type:16; // item类型(0 咨询，1 视频)
      int32_t picture_num:16; // 图片个数
      int32_t category_id; // 分类ID
      int64_t region_id; // 所属地区
      id_set_t circle_and_srp; //所属圈子和SRP词
    };
    typedef struct item_info_ item_info_t;

    struct item_index_ {
      int32_t click_count; // 点击计数
      int32_t click_time; // 最近点击时间
      item_info_t* item_info;
    };
    typedef struct item_index_ item_index_t;

    struct query_ {
      int item_type;
      int32_t start_time;
      int32_t end_time;
    };
    typedef struct query_ query_t;

    typedef std::list<item_info_t*> item_list_t;

    class ItemTable: public BaseTable<item_index_t> {
      public:
        ItemTable(const Options& opts);
        ~ItemTable();

      public:
        // 写入表的版本号
        fver_t tableVersion() const {
          return fver_;
        }
        virtual const std::string tableName() const {
          return options_.work_path + "/" + options_.table_name;
        }
        virtual const std::string workPath() const {
          return options_.work_path;
        }

      public:
      // 淘汰用户，包括已读，不喜欢和推荐信息
        Status eliminate(int32_t hold_time);

      public:
        // 添加item, item_info由外部分配内存(避免复制)
        Status addItem(item_info_t* item_info);
        Status updateAction(const action_t& item_action);

        Status queryCandidateSet(const query_t& query, candidate_set_t& candset);

      public:
        virtual bool parseFrom(const std::string& data, uint64_t* item_id, item_index_t* item_index);
        virtual bool serializeTo(uint64_t item_id, const item_index_t* item_index, std::string& data);

      protected:
        int getWindowIndex(int32_t ctime);
        Status addItemIndex(item_info_t* item_info);

        int loadSlipWindow(const char* fullpath);
        int syncSlipWindow(const char* fullpath);

      protected:
        virtual AheadLog* createAheadLog() {
          return NULL;
        }
        virtual value_t* newValue() {
          return NULL;
        }

      private:
        static fver_t fver_;
        const Options& options_;

        // 滑窗大小
        int window_size_; 
        // 滑窗基准位
        int window_base_;
        // 滑窗基准时间
        int32_t window_time_;
        
        // 采用循环列表存储item，每个slot表示1小时
        item_list_t* item_window_;
        pthread_rwlock_t* window_lock_;
    };
  }; // namespace news
}; // namespace rsys
#endif // #define RSYS_NEWS_ITEM_TABLE_H
