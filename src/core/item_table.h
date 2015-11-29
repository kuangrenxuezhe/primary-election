#ifndef RSYS_NEWS_ITEM_TABLE_H
#define RSYS_NEWS_ITEM_TABLE_H

#include <stdint.h>
#include <set>
#include <list>
#include <atomic>

#include "status.h"
#include "table_base.h"
#include "core/options.h"
#include "core/core_type.h"
#include "proto/service.pb.h"
#include "sparsehash/dense_hash_map"

namespace rsys {
  namespace news {
    struct item_index_ {
      int32_t          index; // 滑窗内的偏移
      item_info_t* item_info; // item_info地址
    };
    typedef struct item_index_ item_index_t;

    typedef std::list<item_info_t*> item_list_t;

    class ItemTable: public TableBase {
      public:
        typedef google::dense_hash_map<uint64_t, item_index_t> hash_map_t;

      public:
        ItemTable(const Options& opts);
        ~ItemTable();
      
      public:
        // 淘汰过期数据, 更新滑窗基准时间
        Status eliminate();
        // 添加新闻数据, 写ahead-log
        Status addItem(const Item& item);
        // 用户操作状态更新, 写ahead-log
        Status updateAction(const Action& action);

      public:
        // 添加item, item_info由外部分配内存, 且不写ahead-log
        Status addItem(item_info_t* item_info);
        // 更新用户点击, 且不写ahead-log
        Status updateAction(uint64_t item_id, const action_t& action);
        // 查询item 
        Status queryItem(uint64_t item_id, item_info_t& item_info);
        // 获取候选集合
        Status queryCandidateSet(const query_t& query, candidate_set_t& candset);
        
      public:
        // 创建AheadLog对象
        virtual AheadLog* createAheadLog();
        virtual Status loadData(const std::string& data);
        virtual Status dumpToFile(const std::string& temp_name);

      protected:
        bool isObsolete(int32_t publish_time);
        bool isBelongsTo(uint64_t region_id, const map_pair_t& regions);
        // 计算存储在滑窗内的物理位置
        int windowIndex(int32_t ctime);
        Status addItemIndex(int index, item_info_t* item_info);
        void addToList(item_list_t& item_list, item_info_t* term_info);

      private:
        Options               options_;
        int               window_size_; // 滑窗大小
        std::atomic_int   window_base_; // 滑窗基准位
        std::atomic_int   window_time_; // 滑窗基准时间
        item_list_t*      item_window_; // 采用循环列表存储item，每个slot表示1小时
        pthread_rwlock_t* window_lock_;

        hash_map_t*        item_index_; // 存储item索引
        pthread_mutex_t    index_lock_;
        friend class      ItemAheadLog;
    };
  }; // namespace news
}; // namespace rsys
#endif // #define RSYS_NEWS_ITEM_TABLE_H

