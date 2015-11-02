#ifndef __CANDIDATE_H__
#define __CANDIDATE_H__

#include <cstdio>
#include <cstring>

#include <iostream>
#include <map>
#include <fstream>
#include <vector>
#include <boost/unordered_map.hpp>
using namespace std;

#include "UC_MD5.h"
#include "UH_Define.h"
#include "UC_LogManager.h"
#include "UC_Persistent_Storage.h"
#include "UC_Allocator_Recycle.h"
#include "CF_framework_interface.h"
#include "CF_framework_config.h"

#define LOG_ERROR(x,y)		printf("FILE: %s LINE: %d\nERROR: [%s] [%s] \n", __FILE__, __LINE__, x, y)

#define SLIP_WINDOW_SIZE	(1440)
#define SLIP_ITEM_NUM		(1000000)
#define CIRCLE_AND_SRP_NUM	(200)
#define MAX_BUFFER_SIZE		(1<<24)

#define HEAP_SIZE			(1000)
#define SECOND_PER_DAY		(86400)
#define ITEM_KEEP_TIME		(SECOND_PER_DAY*3)

#define MAX_FUN(a,b)	((a)>(b)?(a):(b))
#define MIN_FUN(a,b) 	((a)<(b)?(a):(b))

//#define unmap			boost::unordered_map
#define unmap			map

const var_4 UPDATE_USER 	= 1001;
const var_4 UPDATE_ITEM 	= 1002;
const var_4 UPDATE_CLICK	= 1003;
const var_4 RESPONSE_RECOMMEND	= 1004;
const var_4 CATEGORY_NUM = 13;
const var_4 default_category_id = 13;
const var_1 global_category_name[][16] = {"互联网新闻","国内新闻","国际新闻","社会新闻","娱乐新闻","军事新闻","体育新闻","汽车新闻","科技新闻","财经新闻","教育新闻","房产新闻","女性新闻"};

typedef struct st_user_info
{
	var_u8					user_id;
	unmap<var_u8, var_4>	m_circle_and_srp;	// 圈子或者SRP词     
	unmap<var_u8, var_4>	m_has_read;			// 已阅新闻列表
	unmap<var_u8, var_4>	m_has_recommend;	// 已推荐新闻列表
	unmap<var_u8, var_4>	m_dislike;			// 不喜欢新闻列表
	vector<var_u8>		v_has_read;		
	vector<var_u8>		v_has_recommend;
	vector<var_u8>		v_circle_and_srp;		
	vector<var_u8>		v_dislike;		
	
	st_user_info()
	{
		m_circle_and_srp.clear();
		v_circle_and_srp.clear();
		m_has_read.clear();
		v_has_read.clear();
		m_has_recommend.clear();
		v_has_recommend.clear();
		m_dislike.clear();
		v_dislike.clear();
	}
}USER_INFO;

typedef struct st_item_info
{
	// 定长写死
	var_4   picture_num;
	var_4   category_id;
	var_4	circle_and_srp_num;
	var_u4	publish_time;
	var_u8	item_id;
	var_u8	circle_and_srp[CIRCLE_AND_SRP_NUM];
}ITEM_INFO;

typedef struct st_item_click
{
	var_u4  click_count;
	var_u4	click_time;
	var_f4 	primary_power;
}ITEM_CLICK;

typedef struct st_slip_window
{
	var_u4 last_update_index;
	var_u4 last_update_time;
}SLIP_WINDOW;

typedef struct st_item_top
{
	var_bl global;
	vector<var_u8> srps;
	vector<var_u8> circles;
	st_item_top()
	{
		global = false;
		srps.clear();
		circles.clear();
	}
}ITEM_TOP;

class Candidate:public CF_framework_interface
{
public:
	Candidate();
	~Candidate();
	
	var_4 load();
	var_4 invalid_old_items();
	
	var_4 module_type();
	var_4 init_module(var_vd* config_info);
	
	var_4 update_user(var_1* user_info);
	var_4 update_item(var_1* item_info);
	var_4 update_click(var_1* click_info);

	var_4 query_user(var_u8 user_id, var_1* result_buf, var_4 result_max, var_4& result_len);	
	var_4 query_recommend(var_u8 user_id, var_4 flag, var_1* result_buf, var_4 result_max, 
			              var_4& result_len);
	
	var_4 query_history(var_u8 user_id, var_1* result_buf, var_4 result_max, var_4& result_len);

	var_4 is_persistent_library();
	var_4 persistent_library();

	var_4 is_update_train();
	var_4 update_pushData(var_u8 user_id, var_4 push_num, var_u8* push_data);

	var_4 cold_boot(var_4 user_index, var_4 item_num, ITEM_INFO* item_list, var_f4* recommend_power, 
			        var_4* item_times, var_4 flag);
	
private:
	static UC_MD5							m_md5;
	var_bl									m_is_init;
	// 用户信息的正排
	vector<USER_INFO>						m_user_info;
	// 用户信息的倒排
	unmap<var_u8, var_4>					m_slip_hash;
	unmap<var_u8, st_item_top>				m_item_top;
	unmap<var_u8, st_item_click>			m_item_hash;	
	unmap<var_u8, std::pair<var_4,var_u4> >	m_user_indexer;

	// 文档ID列表，4W容量
	var_4						m_end;
	var_4						m_last_index;
	// 滑窗最后更新时间
	var_u4						m_last_time;
	
	st_item_info*				m_slip_items;
	
	var_4 						max_user_num;		// 用户最大数量
	var_4						max_circle_num;		// 一个用户最多属于几个圈子
	var_4						max_read_num;		// 用户已经阅读的item保留条数
	var_4						max_recommend_num;	// 已经推荐给用户的新闻保留条数
	var_4						max_dislike_num;	// 用户不喜欢的新闻保留条数
	var_4						slip_item_num;		// 每天的新闻数(滑窗大小)
	var_4						choose_minutes_scope;	// 选择过去n分钟的Items作推荐初选集
	
	var_4						item_num_limit;
	
	var_1						m_sto_path[256];
	// 日志
	UC_LogManager*              m_log_manager;
	// 保留一天的文档集合，滑动窗口，每分钟淘汰一次
	SLIP_WINDOW					m_window_control[SLIP_WINDOW_SIZE];
	CP_MUTEXLOCK_RW				m_item_lock;
	CP_MUTEXLOCK_RW				m_user_lock;
	// 待推荐给用户的新闻列表，大小SLIP_ITEM_NUM * sizeof(ITEM_INFO)
	UC_Allocator_Recycle*		m_item_allocator;
	UC_Allocator_Recycle*		m_large_allocator;
	UC_Persistent_Storage*		m_data_storage;
};

static var_u8 GetTimeDiff(struct timeval begin, struct timeval end)
{
	struct timeval diff;
	if (end.tv_usec >= begin.tv_usec)
	{   
		diff.tv_sec = end.tv_sec - begin.tv_sec;
		diff.tv_usec = end.tv_usec - begin.tv_usec;
	}   
	else
	{   
		diff.tv_sec = end.tv_sec - begin.tv_sec - 1;
		diff.tv_usec = 1000000 -begin.tv_usec + end.tv_usec;
	}   
	return (diff.tv_sec * 1000000 + diff.tv_usec);
}

#endif



