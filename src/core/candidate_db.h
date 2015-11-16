#ifndef RSYS_NEWS_CANDIDATE_DB_H
#define RSYS_NEWS_CANDIDATE_DB_H

#include <pthread.h>
#include <vector>

#include "util/status.h"
#include "util/options.h"
#include "util/wal.h"
#include "core/user_table.h"
#include "core/item_table.h"
#include "proto/news_rsys.pb.h"

namespace rsys {
  namespace news {
    class CandidateDB {
      public:
        CandidateDB(const Options& opts);
        ~CandidateDB();

      public:
        static Status openDB(const Options& opts, CandidateDB** dbptr);

      public:
        // 可异步方式，线程安全
        Status flush();
        Status reload();

        //与flush采用不同的周期,只保留days天
        Status rollOverWALFile(int32_t expired_days);

      public:
        // 查询用户是否在用户表中
        bool findUser(uint64_t user_id);

        // 添加新闻数据
        Status addItem(const ItemInfo& item);
        
        // 用户操作状态更新
        Status updateAction(const UserAction& action);
        // 全局更新用户订阅信息
        Status updateUser(const UserSubscribe& subscribe);
        // 更新用户候选集合
        Status updateCandidateSet(uint64_t user_id, const IdSet& id_set);

        // 获取用户已阅读的历史记录
        Status queryHistory(uint64_t user_id, IdSet& history);
        // 获取待推荐的候选集
        Status queryCandidateSet(const Query& query, CandidateSet& candidate_set);

      private:
        Options options_;
        UserTable* user_table_;
        ItemTable* item_table_;
        // 只是记录，该部分数据暂时没用
        WALWriter* writer_;
        pthread_mutex_t mutex_; 
    };
  }; // namespace news
}; // namespace rsys
#endif // #define RSYS_NEWS_CANDIDATE_DB_H

