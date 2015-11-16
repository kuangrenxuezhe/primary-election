#include "core/candidate_db.h"

#include "core/core_type.h"
#include "glog/logging.h"
#include "proto/log_record.pb.h"

namespace rsys {
  namespace news {
    // 默认推荐时间范围
    static const int32_t kDefaultRecommendGap = 1*24*60*60;

    const char kCategoryName[][16] = {
      "\xbb\xa5\xc1\xaa\xcd\xf8\xd0\xc2\xce\xc5", // 互联网新闻 GBK编码
      "\xb9\xfa\xc4\xda\xd0\xc2\xce\xc5", // 国内新闻 GBK编码
      "\xb9\xfa\xbc\xca\xd0\xc2\xce\xc5", // 国际新闻 GBK编码
      "\xc9\xe7\xbb\xe1\xd0\xc2\xce\xc5", // 社会新闻 GBK编码
      "\xd3\xe9\xc0\xd6\xd0\xc2\xce\xc5", // 娱乐新闻 GBK编码
      "\xbe\xfc\xca\xc2\xd0\xc2\xce\xc5", // 军事新闻 GBK编码
      "\xcc\xe5\xd3\xfd\xd0\xc2\xce\xc5", // 体育新闻 GBK编码
      "\xc6\xfb\xb3\xb5\xd0\xc2\xce\xc5", // 汽车新闻 GBK编码
      "\xbf\xc6\xbc\xbc\xd0\xc2\xce\xc5", // 科技新闻 GBK编码
      "\xb2\xc6\xbe\xad\xd0\xc2\xce\xc5", // 财经新闻 GBK编码
      "\xbd\xcc\xd3\xfd\xd0\xc2\xce\xc5", // 教育新闻 GBK编码
      "\xb7\xbf\xb2\xfa\xd0\xc2\xce\xc5", // 房产新闻 GBK编码
      "\xc5\xae\xd0\xd4\xd0\xc2\xce\xc5", // 女性新闻 GBK编码
    };

    Status CandidateDB::openDB(const Options& opts, CandidateDB** dbptr)
    {
      *dbptr = new CandidateDB(opts);
      return (*dbptr)->reload();
    }

    CandidateDB::CandidateDB(const Options& opts)
    {
      options_ = opts;
      pthread_mutex_init(&mutex_, NULL);
    }

    CandidateDB::~CandidateDB() 
    {
      pthread_mutex_destroy(&mutex_);
    }

    //与flush采用不同的周期,只保留days天
    Status CandidateDB::rollOverWALFile(int32_t expired_days)
    {
      char today[300], yesterday[300];

      pthread_mutex_lock(&mutex_);
      for (int i = expired_days; i > 0; --i) {
        sprintf(today, "%s/wal.day-%d", options_.path.c_str(), i);
        if (access(today, F_OK))
          continue;
        if (i == expired_days) {
          if (remove(today)) {
            LOG(FATAL) << "remove file failed: " << strerror(errno)
              << ", file: " << today;
            pthread_mutex_unlock(&mutex_);
            return Status::IOError(strerror(errno));
          }
        } else {
          sprintf(yesterday, "%s/wal.day-%d", options_.path.c_str(), i);
          if (rename(today, yesterday)) {
            LOG(FATAL) << "rename file failed: " << strerror(errno)
              << ", oldfile: " << today << ", newfile: " << yesterday;
            pthread_mutex_unlock(&mutex_);
            return Status::IOError(strerror(errno));
          }
        }
      }
      writer_->close();

      sprintf(today, "%s/wal.day-0", options_.path.c_str());
      sprintf(yesterday, "%s/wal.day-1", options_.path.c_str());
      if (rename(today, yesterday)) {
        LOG(FATAL) << "rename file failed: " << strerror(errno)
          << ", oldfile: " << today << ", newfile: " << yesterday;
        pthread_mutex_unlock(&mutex_);
        return Status::IOError(strerror(errno));
      }
      writer_ = new WALWriter(today);
      pthread_mutex_unlock(&mutex_);

      return Status::OK();
    }

    // 可异步方式，线程安全
    Status CandidateDB::flush()
    {
      Status status = user_table_->eliminate(options_.user_hold_time);
      if (!status.ok()) {
        LOG(WARNING) << "eliminate user failed: " << status.toString();
      }
      status = user_table_->flushTable();
      if (!status.ok()) {
        LOG(ERROR) << "flush user table failed: " << status.toString();
        return status;
      }

      status = item_table_->eliminate(options_.item_hold_time);
      if (!status.ok()) {
        LOG(WARNING) << "eliminate item failed: " << status.toString();
      }
      status = item_table_->flushTable();
      if (!status.ok()) {
        LOG(ERROR) << "flush item table failed: " << status.toString();
        return status;
      }

      return Status::OK();
    }

    Status CandidateDB::reload()
    {
      Status status = user_table_->loadTable();
      if (!status.ok()) {
        LOG(ERROR) << "load user table failed: " << status.toString();
        return status;
      }

      status = item_table_->loadTable();
      if (!status.ok()) {
        LOG(ERROR) << "load item table failed: " << status.toString();
        return status;
      }

      return Status::OK();
    }

    bool CandidateDB::findUser(uint64_t user_id)
    {
      return user_table_->findUser(user_id);
    }

    Status CandidateDB::addItem(const ItemInfo& item)
    {
      LogRecord log_record;
      std::string serialize_str;

      // write ahead-log
      log_record.mutable_record()->PackFrom(item);
      serialize_str = log_record.SerializeAsString();

      Status status = writer_->append(serialize_str);
      if (!status.ok()) {
        LOG(WARNING) << "write item log failed: " << status.toString() 
          << std::hex << ", item id=" << item.item_id();
      }

      // convert ItemInfo to item_info_t
      item_info_t* item_info = new item_info_t;
      item_info->item_id = item.item_id();
      item_info->publish_time = item.publish_time();
      item_info->primary_power = item.power();
      item_info->item_type = item.item_type();
      
      // 匹配分类
      for (int i = 0; i < item.category_size(); ++i) {
        for (size_t j = 0; j < sizeof(kCategoryName)/sizeof(char[16]); ++j) {
          if (strcasecmp(item.category(i).c_str(), kCategoryName[j]))
            continue;
          item_info->category_id = j;
          break;
        }
      }
      item_info->region_id = 0; //TODO: 下一个版本补入

      for (int i = 0; i < item.srp_size(); ++i) {
        const Srp& srp = item.srp(i);
        item_info->circle_and_srp.insert(srp.srp_id());
      }

      return item_table_->addItem(item_info);
    }

    Status CandidateDB::queryHistory(uint64_t user_id, IdSet& history)
    {
      id_set_t id_set;
      Status status = user_table_->queryHistory(user_id, id_set);
      if (status.ok()) {
        id_set_t::iterator iter = id_set.begin();
        for (; iter != id_set.end(); ++iter) {
          history.add_id(*iter);
        }
      }
      return status;
    }

    Status CandidateDB::queryCandidateSet(const Query& query, CandidateSet& cset)
    {
      query_t cand_query;
      candidate_set_t cand_set;   

      if (query.end_time() >= query.start_time()) {
        cand_query.end_time = time(NULL);
        cand_query.start_time = cand_query.end_time - kDefaultRecommendGap;
      } else {
        cand_query.start_time = query.start_time();
        cand_query.end_time = query.end_time();
      }

      Status status = item_table_->queryCandidateSet(cand_query, cand_set);
      if (!status.ok()) {
        LOG(ERROR) << "query candidate set failed: " << status.toString()
          << ", begin time=" << cand_query.start_time << ", end time=" << cand_query.end_time;
        return status;
      }

      status = user_table_->filterCandidateSet(query.user_id(), cand_set);
      if (!status.ok()) {
        LOG(ERROR) << "filter candidate set failed: " << status.toString()
          << " user id=" << query.user_id();
        return status;
      }

      id_set_t id_set;
      status = user_table_->queryHistory(query.user_id(), id_set);
      if (!status.ok()) {
        LOG(ERROR) << "query history failed: " << status.toString()
          << " user id=" << query.user_id();
        return status;
      }

      for (candidate_set_t::iterator iter = cand_set.begin(); 
          iter != cand_set.end(); ++iter) {
        CandidateSet_Candidate* cand = cset.add_candidate();

        cand->set_item_id(iter->item_id);
        cand->set_power(iter->power);
        cand->set_publish_time(iter->publish_time);
        cand->set_category_id(iter->category_id);
        cand->set_picture_num(iter->picture_num);
      }

      for (id_set_t::iterator iter = id_set.begin(); iter != id_set.end(); ++iter) {
        cset.add_history(*iter);
      }

      return Status::OK();
    }

