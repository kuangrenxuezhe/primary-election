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

#define unmap			map

const var_4 UPDATE_USER 	= 1001;
const var_4 UPDATE_ITEM 	= 1002;
const var_4 UPDATE_CLICK	= 1003;
const var_4 RESPONSE_RECOMMEND	= 1004;
const var_4 CATEGORY_NUM = 13;
const var_4 default_category_id = 13;

using namespace std;

struct user_info_ {
  var_u8					user_id;
  unmap<var_u8, var_4>	m_circle_and_srp;	// 圈子或者SRP词     
  unmap<var_u8, var_4>	m_has_read;			// 已阅新闻列表
  unmap<var_u8, var_4>	m_has_recommend;	// 已推荐新闻列表
  unmap<var_u8, var_4>	m_dislike;			// 不喜欢新闻列表
  vector<var_u8>		v_has_read;		
  vector<var_u8>		v_has_recommend;
  vector<var_u8>		v_circle_and_srp;		
  vector<var_u8>		v_dislike;		
};
typedef struct user_info_ user_info_t;

struct item_info_ {
  var_4   picture_num;
  var_4   category_id;
  var_4	circle_and_srp_num;
  var_u4	publish_time;
  var_u8	item_id;
  var_u8	circle_and_srp[CIRCLE_AND_SRP_NUM];
};
typedef struct item_info_ item_info_t;

struct item_click_ {
  var_u4  click_count;
  var_u4	click_time;
  var_f4 	primary_power;
};
typedef struct item_click_ item_click_t;

struct item_index_ {
  int32_t item_index;
  int32_t publish_time;
};
typedef struct item_index_ item_index_t;

struct slip_window_ {
  std::list<item_index_t> item_list;
};
typedef struct slip_window_ slip_window_t;

struct top_item_ {
  var_bl global;
  vector<var_u8> srps;
  vector<var_u8> circles;
  top_item_() {
    global = false;
  }
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