/*
#ifndef CF_CANDIDATE_H
#define CF_CANDIDATE_H

#include <stdint.h>

#include <cstdio>
#include <cstring>
#include <map>
#include <vector>
#include <list>
#include <iostream>
#include <fstream>

#include "util/UC_MD5.h"
#include "util/UH_Define.h"
#include "util/UC_LogManager.h"
#include "util/UC_Persistent_Storage.h"
#include "util/UC_Allocator_Recycle.h"

#include "sparsehash/dense_hash_map"
#include "framework/CF_framework_interface.h"
#include "framework/CF_framework_config.h"

#define RESERVED_MAX_DAYS  (7) // 保留历史的最大天数
#define RESERVED_TIME_SLOT (24) // 按照每小时切分历史数据
#define SLIP_WINDOW_SIZE	(RESERVED_MAX_DAYS*RESERVED_TIME_SLOT)
#define SECOND_PER_HOUR    (3600)

#define SLIP_ITEM_NUM		(1000000)
#define CIRCLE_AND_SRP_NUM	(200)
#define MAX_BUFFER_SIZE		(1<<24)

#define HEAP_SIZE			(1000)
#define SECOND_PER_DAY		(86400)
#define ITEM_KEEP_TIME		(SECOND_PER_DAY*3)

#define MAX_FUN(a,b)	((a)>(b)?(a):(b))
#define MIN_FUN(a,b) 	((a)<(b)?(a):(b))

const var_4 UPDATE_USER 	= 1001;
const var_4 UPDATE_ITEM 	= 1002;
const var_4 UPDATE_CLICK	= 1003;
const var_4 RESPONSE_RECOMMEND	= 1004;
const var_4 CATEGORY_NUM = 13;
const var_4 default_category_id = 13;

#pragma pack(4)
struct itemid_ {
  uint64_t id;  // itemid
  int32_t ctime; // 添加时间, 用于时间淘汰
};
typedef struct itemid_ itemid_t;
#pragma pack()

typedef std::set<uint64_t> id_set_t;
typedef std::set<itemid_t> itemid_set_t;

struct user_info_ {
  id_set_t	circle_and_srp;	// 圈子或者SRP词     

  itemid_set_t	dislike;			// 不喜欢新闻集合
  itemid_set_t  readed;			// 已阅新闻集合
  itemid_set_t	recommended;	// 已推荐新闻集合
};
typedef struct user_info_ user_info_t;

struct item_info_ {
  uint64_t itemid; // itemid
  float primary_power; // 初选权重
  int32_t picture_num:24; // 图片个数
  int32_t item_type:8; // item类型(0 咨询，1 视频)
  int32_t category_id; // 分类ID
  int32_t	publish_time; // 发布时间
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

typedef std::list<item_info_t*> item_list_t;

struct slip_window_ {
  int32_t index_base; // 基准位
  int32_t index_cursor; // 当前位置
  item_list_t* item_window; // 滑窗数组
};
typedef struct slip_window_ slip_window_t;

struct top_item_ {
  int32_t is_global; // 是否全局推荐
  int32_t expired_time; // 推荐过期时间
  id_set_t srp; // 部分推荐，所属srp
  id_set_t circle; // 部分推荐，所属圈子
};
typedef struct top_item_ top_item_t;

class Candidate: public CF_framework_interface {
  public:
    Candidate();
    virtual ~Candidate();

  public:
    virtual var_4 module_type();
    virtual var_4 init_module(var_vd* config_info);

    virtual var_4 update_user(var_1* user_info);
    virtual var_4 update_item(var_1* item_info);
    virtual var_4 update_click(var_1* click_info);

    virtual var_4 query_user(var_u8 user_id, var_1* result_buf, var_4 result_max, var_4& result_len);	
    virtual var_4 query_recommend(var_u8 user_id, var_4 flag, var_1* result_buf, var_4 result_max, var_4& result_len);
    virtual var_4 query_history(var_u8 user_id, var_1* result_buf, var_4 result_max, var_4& result_len);

    virtual var_4 is_persistent_library();
    virtual var_4 persistent_library();

    virtual var_4 is_update_train();
    virtual var_4 update_pushData(var_u8 user_id, var_4 push_num, var_u8* push_data);

  protected:
    var_4 load();
    var_4 invalid_old_items();
    var_4 cold_boot(var_4 user_index, var_4 item_num, item_info_t* item_list, var_f4* recommend_power, var_4* item_times, var_4 flag);

  private:
    void parse_slip_window(char* buffer, int buflen);
    int serialize_slip_window(char* buffer, int maxsize);
    void insert_slip_window(int32_t publish_time, int32_t term_index);
    var_4 query_recommend(var_u8 user_id, var_4 flag, int32_t start_time, int32_t end_time, var_1* result_buf, var_4 result_max, var_4& result_len);

  private:
    static UC_MD5							m_md5;
    var_bl									m_is_init;
    // 用户信息的正排
    vector<user_info_t>						m_user_info;
    // 用户信息的倒排
    unmap<var_u8, var_4>					m_slip_hash;
    unmap<var_u8, top_item_t>				m_item_top;
    unmap<var_u8, item_click_t>			m_item_hash;	
    unmap<var_u8, std::pair<var_4,var_u4> >	m_user_indexer;

    // 文档ID列表，4W容量
    var_4						m_end;
    var_4						m_last_index;
    // 滑窗最后更新时间
    var_u4						m_last_time;

    item_info_t*				m_slip_items;

    var_4 						max_user_num;		// 用户最大数量
    var_4						max_circle_num;		// 一个用户最多属于几个圈子
    var_4						max_read_num;		// 用户已经阅读的item保留条数
    var_4						max_recommend_num;	// 已经推荐给用户的新闻保留条数
    var_4						max_dislike_num;	// 用户不喜欢的新闻保留条数
    var_4						slip_item_num;		// 每天的新闻数(滑窗大小)
    var_4						choose_minutes_scope;	// 选择过去n分钟的Items作推荐初选集

    var_4						item_num_limit;

    var_1						m_sto_path[256];

    int32_t base_time_; // 滑窗基准时间
    // 保留7天的文档集合，滑动窗口，小时淘汰一次
    slip_window_t slip_window_[SLIP_WINDOW_SIZE+1];

    // 日志
    UC_LogManager*              m_log_manager;
    CP_MUTEXLOCK_RW				m_item_lock;
    CP_MUTEXLOCK_RW				m_user_lock;
    // 待推荐给用户的新闻列表，大小SLIP_ITEM_NUM * sizeof(item_info_t)
    UC_Allocator_Recycle*		m_item_allocator;
    UC_Allocator_Recycle*		m_large_allocator;
    UC_Persistent_Storage*		m_data_storage;
};

// 返回时间差，单位毫秒
static uint64_t time_diff_ms(const struct timeval& begin, const struct timeval& end)
{
  return ((end.tv_sec*1000000UL+end.tv_usec) - (begin.tv_sec*1000000UL+begin.tv_usec))/1000;
}

#endif // #define CF_CANDIDATE_H
*/