    Status CandidateDB::updateAction(const UserAction& action)
    {
      LogRecord log_record;
      std::string serialize_str;

      log_record.mutable_record()->PackFrom(action);
      serialize_str = log_record.SerializeAsString();

      Status status = writer_->append(serialize_str);
      if (!status.ok()) {
        LOG(WARNING) << "write action log failed: " << status.toString()
          << std::hex << ", user id=" << action.user_id() << ", item id=" << action.item_id();
      }
      action_t item_action;

      item_action.item_id = action.item_id();
      item_action.action = action.action();
      item_action.action_time = action.click_time();
      status = item_table_->updateAction(item_action);
      if (!status.ok()) {
        LOG(WARNING) << "add action failed: " << status.toString()
          << std::hex << ", user id=" << action.user_id() << ", item id=" << action.item_id();
        return status;
      }
      action_t user_action;

      user_action.item_id = action.item_id();
      user_action.action = action.action();
      user_action.action_time = action.click_time();
      status = user_table_->updateReaded(action.user_id(), user_action);
      if (!status.ok()) {
        LOG(WARNING) << "add action failed: " << status.toString()
          << std::hex << ", user id=" << action.user_id() << ", item id=" << action.item_id();
      }

      return status;
    }

    Status CandidateDB::updateUser(const UserSubscribe& subscribe)
    {
      LogRecord log_record;
      std::string serialize_str;

      log_record.mutable_record()->PackFrom(subscribe);
      serialize_str = log_record.SerializeAsString();

      Status status = writer_->append(serialize_str);
      if (!status.ok()) {
        LOG(WARNING) << "write subscribe log failed: " << status.toString()
          << std::hex << ", user id=" << subscribe.user_id();
      }
      user_info_t* user_info = new user_info_t;

      for (int i = 0; i < subscribe.srp_id_size(); ++i) {
        user_info->srp.insert(subscribe.srp_id(i));
      }

      for (int i = 0; i < subscribe.circle_id_size(); ++i) {
        user_info->circle.insert(subscribe.circle_id(i));
      }
      return user_table_->updateUser(subscribe.user_id(), user_info);
    }

    Status CandidateDB::updateCandidateSet(uint64_t user_id, const IdSet& recommend_set)
    {
      LogRecord log_record;
      std::string serialize_str;

      log_record.mutable_record()->PackFrom(recommend_set);
      serialize_str = log_record.SerializeAsString();

      Status status = writer_->append(serialize_str);
      if (!status.ok()) {
        LOG(WARNING) << "write recommend set log failed: " << status.toString()
          << std::hex << ", user id=" << user_id;
      }
      id_set_t id_set;

      for (int i = 0; i < recommend_set.id_size(); ++i) {
        id_set.insert(recommend_set.id(i));
      }
      return user_table_->updateCandidateSet(user_id, id_set);
    }
  } // namespace news
} // namespace rsys


