#ifndef RSYS_NEWS_ITEM_TABLE_H
#define RSYS_NEWS_ITEM_TABLE_H

#include <stdint.h>
#include <set>
#include <list>

#include "core/options.h"
#include "core/core_type.h"
#include "util/status.h"
#include "util/table_base.h"
#include "proto/service.pb.h"

namespace rsys {
  namespace news {
    struct item_info_ {
      uint64_t       item_id; // itemid
      float            power; // 初选权重
      int32_t	  publish_time; // 发布时间
      int32_t      item_type; // item类型
      int32_t    picture_num; // 图片个数
      int32_t    click_count; // 点击计数
      int32_t     click_time; // 最近点击时间
      int32_t    category_id; // 所属分类, 用于返回
      map_str_t    region_id; // 所属地区
      map_str_t   belongs_to; // 所属分类, 圈子, SRP词
    };
    typedef struct item_info_ item_info_t;

    struct item_index_ {
      int32_t          index; // 滑窗内的偏移
      item_info_t* item_info; // item_info地址
    };
    typedef struct item_index_ item_index_t;

    struct query_ {
      int      item_type;
      int32_t start_time;
      int32_t   end_time;
    };
    typedef struct query_ query_t;

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

      public:
        // 添加item, item_info由外部分配内存(避免复制)
        Status addItem(item_info_t* item_info);
        // 更新用户点击
        Status updateAction(const action_t& action);
        // 查询item, 需设置item_info.item_id
        Status queryItem(item_info_t& item_info);
        // 获取候选集合
        Status queryCandidateSet(const query_t& query, candidate_set_t& candset);
        
      public:
        // 创建AheadLog对象
        virtual AheadLog* createAheadLog();
        virtual Status loadData(const std::string& data);
        virtual Status dumpToFile(const std::string& temp_name);

      protected:
        // 计算存储在滑窗内的物理位置
        int windowIndex(int32_t ctime);
        Status addItemIndex(int index, item_info_t* item_info);
        void addToList(item_list_t& item_list, item_info_t* term_info);

        int loadSlipWindow(const char* fullpath);
        int syncSlipWindow(const char* fullpath);

      private:
        Options               options_;
        int               window_size_; // 滑窗大小
        int               window_base_; // 滑窗基准位
        int32_t           window_time_; // 滑窗基准时间
        item_list_t*      item_window_; // 采用循环列表存储item，每个slot表示1小时
        pthread_rwlock_t* window_lock_;

        hash_map_t*        item_index_; // 存储item索引
        pthread_mutex_t    index_lock_;
    };
  }; // namespace news
}; // namespace rsys
#endif // #define RSYS_NEWS_ITEM_TABLE_H