/*
#define LOG_ERROR(x,y)		printf("FILE: %s LINE: %d\nERROR: [%s] [%s] \n", __FILE__, __LINE__, x, y)

var_4 query_cnt = 0;
UC_MD5 Candidate::m_md5;

Candidate::Candidate()
: m_is_init(0)
, m_last_index(0)
, m_last_time(0)
, m_user_info(NULL)
, m_slip_items(NULL)
, m_log_manager(NULL)
, m_item_allocator(NULL)
, m_large_allocator(NULL)
, m_data_storage(NULL)
{
memset(slip_window_, 0, sizeof(slip_window_t)*(SLIP_WINDOW_SIZE+1));
}

inline var_1* block_alloc(UC_Allocator_Recycle* _mem_pool)
{
var_1* temp = _mem_pool->AllocMem();
while (NULL == temp)
{
cp_sleep(10);
temp = _mem_pool->AllocMem();
}
return temp;
}

var_4 Candidate::init_module(var_vd *cfg_info)
{
MODULE_CONFIG *cfg = (MODULE_CONFIG*)cfg_info;

m_is_init = true;

assert(NULL != cfg_info);
max_user_num = cfg->pf_max_user_num;
max_circle_num = cfg->pf_max_circle_num;
max_read_num = cfg->pf_max_read_num;
max_recommend_num = cfg->pf_max_recommend_num;
max_dislike_num = cfg->pf_max_dislike_num;
choose_minutes_scope = cfg->pf_choose_minutes_scope;
item_num_limit = cfg->pf_item_num_limit;
strcpy(m_sto_path, cfg->pf_store_path);

assert(NULL == m_slip_items);
m_slip_items = new item_info_t[SLIP_ITEM_NUM];
if (NULL == m_slip_items)
return -10;

assert(NULL == m_item_allocator);
m_item_allocator = new UC_Allocator_Recycle;
if (NULL == m_item_allocator)
return -11;

if (m_item_allocator->initMem(SLIP_ITEM_NUM * sizeof(item_info_t), 400, 20))
  return -12;

  assert(NULL == m_large_allocator);
  m_large_allocator = new UC_Allocator_Recycle;
if (NULL == m_large_allocator)
  return -12;

if (m_large_allocator->initMem(MAX_BUFFER_SIZE, 100, 10))
  return -12;

  assert(NULL == m_log_manager);
  m_log_manager = new UC_LogManager;
  if (m_log_manager->init((var_1*)"log", (var_1*)"log", 30, 1))
  return -6;

  m_data_storage = new UC_Persistent_Storage;
if (NULL == m_data_storage)
  return -13;

  if (m_data_storage->init(m_sto_path, (var_1*)"data_sto"))
  return -14;

if (load())
  return -15;

  m_is_init = false;
  return 0;
  }

var_4 Candidate::update_user(var_1* user_buf)
{
  var_4 user_index;

  var_4 process_len = 4;
  var_u8 user_id = *(var_u8*)(user_buf + process_len);
  process_len = process_len + 8;

  m_user_lock.lock_w();
  if (m_user_indexer.find(user_id) == m_user_indexer.end())
  {
    user_info_t user_info;
    user_info.user_id = user_id;
    m_user_info.push_back(user_info);
    user_index = m_user_info.size() - 1;
    m_user_indexer[user_id] = std::make_pair(user_index, time(NULL));
  }
  else
  {
    user_index = m_user_indexer[user_id].first;
    m_user_indexer[user_id].second = time(NULL);
  }
  m_user_info[user_index].user_id = user_id;
  // 清除用户的圈子和SRP信息
  m_user_info[user_index].m_circle_and_srp.clear();
  m_user_info[user_index].v_circle_and_srp.clear();

  var_4 circle_and_srp_num = *(var_4*)(user_buf + process_len);
  process_len = process_len + 4;
  // SRP	
  for (int k = 0; k < circle_and_srp_num; k++)
  {
    var_4 circle_and_srp_len = *(var_4*)(user_buf + process_len);
    var_u4 md5_id = m_md5.MD5Bits32((var_u1*)(user_buf + process_len + 4), circle_and_srp_len); 
    process_len = process_len + 4 + circle_and_srp_len;
    // 为了区分SRP ID和圈子ID, SRP词压缩为U32位,然后左移32位
    var_u8 circle_and_srp_id = ((var_u8)md5_id)<<32UL;  
    m_user_info[user_index].v_circle_and_srp.push_back(circle_and_srp_id);
    m_user_info[user_index].m_circle_and_srp[circle_and_srp_id] = k;
  }

  circle_and_srp_num = *(var_4*)(user_buf + process_len);
  process_len = process_len + 4;
  // 圈子
  for (int k = 0; k < circle_and_srp_num; k++)
  {
    var_4 circle_and_srp_len = *(var_4*)(user_buf + process_len);
    var_u8 circle_and_srp_id = m_md5.MD5Bits64((var_u1*)(user_buf + process_len + 4), circle_and_srp_len); 
    process_len = process_len + 4 + circle_and_srp_len;

    m_user_info[user_index].v_circle_and_srp.push_back(circle_and_srp_id);
    m_user_info[user_index].m_circle_and_srp[circle_and_srp_id] = k;
  }

  m_user_lock.unlock();

  if (!m_is_init)
  {// 如果不是初始化，写增量
    var_1* buffer = (var_1*)block_alloc(m_large_allocator);
    memcpy(buffer, "INCR", 4);
    memcpy(buffer + 4, &UPDATE_USER, 4);
    memcpy(buffer + 8, (var_1*)user_buf, process_len);

    if (m_data_storage->save(buffer, 8 + process_len))
    {
      LOG_ERROR("Candidate:update_user", "m_data_storage->save");
    }
    m_large_allocator->FreeMem(buffer);
  }
  printf("process update user, user id is %lu\n", user_id);
  return 0;
}


var_4 Candidate::update_item(var_1* item_buf)
{
  item_info_t item_info;
  item_click_t item_click;

  item_info.item_id = *(var_u8*)(item_buf + 4);
  item_info.publish_time = *(var_u4*)(item_buf + 12);
  item_info.category_id = default_category_id;
  var_u4 update_time = *(var_u4*)(item_buf + 16);

  item_click.primary_power = *(var_f4*)(item_buf + 20);

  var_4 category_num = *(var_4*)(item_buf + 24);
  var_4 process_len = 28;
  for (var_4 i = 0; i < category_num; i++)
  {
    var_4 category_len = *(var_4*)(item_buf + process_len);
    for (var_4 j = 0; j < CATEGORY_NUM; j++)
    {
      if (!strncmp((var_1*)(item_buf + process_len + 4), global_category_name[j], category_len))
        item_info.category_id = j;
    }
    process_len = process_len + 4 + category_len;
  }
  var_4 word_num = *(var_4*)(item_buf + process_len);
  process_len = process_len + 4;
  for (var_4 i = 0; i < word_num; i++)
  {
    var_4 word_len = *(var_4*)(item_buf + process_len);
    //var_4 word_power = *(var_4*)(item_buf + process_len + word_len + 4);
    process_len = process_len + 4 + word_len + 4;
  }

  m_item_lock.lock_w();

  // SRP
  var_4 srp_num = *(var_4*)(item_buf + process_len);
  process_len  = process_len + 4;

  item_info.circle_and_srp_num = (srp_num < CIRCLE_AND_SRP_NUM ? srp_num : CIRCLE_AND_SRP_NUM);

  int i, k;
  for (k = 0; k < item_info.circle_and_srp_num; k++)
  {
    var_4 srp_len = *(var_4*)(item_buf + process_len);
    var_u4 md5_id = m_md5.MD5Bits32((var_u1*)(item_buf + process_len + 4), srp_len);
    var_u8 srp_id = ((var_u8)md5_id)<<32UL;

    item_info.circle_and_srp[k] = srp_id;
    process_len = process_len + 4 + srp_len;
  }
  // 圈子
  var_4 circle_num = *(var_4*)(item_buf + process_len);
  process_len = process_len + 4;
  if (item_info.circle_and_srp_num + circle_num < CIRCLE_AND_SRP_NUM)
  {
    item_info.circle_and_srp_num += circle_num;
  }
  else
  {
    item_info.circle_and_srp_num = CIRCLE_AND_SRP_NUM;
  }
  for (i = k; i < item_info.circle_and_srp_num; i++)
  {
    var_4 circle_len = *(var_4*)(item_buf + process_len);
    var_u8 circle_id = m_md5.MD5Bits64((var_u1*)(item_buf + process_len + 4), circle_len);

    item_info.circle_and_srp[i] = circle_id;
    process_len = process_len + 4 + circle_len;
  }
  // picture number
  item_info.picture_num = *(var_4*)(item_buf + process_len);
  process_len = process_len + 4;
  // 置顶标记 0, 1, 2
  var_4 top_flag = *(var_4*)(item_buf + process_len);
  process_len = process_len + 4;
  top_item_t top_info; 
  if (top_flag == 1)
  {
    top_info.global = true;
    m_item_top[item_info.item_id] = top_info;	
  }
  else if (top_flag == 2)
  {
    var_4 top_srp_num = *(var_4*)(item_buf + process_len);
    process_len = process_len + 4;
    for (i = 0; i < top_srp_num; i++)
    {
      var_4 top_srp_len = *(var_4*)(item_buf + process_len);
      var_u4 md5_id = m_md5.MD5Bits32((var_u1*)(item_buf + process_len + 4), top_srp_len);
      var_u8 top_srp_id = ((var_u8)md5_id)<<32UL;

      top_info.srps.push_back(top_srp_id);
      process_len = process_len + 4 + top_srp_len;
    }
    var_4 top_circle_num = *(var_4*)(item_buf + process_len);
    process_len = process_len + 4;
    for (i = 0; i < top_circle_num; i++)
    {
      var_4 top_circle_len = *(var_4*)(item_buf + process_len);
      var_u8 top_circle_id = m_md5.MD5Bits64((var_u1*)(item_buf + process_len + 4), top_circle_len);

      top_info.circles.push_back(top_circle_id);
      process_len = process_len + 4 + top_circle_len;
    }
    m_item_top[item_info.item_id] = top_info;	
  }
  // 如果新闻的时间太旧，丢弃
  // 超过2天
  if (top_flag == 0 && item_info.publish_time + SECOND_PER_DAY*2 < update_time)
  {
    m_log_manager->uc_log_time(2, "item too old, publish time and update time %llu, %d, %d \n", item_info.item_id, item_info.publish_time, update_time);
    m_item_lock.unlock();
    return 0;
  }

  if (m_item_hash.find(item_info.item_id) == m_item_hash.end())
  {//头条
    if (item_click.primary_power == 100000 || item_click.primary_power > 100001)
    {
      m_log_manager->uc_log_time(2, "item primary_power illegal, id %llu, power %lf\n", item_info.item_id, item_click.primary_power);
      m_item_lock.unlock();
      return 0;
    }
    item_click.click_count = 0;
    item_click.click_time = item_info.publish_time;
    m_item_hash[item_info.item_id] = item_click;
  }
  else
  {
    item_click_t old_item = m_item_hash[item_info.item_id];
    if ((item_click.primary_power == 100000 || item_click.primary_power > 100001) &&
        old_item.primary_power < 0.1)
    {
      m_log_manager->uc_log_time(2, "item primary_power illegal, id %llu, power %lf\n", item_info.item_id, item_click.primary_power);
      m_item_lock.unlock();
      return 0;
    }
    item_click.click_count = old_item.click_count;
    item_click.click_time = old_item.click_time;
    m_item_hash[item_info.item_id] = item_click;
  }

  // 赋值
  if (m_slip_hash.find(item_info.item_id) == m_slip_hash.end())
  {
    m_last_index = (m_last_index + 1) % SLIP_ITEM_NUM;

    if (m_slip_hash.find(m_slip_items[m_last_index].item_id) != m_slip_hash.end())
    {
      m_slip_hash.erase(m_slip_items[m_last_index].item_id);
    }

    m_slip_items[m_last_index] = item_info;
    m_slip_hash[item_info.item_id] = m_last_index;

    insert_slip_window(item_info.publish_time, m_last_index);
  }
  else
  {
    m_slip_items[m_slip_hash[item_info.item_id]] = item_info;
  }
  //
  m_item_lock.unlock();

  printf("process update item, item id is %lu\n", item_info.item_id);

  if (!m_is_init)
  {// 如果不是初始化，写增量
    var_1* buffer = (var_1*)block_alloc(m_large_allocator);
    memcpy(buffer, "INCR", 4);
    memcpy(buffer + 4, &UPDATE_ITEM, 4);
    memcpy(buffer + 8, (var_1*)item_buf, process_len);
    if (m_data_storage->save(buffer, 8 + process_len))
    {
      LOG_ERROR("Candidate:::update_item", "m_data_storage->save");
    }
    m_large_allocator->FreeMem(buffer);
  }
  return 0;
}

void Candidate::insert_slip_window(int32_t publish_time, int32_t term_index)
{
  item_index_t item;
  int32_t index = (publish_time - base_time_)/SECOND_PER_HOUR;

  item.item_index = term_index;
  item.publish_time = publish_time;
  std::list<item_index_t>::iterator iter = slip_window_[index].item_list.begin();
  for (; iter != slip_window_[index].item_list.end(); ++iter) {
    if (publish_time < iter->publish_time)
      continue;
    slip_window_[index].item_list.insert(iter, item);
    break;
  }
}

// 入参：<user_id, item_id>
// 出参：
var_4 Candidate::update_click(var_1* click_buf)
{
  var_4 user_index, item_index, read_index;
  var_4 click_time = *(var_4*)(click_buf + 4);
  var_u8 user_id = *(var_u8*)(click_buf + 8);
  var_u8 item_id = *(var_u8*)(click_buf + 16);
  //action
  var_4 action = *(var_4*)(click_buf + 28);
  if (action != 1 && action != 6)
    return 0;

  m_user_lock.lock_w();
  if (m_user_indexer.find(user_id) == m_user_indexer.end())
  {// 用户不存在，新增一个
    user_info_t user_info;
    user_info.user_id = user_id;
    m_user_info.push_back(user_info);
    user_index = m_user_info.size() - 1;
    m_user_indexer[user_id] = std::make_pair(user_index, time(NULL));
  }
  else
  {
    user_index = m_user_indexer[user_id].first;
    if (m_user_indexer[user_id].second < (var_u4)click_time)
      m_user_indexer[user_id].second = (var_u4)click_time;
  }
  if (action == 6)
  {
    if (m_user_info[user_index].m_dislike.find(item_id) == m_user_info[user_index].m_dislike.end())
    {
      if (m_slip_hash.find(item_id) != m_slip_hash.end())
      {
        item_index = m_slip_hash[item_id];
        for (var_4 i = 0; i < m_slip_items[item_index].circle_and_srp_num; i++)
        {
          if (m_slip_items[item_index].circle_and_srp[i] % (1UL<<32) == 0 && i >= 1)
            continue;
          m_user_info[user_index].v_dislike.push_back( m_slip_items[item_index].circle_and_srp[i] );
          read_index = m_user_info[user_index].v_dislike.size() - 1;
          m_user_info[user_index].m_dislike[ m_slip_items[item_index].circle_and_srp[i] ] = read_index;
        }
      }
    }
  }
  if (m_user_info[user_index].m_has_read.find(item_id) != m_user_info[user_index].m_has_read.end())
  {// 已存在
    m_user_lock.unlock();
    return 0;
  }
  m_user_info[user_index].v_has_read.push_back(item_id);

  read_index = m_user_info[user_index].v_has_read.size() - 1;
  m_user_info[user_index].m_has_read[item_id] = read_index;
  m_user_lock.unlock();

  m_item_lock.lock_w();
  if (m_item_hash.find(item_id) == m_item_hash.end())
  {
    item_click_t item_click;
    item_click.click_count = 1;
    item_click.click_time = click_time;
    item_click.primary_power = 0;
    // 加入hash map
    m_item_hash[item_id] = item_click;
  }
  else
  {
    item_click_t item_click = m_item_hash[item_id];
    item_click.click_count += 1;
    // 更新hash map
    m_item_hash[item_id] = item_click;
  }
  m_item_lock.unlock();
  if (!m_is_init)
  {// 如果不是初始化，写增量
    var_1* buffer = (var_1*)block_alloc(m_large_allocator);
    memcpy(buffer, "INCR", 4);
    memcpy(buffer + 4, &UPDATE_CLICK, 4);
    memcpy(buffer + 8, (var_1*)click_buf, 32);

    if (m_data_storage->save(buffer, 40))
    {
      LOG_ERROR("Candidate:update_click", "m_data_storage->save");
    }
    m_large_allocator->FreeMem(buffer);
  }
  printf("process update click, user id is %lu, item id is %lu\n", user_id, item_id);
  return 0;
}

var_4 Candidate::cold_boot(var_4 user_index, var_4 item_num, item_info_t* item_list, 
    var_f4* recommend_power, var_4* publish_time, var_4 flag)
{
  var_f4 max_score, min_score, total_score = 0;

  for (var_4 i = 0; i < item_num; i++)
  {
    //item时间
    publish_time[i] = item_list[i].publish_time;
    if (m_item_top.find(item_list[i].item_id) != m_item_top.end())
    {
      top_item_t top_info = m_item_top[item_list[i].item_id];
      if (top_info.global)
      {
        publish_time[i] = -1;
      }
      for (var_4 k =0; publish_time[i] != -1 && k < top_info.srps.size(); k++)
      {
        if (m_user_info[user_index].m_circle_and_srp.find(top_info.srps[k])!=
            m_user_info[user_index].m_circle_and_srp.end())
        {
          publish_time[i] = -1;
        }
      }
      for (var_4 k = 0; publish_time[i] != -1 && k < top_info.circles.size(); k++)
      {
        if (m_user_info[user_index].m_circle_and_srp.find(top_info.circles[k])!=
            m_user_info[user_index].m_circle_and_srp.end())
        {
          publish_time[i] = -1;
        }
      }
    }

    // 权重
    recommend_power[i] = 0;

    if (flag) { // zyq, 20150820, for "my headline", drop not-focus news
      m_item_lock.lock_w();
      if (m_item_hash.find(item_list[i].item_id) != m_item_hash.end()) {
        item_click_t item_click = m_item_hash[item_list[i].item_id];
        if (item_click.primary_power < 100) {
          recommend_power[i] = -100000;
          m_item_lock.unlock();
          continue;
        }
      } else {
        recommend_power[i] = -100000;
        m_item_lock.unlock();
        continue;
      }
      m_item_lock.unlock();
    } else { // zyq, 20150828, for visiters not to see headlines in recommend channel
      m_item_lock.lock_w();
      if (m_item_hash.find(item_list[i].item_id) != m_item_hash.end()) {
        item_click_t item_click = m_item_hash[item_list[i].item_id];
        if (item_click.primary_power >= 100) {
          recommend_power[i] = -100000;
          m_item_lock.unlock();
          continue;
        }
      }
      m_item_lock.unlock();
    }

    //圈子和SRP
    for (int k = 0; k < item_list[i].circle_and_srp_num; k++)
    {
      if (m_user_info[user_index].m_circle_and_srp.find( item_list[i].circle_and_srp[k] )
          != m_user_info[user_index].m_circle_and_srp.end())
      {// 圈子或者SRP词命中，加权
        //cout <<"circle and srp hit: userid "<<m_user_info[user_index].user_id <<" item_id ";
        //cout <<item_list[i].item_id <<" circle and srp id " <<item_list[i].circle_and_srp[k] <<endl;
        recommend_power[i] = 10000;
        break;
      }
    }
    m_item_lock.lock_w();
    if (m_item_hash.find(item_list[i].item_id)!=m_item_hash.end())
    {
      item_click_t item_click = m_item_hash[item_list[i].item_id];
      //全局点击 点击次数平滑, 时间衰减
      //recommend_power[i] += 1000*log(item_click.click_count + 1)/((time(NULL) - item_click.click_time)/3600 + 1);
      recommend_power[i] += 10*item_click.click_count;
      //文档权重
      if (item_click.primary_power >= 100000)
      {
        recommend_power[i] += item_click.primary_power / 100000;
      }
      else
      {
        recommend_power[i] += item_click.primary_power;
      }
      if (recommend_power[i] > 100000)
      {
        recommend_power[i] = 100000;
      }
    }
    m_item_lock.unlock();

    for (int k = 0; k < item_list[i].circle_and_srp_num; k++)
    {
      if (item_list[i].circle_and_srp[k] % (1UL<<32) == 0 && k >= 1)
        continue;
      if (m_user_info[user_index].m_dislike.find( item_list[i].circle_and_srp[k] )
          != m_user_info[user_index].m_dislike.end())
      {// 命中用户不喜欢的圈子和SRP词，降权
        recommend_power[i] = -100000;
        break;
      }
    }
  }
  max_score = -100000;
  min_score =  100000;
  for (int i = 0; i < item_num; i++)
  {//归一
    if (recommend_power[i] == -100000)
      continue;
    max_score = MAX_FUN(max_score, recommend_power[i]);
    min_score = MIN_FUN(min_score, recommend_power[i]);

    total_score = total_score + recommend_power[i];
  }
  if (min_score < 0)
  {
    min_score = 0;
  }
  if (total_score < 1)
    total_score = 1;

  for (int i = 0; i < item_num; i++)
  {
    if (recommend_power[i] == -100000)
      continue;
    //recommend_power[i] = (recommend_power[i] - min_score + 0.1)/(max_score - min_score + 1);
    recommend_power[i] /= total_score;
  }
  return 0;
}

var_4 Candidate::query_user(var_u8 user_id, var_1* result_buf, var_4 result_max, var_4& result_len)
{
  result_len = 4;
  m_user_lock.lock_w();
  if (m_user_indexer.find(user_id) == m_user_indexer.end())
  {
    *(var_4*)result_buf = 1;
  }
  else
  {
    *(var_4*)result_buf = 0;
  }
  m_user_lock.unlock();
  return 0;
}

// flag: 参数flag用于区分是否查询请求是否来自“我的头条”
var_4 Candidate::query_recommend(var_u8 user_id, var_4 flag, var_1* result_buf, var_4 result_max, 
    var_4& result_len)
{
  int32_t current_time = time(NULL);
  return query_recommend(user_id, flag, current_time-choose_minutes_scope*60, current_time, 
      result_buf, result_max, result_len);
}

var_4 Candidate::query_recommend(var_u8 user_id, var_4 flag, int32_t start_time, int32_t end_time, 
    var_1* result_buf, var_4 result_max, var_4& result_len)
{
  struct timeval beg, end;

  var_4 user_index, recommend_items_index; 
  var_4 recommend_num, history_num, items_bak_num;
  var_4 process_len = 0;
  item_info_t* items_bak = NULL;
  var_4*  publish_time = NULL; 
  var_4*  category_id = NULL;
  var_4*  picture_num = NULL;
  var_f4* recommend_power = NULL;
  var_u8* recommend_list = NULL;

  if (result_max <= 8)
    return -1000;

  query_cnt++;
  gettimeofday(&beg, NULL);

  if (end_time > m_last_time + choose_minutes_scope * 60) {// 没有新Item
    *(var_4*)(result_buf) = 0;
    *(var_4*)(result_buf + 4) = 0;
    result_len = 8;
    LOG_ERROR("Candidate:query_recommend", "no new items");
    m_item_lock.unlock();
    return -1001;
  }

  // recycle use memory
  int k = 0;
  items_bak = (item_info_t*)block_alloc(m_item_allocator);

  m_item_lock.lock_w();
  for (int i=(start_time-base_time_)/SECOND_PER_HOUR; 
      k<item_num_limit && i<(end_time-base_time_)/SECOND_PER_HOUR; ++i) {
    std::list<item_index_t>::iterator iter = slip_window_[i].item_list.begin();
    for (; iter != slip_window_[i].item_list.end(); ++iter) {
      if (m_slip_items[iter->item_index].item_id == 0)
        continue;

      if (iter->publish_time < start_time - choose_minutes_scope * 60)
        continue;

      if (iter->publish_time > end_time)
        break;

      items_bak[k++] = m_slip_items[iter->item_index];
    }
  }
  m_item_lock.unlock();

  items_bak_num = k;
  recommend_power = (var_f4*)block_alloc(m_large_allocator);
  publish_time = (var_4*)block_alloc(m_large_allocator);	
  category_id = (var_4*)block_alloc(m_large_allocator);
  picture_num = (var_4*)block_alloc(m_large_allocator);

  // 查找该用户信息
  m_user_lock.lock_w();
  if (m_user_indexer.find(user_id) == m_user_indexer.end())
  {//用户不存在,增加新用户
    user_info_t user_info;
    user_info.user_id = user_id;
    m_user_info.push_back(user_info);
    user_index = m_user_info.size() - 1;
    m_user_indexer[user_id] = std::make_pair(user_index, time(NULL));
  }
  else
  {
    user_index = m_user_indexer[user_id].first;
    m_user_indexer[user_id].second = time(NULL);
  }
  //
  cout <<"user_id: " <<user_id <<", circle and srp num: " <<m_user_info[user_index].v_circle_and_srp.size() <<endl;
  //for (i = 0; i < m_user_info[user_index].v_circle_and_srp.size(); i++)
  //	cout <<m_user_info[user_index].v_circle_and_srp[i] <<",";
  //cout <<endl;

  var_4 ret = cold_boot(user_index, items_bak_num, items_bak, recommend_power, publish_time, flag);
  if (ret)
  {
  }
  recommend_list = (var_u8*)block_alloc(m_large_allocator);
  recommend_items_index = 0;
  // 从待推荐链表中去掉用户已经推荐的和已经阅读的

  for (int i = 0; i < items_bak_num; i++)
  {
    // 该item是否在用户已读列表中
    if (m_user_info[user_index].m_has_read.find(items_bak[i].item_id) != m_user_info[user_index].m_has_read.end())
    {
      continue;
    }
    // 该item是否在用户已推荐列表
    if (m_user_info[user_index].m_has_recommend.find(items_bak[i].item_id) != m_user_info[user_index].m_has_recommend.end())
    {
      continue;
    }
    if (recommend_power[i] == -100000)
      continue;
    recommend_list[recommend_items_index] = items_bak[i].item_id;
    recommend_power[recommend_items_index] = recommend_power[i];
    publish_time[recommend_items_index] = publish_time[i];
    category_id[recommend_items_index] = items_bak[i].category_id;
    picture_num[recommend_items_index] = items_bak[i].picture_num;
    recommend_items_index++;
  }

  m_item_allocator->FreeMem((var_1*)items_bak);

  recommend_num = MIN_FUN(recommend_items_index, (result_max - 8)/12);
  *(var_4*)result_buf = recommend_num;

  memcpy(result_buf + 4, recommend_list, recommend_num * 8);
  process_len = 4 + recommend_num * 8;

  memcpy(result_buf + process_len, recommend_power, recommend_num * 4);
  process_len = process_len + recommend_num * 4;

  memcpy(result_buf + process_len, publish_time, recommend_num * 4);
  process_len = process_len + recommend_num * 4;

  memcpy(result_buf + process_len, category_id, recommend_num * 4);
  process_len = process_len + recommend_num * 4;

  memcpy(result_buf + process_len, picture_num, recommend_num * 4);
  process_len = process_len + recommend_num * 4;

  // test
  cout <<"user id " <<m_user_info[user_index].user_id <<"\t";
  cout <<"top items :";
  for (int i = 0; i <recommend_num; i++)
  {
    if (publish_time[i] == -1)
      cout <<recommend_list[i]<<",";
  }
  cout<<endl;
  //
  history_num = MIN_FUN((result_max - process_len - 4)/8, m_user_info[user_index].v_has_read.size());
  if (history_num < 0)
  {
    history_num = 0;
  }
  *(var_4*)(result_buf + process_len) = history_num;
  if (history_num > 0)
    memcpy(result_buf + process_len + 4, (var_1*)&m_user_info[user_index].v_has_read[0], 8*history_num);
  //
  //   var_1 log_filename[256] = "";
  //   sprintf(log_filename, "log/%lu.txt", m_user_info[user_index].user_id);
  //   ofstream fout_log(log_filename, ios::app);
  //   fout_log<<beg.tv_sec;
  //   for (i = 0; i < recommend_items_index; i++)
  //   fout_log<<","<<recommend_list[i];
  //   fout_log<<endl;
  //   fout_log.close();
  m_user_lock.unlock();

  process_len = process_len + 4 + history_num * 8;	

  result_len = process_len;

  m_large_allocator->FreeMem((var_1*)recommend_list);
  m_large_allocator->FreeMem((var_1*)recommend_power);
  m_large_allocator->FreeMem((var_1*)publish_time);
  m_large_allocator->FreeMem((var_1*)category_id);
  m_large_allocator->FreeMem((var_1*)picture_num);

  if (recommend_items_index <= 0)
  {// 没有用户未阅的item
    LOG_ERROR("Candidate:query_recommend", "no has not readed items");
    return -1002;
  }
  gettimeofday(&end, NULL);

  var_1 query_log[128];
  snprintf(query_log, 128, "query count:%d, query userid:%lu, recommend num:%d, history_num:%d, current time:%ld, cost time: %ld us\n", query_cnt, user_id, recommend_num, history_num, beg.tv_sec, time_diff_ms(beg,end));
  ofstream fout("query.log", ios::app);
  fout<<query_log;
  fout.close();
  printf("%s", query_log);
  return 0;
}

var_4 Candidate::update_pushData(var_u8 user_id, var_4 item_num, var_u8* item_info)
{
  var_4 user_index, item_index;

  m_user_lock.lock_w();
  if (m_user_indexer.find(user_id) == m_user_indexer.end())
  {// 没有该用户，加进去
    user_info_t user_info;
    user_info.user_id = user_id;
    m_user_info.push_back(user_info);
    user_index = m_user_info.size() - 1;
    m_user_indexer[user_id] = std::make_pair(user_index, time(NULL));
  }
  else
  {
    user_index = m_user_indexer[user_id].first;
    m_user_indexer[user_id].second = time(NULL);
  }

  for (int i = 0; i < item_num; i++)
  {
    m_user_info[user_index].v_has_recommend.push_back(item_info[i]);
    item_index = m_user_info[user_index].v_has_recommend.size() - 1;
    m_user_info[user_index].m_has_recommend[item_info[i]] = item_index;
  }
  //// log
  //var_1 log_filename[256] = "";
  //sprintf(log_filename, "log/pushdata_%lu.txt",user_id);
  //ofstream fout_log(log_filename, ios::app);
  //fout_log<<time(NULL);
  //for (int i = 0; i < item_num; i++)
  //	fout_log<<","<<item_info[i];
  //fout_log<<endl;
  //fout_log.close();
  m_user_lock.unlock();

  if (!m_is_init)
  {// 如果不是初始化，写增量
    var_1* buffer = (var_1*)block_alloc(m_large_allocator);

    memcpy(buffer, "INCR", 4);
    memcpy(buffer + 4,  &RESPONSE_RECOMMEND, 4);
    memcpy(buffer + 8,  &user_id, 8);
    memcpy(buffer + 16, &item_num, 4);
    memcpy(buffer + 20, (var_1*)item_info, item_num * 8);

    if (m_data_storage->save(buffer, 20 + item_num * 8))
    {
      LOG_ERROR("Candidate::update_pushData", "m_data_storage->save");
    }
    m_large_allocator->FreeMem(buffer);
  }
  return 0;
}


var_4 Candidate::query_history(var_u8 user_id, var_1* result_buf, var_4 result_max, var_4& result_len)
{
  var_4 user_index, history_num = 0;
  m_user_lock.lock_w();
  if (m_user_indexer.find(user_id) != m_user_indexer.end()) {
    user_index = m_user_indexer[user_id].first;
    m_user_indexer[user_id].second = time(NULL);
    history_num = MIN_FUN((result_max - 4)/8, m_user_info[user_index].v_has_read.size());
  }
  *(var_4*)result_buf = history_num;
  if (history_num > 0)
    memcpy(result_buf + 4, (var_1*)&m_user_info[user_index].v_has_read[0], 8 * history_num);
  result_len = 4 + history_num * 8;
  m_user_lock.unlock();	
  return 0;
}

var_4 Candidate::invalid_old_items()
{
  int i, k, user_index;
  // 淘汰用户,注册用户18天，非注册用户3天
  // 注册用户ID < 10 0000 0000
  // 为了防止长时间加锁，多次淘汰，每次淘汰10个用户
  var_4 delete_user_num_total = 0;
  while (1)
  {
    var_4 delete_user_num = 0;
    m_user_lock.lock_w();
    unmap<var_u8, std::pair<var_4, var_u4> >::iterator it;
    for (it = m_user_indexer.begin(); delete_user_num < 10 && it != m_user_indexer.end();)
    {
      if ((it->first < 1000000000 && it->second.second +  18*SECOND_PER_DAY < time(NULL)) || 
          (it->first >= 1000000000 && it->second.second + 3*SECOND_PER_DAY < time(NULL)))
      {
        printf("delete user id %lu, last access time %d.\n", it->first, it->second.second);	

        user_index = it->second.first;
        m_user_info[user_index].m_circle_and_srp.clear();
        m_user_info[user_index].v_circle_and_srp.clear();
        m_user_info[user_index].m_has_read.clear();
        m_user_info[user_index].v_has_read.clear();
        m_user_info[user_index].m_has_recommend.clear();
        m_user_info[user_index].v_has_recommend.clear();
        m_user_info[user_index].m_dislike.clear();
        m_user_info[user_index].v_dislike.clear();

        delete_user_num++;
        delete_user_num_total++;

        m_user_indexer.erase(it++);
      }
      else
        it++;
    }
    m_user_lock.unlock();
    if (delete_user_num == 0)
      break;
    if (delete_user_num_total > 10000)
      break;
    cp_sleep(10);
  }

  delete_user_num_total = 0;
  while (1)
  {
    var_4 delete_user_num = 0;
    m_user_lock.lock_w();
    for (i=0; delete_user_num < 40 && i< m_user_info.size(); i++)
    {
      if (m_user_info[i].v_circle_and_srp.size() > max_circle_num)
      {
        var_4 delete_num = m_user_info[i].v_circle_and_srp.size() - max_circle_num;
        for (k = 0; k < delete_num; k++)
        {
          if (m_user_info[i].m_circle_and_srp.find(m_user_info[i].v_circle_and_srp[k]) 
              != m_user_info[i].m_circle_and_srp.end())
          {
            m_user_info[i].m_circle_and_srp.erase(m_user_info[i].v_circle_and_srp[k]);
          }
        }
        vector<var_u8>::iterator it = m_user_info[i].v_circle_and_srp.begin();

        for (; 0 < delete_num && it != m_user_info[i].v_circle_and_srp.end(); delete_num--)
        {
          it = m_user_info[i].v_circle_and_srp.erase(it);
        }
        delete_user_num++;
      }
      if (m_user_info[i].v_has_read.size() > max_read_num)
      {
        var_4 delete_num = m_user_info[i].v_has_read.size() - max_read_num;
        for (k = 0; k < delete_num; k++)
        {
          if (m_user_info[i].m_has_read.find(m_user_info[i].v_has_read[k]) 
              != m_user_info[i].m_has_read.end())
          {
            m_user_info[i].m_has_read.erase(m_user_info[i].v_has_read[k]);
          }
        }
        vector<var_u8>::iterator it = m_user_info[i].v_has_read.begin();
        for (; 0 < delete_num && it != m_user_info[i].v_has_read.end(); delete_num--)
        {
          it = m_user_info[i].v_has_read.erase(it);
        }
        delete_user_num++;
      }
      if (m_user_info[i].v_has_recommend.size() > max_recommend_num)
      {
        var_4 delete_num = m_user_info[i].v_has_recommend.size() - max_recommend_num;
        for (k = 0; k < delete_num; k++)
        {
          if (m_user_info[i].m_has_recommend.find(m_user_info[i].v_has_recommend[k]) 
              != m_user_info[i].m_has_recommend.end())
          {
            m_user_info[i].m_has_recommend.erase(m_user_info[i].v_has_recommend[k]);
          }
        }
        vector<var_u8>::iterator it = m_user_info[i].v_has_recommend.begin();
        for (; 0 < delete_num && it != m_user_info[i].v_has_recommend.end(); delete_num--)
        {
          it = m_user_info[i].v_has_recommend.erase(it);
        }
        delete_user_num++;
      }
      if (m_user_info[i].v_dislike.size() > max_dislike_num)
      {
        var_4 delete_num = m_user_info[i].v_dislike.size() - max_dislike_num;
        for (k = 0; k < delete_num; k++)
        {
          if (m_user_info[i].m_dislike.find(m_user_info[i].v_dislike[k])
              != m_user_info[i].m_dislike.end())
          {
            m_user_info[i].m_dislike.erase(m_user_info[i].v_dislike[k]);
          }
        }
        vector<var_u8>::iterator it = m_user_info[i].v_dislike.begin();
        for (; 0 < delete_num && it != m_user_info[i].v_dislike.end(); delete_num--)
        {
          it = m_user_info[i].v_dislike.erase(it);
        }
        delete_user_num++;
      }
    }
    m_user_lock.unlock();
    if (delete_user_num == 0)
      break;
    cp_sleep(10);
  }
  return 0;
}

var_4 Candidate::persistent_library()
{
  var_4 i, num, ret;
  // 先淘汰
  //time_t now_sec = (time(NULL) + 28800) % 86400;
  //if (now_sec > 7200 && now_sec < 21600)
  {
    ret = invalid_old_items();
    if (ret)
      return -1;
  }
  // 落地m_item_hash
  var_1 sto_name[256] ="";
  snprintf(sto_name, 256, "%s/item_info.dat", m_sto_path);

  m_item_lock.lock_w();
  ofstream fout(sto_name);
  unmap<var_u8, item_click_t>::iterator it;
  num = m_item_hash.size(); 
  fout.write((var_1*)&num, 4);
  for (it = m_item_hash.begin(); it != m_item_hash.end(); it++)
  {
    fout.write((var_1*)&it->first, 8);
    fout.write((var_1*)&it->second, sizeof(item_click_t));
  }
  fout.close();
  //
  snprintf(sto_name, 256, "%s/item_top.dat", m_sto_path);
  fout.open(sto_name);
  unmap<var_u8, top_item_t>::iterator another_it;
  num = m_item_top.size();
  fout.write((var_1*)&num, 4);
  for (another_it = m_item_top.begin(); another_it != m_item_top.end(); another_it++)
  {
    fout.write((var_1*)&another_it->first, 8);
    fout.write((var_1*)&another_it->second.global, 1);
    var_4 srps_size = another_it->second.srps.size();
    fout.write((var_1*)&srps_size, 4);
    for (var_4 i = 0; i < srps_size; i++ )
    {
      fout.write((var_1*)&another_it->second.srps[i], 8);
    }
    var_4 circles_size = another_it->second.circles.size();
    fout.write((var_1*)&circles_size, 4);
    for (var_4 i = 0; i < circles_size; i++)
    {
      fout.write((var_1*)&another_it->second.circles[i], 8);
    }
  }
  fout.close();
  m_item_lock.unlock();

  var_1* buffer = block_alloc(m_large_allocator);	
  try
  {
    if (m_data_storage->trim_prepare())
      throw -1;
    if (m_data_storage->trim_data((var_1*)"MAIN", 4))
      throw -2;

    m_user_lock.lock_w();
    var_4 user_num = m_user_info.size(); 
    if (m_data_storage->trim_data((var_1*)&user_num, 4))
    {
      m_user_lock.unlock();
      throw -3;
    }
    for (i = 0; i < user_num; i++)
    {
      if (m_data_storage->trim_data((var_1*)&m_user_info[i].user_id, 8))
      {
        m_user_lock.unlock();
        throw -4;
      }
      // 落地用户圈子和SRP词数据
      num = m_user_info[i].v_circle_and_srp.size();
      if (m_data_storage->trim_data((var_1*)&num, 4))
      {
        m_user_lock.unlock();
        throw -4;
      }
      memcpy(buffer, (var_1*)&m_user_info[i].v_circle_and_srp[0], 8*num);
      if (m_data_storage->trim_data((var_1*)buffer,  num*8))
      {
        m_user_lock.unlock();
        throw -5;
      }
      // 落地用户已阅新闻数据
      num = m_user_info[i].v_has_read.size();
      if (m_data_storage->trim_data((var_1*)&num, 4))
      {
        m_user_lock.unlock();
        throw -6;
      }
      memcpy(buffer, (var_1*)&m_user_info[i].v_has_read[0], 8*num);
      if (m_data_storage->trim_data((var_1*)buffer, num*8))
      {
        m_user_lock.unlock();
        throw -7;
      }
      // 落地用户已推荐新闻数据
      num = m_user_info[i].v_has_recommend.size();
      if (m_data_storage->trim_data((var_1*)&num, 4))
      {
        m_user_lock.unlock();
        throw -8;
      }
      memcpy(buffer, (var_1*)&m_user_info[i].v_has_recommend[0], 8*num);
      if (m_data_storage->trim_data((var_1*)buffer, num*8))
      {
        m_user_lock.unlock();
        throw -9;
      }
      // 落地用户不喜欢新闻数据
      num = m_user_info[i].v_dislike.size();
      if (m_data_storage->trim_data((var_1*)&num, 4))
      {
        m_user_lock.unlock();
        throw -10;
      }
      memcpy(buffer, (var_1*)&m_user_info[i].v_dislike[0], 8*num);
      if (m_data_storage->trim_data((var_1*)buffer, num*8))
      {
        m_user_lock.unlock();
        throw -11;
      }
    }
    m_user_lock.unlock();
    // 落地item数据
    m_item_lock.lock_w();
    if (m_data_storage->trim_data((var_1*)&m_last_time, 4))
    {
      m_item_lock.unlock();
      throw -10;
    }
    int ret_len = serialize_slip_window(buffer, MAX_BUFFER_SIZE);
    if (m_data_storage->trim_data((var_1*)buffer, ret_len))
    {
      m_item_lock.unlock();
      throw -11;
    }
    if (m_data_storage->trim_data((var_1*)&m_last_index, 4))
    {
      m_item_lock.unlock();
      throw -12;
    }
    if (m_data_storage->trim_data((var_1*)m_slip_items, sizeof(item_info_t) * SLIP_ITEM_NUM))
    {
      m_item_lock.unlock();
      throw -13;
    }
    m_item_lock.unlock();
  }
  catch(const var_4 err)
  {
    m_data_storage->trim_failure();
    m_large_allocator->FreeMem((var_1*)buffer);
    return err;
  }
  m_data_storage->trim_success();
  m_large_allocator->FreeMem((var_1*)buffer);
  return 0;
}

var_4 Candidate::load()
{
  var_4 data_length, click_time, value_size;
  var_4 num, user_num, item_num, opt_type, data_type;
  var_u8 item_id, user_id;

  var_1 sto_name[256] = "";	
  item_click_t item_click;

  var_1* buffer = block_alloc(m_large_allocator);

  // recover m_item_hash
  snprintf(sto_name, 256, "%s/item_info.dat", m_sto_path);
  ifstream fin(sto_name);
  if (fin.read((var_1*)&num, 4)<=0)
  {
    num = 0;
  }
  for (int k = 0; k < num; k++)
  {
    fin.read((var_1*)&item_id, 8);
    fin.read((var_1*)&item_click, sizeof(item_click_t));
    m_item_hash[item_id] = item_click;
  }
  fin.close();
  //
  snprintf(sto_name, 256, "%s/item_top.dat", m_sto_path);
  fin.open(sto_name);
  if (fin.read((var_1*)&num, 4)<=0)
  {
    num = 0;
  }
  top_item_t item_top;
  var_4 srps_size, circles_size;
  var_u8 srp_id, circle_id;
  for (int k = 0; k < num; k++)
  {
    fin.read((var_1*)&item_id, 8);
    fin.read((var_1*)&item_top.global, 1);
    fin.read((var_1*)&srps_size, 4);
    for (int i = 0; i < srps_size; i++)
    {
      fin.read((var_1*)&srp_id, 8);
      item_top.srps.push_back(srp_id);
    }
    fin.read((var_1*)&circles_size, 4);
    for (int i = 0; i < circles_size; i++)
    {
      fin.read((var_1*)&circle_id, 8);
      item_top.circles.push_back(circle_id);
    }
    m_item_top[item_id] = item_top;
  }
  fin.close();
  //
  try
  {
    m_data_storage->travel_prepare();
    // 加载用户数据
    var_4 ret = m_data_storage->travel_data(buffer, 4, data_length, data_type);
    if (ret == 1)
      throw 0;
    if (strncmp(buffer, "MAIN", 4) == 0)
    {//主数据
      var_4 user_num = 0;
      if (m_data_storage->travel_data((var_1*)&user_num, 4, data_length, data_type))
        throw -1;
      for (int i = 0; i < user_num; i++)
      {
        user_info_t user_info;
        if (m_data_storage->travel_data((var_1*)&user_info.user_id, 8, data_length, data_type))
          throw -2;

        m_user_indexer[user_info.user_id] = std::make_pair(i, time(NULL));

        // circle and srp num (4 bytes)
        if (m_data_storage->travel_data((var_1*)&num, 4, data_length, data_type))
          throw -3;
        if (num > 0)
        {
          if (m_data_storage->travel_data(buffer, MAX_BUFFER_SIZE, data_length, data_type))
            throw -4;
          for (int k = 0; k < num; k++)
          {
            user_info.v_circle_and_srp.push_back(*(var_u8*)(buffer + 8*k));
            user_info.m_circle_and_srp.insert(unmap<var_u8, var_4>::value_type(*(var_u8*)(buffer + 8*k), k));
          }
        }
        // has read num(4 bytes)
        if (m_data_storage->travel_data((var_1*)&num, 4, data_length, data_type))
          throw -7;
        if (num > 0)
        {
          if (m_data_storage->travel_data(buffer, MAX_BUFFER_SIZE, data_length, data_type))
            throw -8;
          for (int k = 0; k < num; k++)
          {
            user_info.v_has_read.push_back(*(var_u8*)(buffer + 8*k));
            user_info.m_has_read.insert(unmap<var_u8, var_4>::value_type(*(var_u8*)(buffer + 8*k), k));
          }
        }
        // has recommend num(4 bytes)
        if (m_data_storage->travel_data((var_1*)&num, 4, data_length, data_type))
          throw -11;
        if (num > 0)
        {
          if (m_data_storage->travel_data(buffer, MAX_BUFFER_SIZE, data_length, data_type))
            throw -12;
          for (int k = 0; k < num; k++)
          {
            user_info.v_has_recommend.push_back(*(var_u8*)(buffer + 8*k));
            user_info.m_has_recommend.insert(unmap<var_u8, var_4>::value_type(*(var_u8*)(buffer + 8*k), k));
          }
        }
        // dislike num(4 bytes)
        if (m_data_storage->travel_data((var_1*)&num, 4, data_length, data_type))
          throw -15;
        if (num > 0)
        {
          if (m_data_storage->travel_data(buffer, MAX_BUFFER_SIZE, data_length, data_type))
            throw -16;
          for (int k = 0; k < num; k++)
          {
            user_info.v_dislike.push_back(*(var_u8*)(buffer + 8*k));
            user_info.m_dislike.insert(unmap<var_u8, var_4>::value_type(*(var_u8*)(buffer + 8*k), k));
          }
        }
        if (user_info.user_id != 0)
          m_user_info.push_back(user_info);
      }
      // load item info
      if (m_item_hash.size() > 0)
      {
        if (m_data_storage->travel_data((var_1*)&m_last_time, 4, data_length, data_type))
          throw -19;
        if (m_data_storage->travel_data((var_1*)buffer, MAX_BUFFER_SIZE, data_length, data_type))
          throw -20;
        parse_slip_window(buffer, data_length);
        if (m_data_storage->travel_data((var_1*)&m_last_index, 4, data_length, data_type))
          throw -21;
        if (m_data_storage->travel_data((var_1*)m_slip_items, sizeof(item_info_t) * SLIP_ITEM_NUM, data_length, data_type))
          throw -22;
        // 重建
        for (int i = 0; i < SLIP_ITEM_NUM; i++)
        {
          m_slip_hash.insert(unmap<var_u8, var_4>::value_type(m_slip_items[i].item_id, i));	
        }
      }
      else
        throw 0;
    }
    // recover increase data
    while (1)
    {
      if (1 == m_data_storage->travel_data(buffer, MAX_BUFFER_SIZE, data_length, data_type))
        break;
      if (strncmp(buffer, "INCR", 4))
        continue;
      int opt_type = *(var_4*)(buffer + 4);
      switch (opt_type)
      {
        case UPDATE_ITEM:
          ret = update_item(buffer + 8);
          if (ret)
            throw -25;
          break;
        case UPDATE_USER:
          ret = update_user(buffer + 8);
          if (ret)
            throw -28;
          break;
        case UPDATE_CLICK:
          ret = update_click(buffer + 8);
          if (ret)
            throw -32;
          break;
        case RESPONSE_RECOMMEND:
          user_id = *(var_u8*)(buffer + 8);
          item_num = *(var_4*)(buffer + 16);
          ret = update_pushData(user_id, item_num, (var_u8*)(buffer + 20));
          if (ret)
            throw -36;
          break;
        default:
          break;
      }
    }

    throw 0;
  }
  catch (const var_4 err)
  {
    m_data_storage->travel_finish();
    m_large_allocator->FreeMem(buffer);
    return err;
  }
}

void Candidate::parse_slip_window(char* buffer, int buflen)
{
  int num = buflen/sizeof(item_index_t);
  item_index_t* p = (item_index_t*)buffer;
  for (int i=0; i<num; ++i) {
    int index = (p[i].publish_time-base_time_)/SECOND_PER_HOUR;
    slip_window_[index].item_list.push_back(p[i]);
  }
}

int Candidate::serialize_slip_window(char* buffer, int maxsize)
{
  int k = 0;
  item_index_t* p = (item_index_t*)buffer;
  for (size_t i=0; i<0 && k*sizeof(item_index_t)<maxsize; ++i) {
    std::list<item_index_t>::iterator iter = slip_window_[i].item_list.begin();
    for (; iter != slip_window_[i].item_list.end(); ++iter) {
      p[k++] = *iter;
    }
  }
  return k * sizeof(item_index_t);
}

var_4 Candidate::is_update_train()
{
  return 0;
}

var_4 Candidate::module_type()
{
  return 1; // 1 表示初选集
}

var_4 Candidate::is_persistent_library()
{
  return 1;
}

Candidate::~Candidate()
{
}

*/
