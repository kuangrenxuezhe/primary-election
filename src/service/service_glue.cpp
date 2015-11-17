#include "service/service_glue.h"

#include <assert.h>
#include <time.h>
#include <sstream>

#include "glog/logging.h"
#include "util/status.h"
#include "core/options.h"

namespace rsys {
  namespace news {
    static const std::string kConfigFile = "conf/primary_election.conf";

    static void make_time_str(struct tm* ctm, std::vector<std::string>& time_str)
    {
      int tm_val[] = {ctm->tm_min, ctm->tm_hour, ctm->tm_wday, ctm->tm_mday};
      const char* tm_str[] = {"/hour", "/day", "/week", "/mon"};

      for (size_t i = 0; i < sizeof(tm_val)/sizeof(int); ++i) {
        std::ostringstream oss;
        oss << tm_val[i] << tm_str[i];
        time_str.push_back(oss.str()); 
      }
    }

    static bool match_timer(const std::vector<std::string>& time_str, const std::string& timer_str)
    {
      for (size_t i = 0; i < time_str.size(); ++i) {
        if (timer_str != time_str[i])
          continue;
        return true;
      }
      return false;
    }

    void* ServiceGlue::flushTimer(void* arg)
    {
      ServiceGlue* glue = (ServiceGlue*)arg;

      bool has_flushed_candb = false, has_flushed_wal = false;
      // 由于CF_framework无法退出，这里也不做退出处理
      for (;;) {
        time_t ctime = time(NULL);
        struct tm* ctm = localtime(&ctime);

        std::vector<std::string> time_str;

        make_time_str(ctm, time_str);
        // 定期flush候选库
        if (!match_timer(time_str, glue->options_.flush_timer)) {
          has_flushed_candb = false;
        } else {
          if (!has_flushed_candb) { // 保证在这一时间段内只flush一次
            Status status = glue->candb_->flush();
            if (status.ok()) {
              has_flushed_candb = true;
              LOG(INFO) << "flush candidate db.";
            } else {
              LOG(ERROR) << "flush candidate db failed: " << status.toString();
            }
          }
        }

        // 定时每天23点生成增量log文件，且只保留7天数据
        if (!match_timer(time_str, "23/day")) {
          has_flushed_wal = false;
        } else {
          if (!has_flushed_wal) {
            Status status = glue->candb_->rollover(glue->options_.wal_expired_days);
            if (status.ok()) {
              has_flushed_wal = true;
              LOG(INFO) << "roll over wal file.";
            } else {
              LOG(ERROR) << "roll over wal file failed: " << status.toString();
            }
          }
        }
        usleep(10000);
      }
    }

    var_4 ServiceGlue::init_module(var_vd* config_info)
    {
      if (NULL == config_info) {
        LOG(ERROR) << "invalid argument.";
        return -1;
      }

      Status status = Options::fromConf(kConfigFile, options_);
      if (!status.ok()) {
        LOG(FATAL) << "parser config failed: " << status.toString();
        return -1;
      }

      status = CandidateDB::openDB(options_, &candb_);
      if (!status.ok()) {
        LOG(FATAL) << "open db failed: " << status.toString();
        return -1;
      }

      pthread_create(&flush_handler_, NULL, flushTimer, this);
      return 0;
    }

    var_4 ServiceGlue::update_user(var_1* buffer)
    {
      UserSubscribe subscribe;

      if (parseUserSubscribe(buffer, -1, &subscribe)) {
        LOG(ERROR) << "parse user subscribe failed.";
        return -1;
      }

      Status status = candb_->updateUser(subscribe);
      if (!status.ok()) {
        LOG(ERROR) << "update user failed: " << status.toString();
        return -1;
      }
      return 0;
    }

    var_4 ServiceGlue::update_item(var_1* buffer)
    {
      ItemInfo item_info;

      if (parseItemInfo(buffer, -1, &item_info)) {
        LOG(ERROR) << "parse item info failed.";
        return -1;
      }

      Status status = candb_->addItem(item_info);
      if (!status.ok()) {
        LOG(ERROR) << "add item failed: " << status.toString();
        return -1;
      }
      return 0;
    }

    var_4 ServiceGlue::update_click(var_1* buffer)
    {
      UserAction user_action;

      if (parseUserAction(buffer, -1, &user_action)) {
        LOG(ERROR) << "parse user action failed.";
        return -1;
      }

      Status status = candb_->updateAction(user_action);
      if (!status.ok()) {
        LOG(ERROR) << "update user action failed: " << status.toString();
        return -1;
      }
      return 0;
    }

    var_4 ServiceGlue::query_user(var_u8 user_id, var_1* result_buf, var_4 result_max, var_4& result_len)
    {
      bool user_exist = false;
      if (candb_->findUser(user_id)) {
        user_exist = true; 
      }

      result_len = serializeUserInfo(user_exist, result_buf, result_max);
      return 0;
    }

    var_4 ServiceGlue::query_recommend(var_u8 user_id, var_4 flag, var_4 beg_time, var_4 end_time, 
        var_1* result_buf, var_4 result_max, var_4& result_len)
    {
      CandidateQuery query;
      CandidateSet candidate_set;

      query.set_user_id(user_id);
      query.set_start_time(beg_time);
      query.set_end_time(end_time);
      query.set_item_type(ITEM_TYPE_NONE);
      candb_->queryCandidateSet(query, candidate_set);
      result_len = serializeCandidateSet(candidate_set, result_buf, result_max);
      return 0;
    }

    var_4 ServiceGlue::query_history(var_u8 user_id, var_1* result_buf, var_4 result_max, var_4& result_len)
    {
      IdSet id_set;

      candb_->queryHistory(user_id, id_set); 
      result_len = serializeUserHistory(id_set, result_buf, result_max);
      return 0;
    }

    var_4 ServiceGlue::update_pushData(var_u8 user_id, var_4 push_num, var_u8* push_data)
    {
      IdSet id_set;
      for (int i = 0; i < push_num; ++i) {
        id_set.add_id(push_data[i]);
      }

      Status status = candb_->updateCandidateSet(user_id, id_set);
      if (!status.ok()) {
        LOG(ERROR) << "update recommendation set failed: " << status.toString();
        return -1;
      }
      return 0;
    }

    // for candidate: type(var_4, no_use), clicktime(var_4), user_id(var_u8), doc_id(var_u8), staytime(var_4), action(var_4),
    //                location_len(var_4), location(location_len), srpid_len(var_4), srpid(srpid_len)
    //                source(var_4)
    int ServiceGlue::parseUserAction(const char* buffer, int buflen, UserAction* user_action)
    {
      int len;
      const char* p = buffer;
      p += 4; // type(4Bytes, 没有使用)
      user_action->set_click_time(*(int32_t*)p);
      p += sizeof(int32_t);
      user_action->set_user_id(*(uint64_t*)p);
      p += sizeof(uint64_t);
      user_action->set_item_id(*(uint64_t*)p);
      p += sizeof(uint64_t);
      user_action->set_stay_time(*(int32_t*)p);
      p += sizeof(int32_t);
      if (Action_IsValid(*(int32_t*)p))
        user_action->set_action((Action)(*(int32_t*)p));
      p += sizeof(int32_t);
      len = *(int32_t *)p;
      p += sizeof(int32_t);
      user_action->set_location(p, len);
      p += len;

      len = *(int32_t*)p;
      p += sizeof(int32_t);
      
      std::string srpid(p, len);
      user_action->set_srp_id(atoi(srpid.c_str()));
      p += len;

      user_action->set_source_id(*(int32_t*)p);
      return 0;
    }

    // type(var_4, no_use), doc_id(var_u8), public_time(var_4), push_time(var_4), power(var_f4),
    // category_num(var_4), [category_name_len(var_4), category_name(category_name_len)] * category_num,
    // word_num(var_4), [word_len(var_4), word(word_len), word_power(var_4)] * word_num,
    // srp_num(var_4), [srp_name_len(var_4), srp_name(srp_name_len)] * srp_num,
    // circle_num(var_4), [circle_name_len(var_4), circle_name(circle_name_len)] * circle_num,
    // picture_num(var_4),
    // top_flag(var_4),
    // top_srp_num(var_4), [top_srp_name_len(var_4), top_srp_name(top_srp_name_len)] * top_srp_num,
    // top_circle_num(var_4), [top_circle_name_len(var_4), top_circle_name(top_circle_name_len)] * top_circle_num,
    // doc_type(var_4), srp_power_num(var_4), [srp_power(var_f4)] * srp_power_num
    int ServiceGlue::parseItemInfo(const char* buffer, int len, ItemInfo* item_info)
    {
      int num;
      const char* p = buffer;
      p += 4; // no use
      item_info->set_item_id(*(uint64_t*)p);
      p += sizeof(uint64_t);
      item_info->set_publish_time(*(int32_t*)p);
      p += sizeof(int32_t);
      item_info->set_push_time(*(int32_t*)p);
      p += sizeof(int32_t);
      item_info->set_power(*(float*)p);
      p += sizeof(float);
      num = *(int *)p;
      p += sizeof(int);
      for (int i = 0; i < num; ++i) {
        int len = *(int *)p;
        p += sizeof(int);
        item_info->add_category(p, len);
        p += len;
      }

      num = *(int*)p;
      p += sizeof(int);
      for (int i = 0; i < num; ++i) {
        Word* word = item_info->add_word();
        int len = *(int*)p;
        p += sizeof(int);
        word->set_word(p, len);
        p += len;
        word->set_count((float)(*(int32_t*)p));
        p += sizeof(int32_t);
      }
      num = *(int*)p;
      p += sizeof(int);
      for (int i = 0; i < num; ++i) {
        Srp* srp = item_info->add_srp();
        int len = *(int *)p;
        p += sizeof(int);
        std::string srp_id(p, len);
        srp->set_srp_id(atoi(srp_id.c_str()));
      }
      num = *(int*)p;
      p += sizeof(int);
      for (int i = 0; i < num; ++i) {
        int len = *(int *)p;
        p += sizeof(int);
        std::string circle_id(p, len);
        item_info->add_circle_id(atoi(circle_id.c_str()));
      }
      item_info->set_picture_num(*(int32_t*)p);
      p += sizeof(int32_t);

      TopInfo* top_info = item_info->mutable_top_info();
      if (TopFlag_IsValid(*(int*)p))
        top_info->set_top_flag((TopFlag)(*(int*)p));
      p += sizeof(int);
      num = *(int*)p;
      p += sizeof(int);
      for (int i = 0; i < num; ++i) {
        Srp* srp = top_info->add_srp();
        int len = *(int *)p;
        p += sizeof(int);
        std::string srp_id(p, len);
        srp->set_srp_id(atoi(srp_id.c_str()));
      }
      num = *(int*)p;
      p += sizeof(int);
      for (int i = 0; i < num; ++i) {
        int len = *(int *)p;
        p += sizeof(int);
        std::string circle_id(p, len);
        top_info->add_circle_id(atoi(circle_id.c_str()));
      }
      return 0; 
    }

    // type(var_4, no_use), user_id(var_u8),
    // srp_num(var_4), [srp_name_len(var_4), srp_name(srp_name_len)] * srp_num,
    // circle_num(var_4), [circle_name_len(var_4), circle_name(circle_name_len)] * circle_num
    int ServiceGlue::parseUserSubscribe(const char* buffer, int len, UserSubscribe* user_subscribe)
    {
      int num;
      const char* p = buffer;
      p += 4; // no use
      user_subscribe->set_user_id(*(uint64_t*)p);
      p += sizeof(uint64_t);
      
      num = *(int*)p;
      p += sizeof(int);
      for (int i = 0; i < num; ++i) {
        int len = *(int *)p;
        std::string srp_id(p, len);
        user_subscribe->add_srp_id(atoi(srp_id.c_str()));
        p += len;
      }

      num = *(int *)p;
      p += sizeof(int);
      for (int i = 0; i < num; ++i) {
        int len = *(int *)p;
        std::string circle_id(p, len);
        user_subscribe->add_circle_id(atoi(circle_id.c_str()));
      }
      return 0;
    }

    // var_4 new_user_flag: 1 - new user, 0 - old user
    int ServiceGlue::serializeUserInfo(bool user_exist, char* buffer, int maxlen)
    {
      if (maxlen < sizeof(int32_t)) {
        LOG(ERROR) << "not enough memory.";
        return 0;
      }

      int32_t value = user_exist ? 1:0;
      *(int32_t *)buffer = value;

      return sizeof(int32_t);
    }

    // var_4& recommend_num, var_u8* recommend_list, var_f4* recommend_power, var_4* recommend_time, var_4* recommend_class, var_4* recommend_picnum, var_4& history_num, var_u8* history_list
    int ServiceGlue::serializeCandidateSet(const CandidateSet& candset, char* buffer, int maxlen)
    {
      char* p = buffer;
      if (maxlen < 8 + 20*candset.candidate_size() + 8*candset.history_size()) {
        LOG(ERROR) << "not enough memory.";
        return 0;
      }

      *(int32_t*)p = candset.candidate_size();
      p += sizeof(int32_t);
      for (int i = 0; i < candset.candidate_size(); ++i) {
        *(uint64_t*)p = candset.candidate(i).item_id();
        p += sizeof(uint64_t);
      }
      for (int i = 0; i < candset.candidate_size(); ++i) {
        *(float*)p = candset.candidate(i).power();
        p += sizeof(float);
      }
      for (int i = 0; i < candset.candidate_size(); ++i) {
        *(int32_t*)p = candset.candidate(i).publish_time();
        p += sizeof(int32_t);
      }
      for (int i = 0; i < candset.candidate_size(); ++i) {
        *(int32_t*)p = candset.candidate(i).category_id();
        p += sizeof(int32_t);
      }
      for (int i = 0; i < candset.candidate_size(); ++i) {
        *(int32_t*)p = candset.candidate(i).picture_num();
        p += sizeof(int32_t);
      }
      *(int32_t*)p = candset.history_size();
      for (int i = 0; i < candset.candidate_size(); ++i) {
        *(int32_t*)p = candset.candidate(i).picture_num();
        p += sizeof(int32_t);
      }

      return p - buffer;
    }

    // var_4& history_num, var_u8* history_list
    int ServiceGlue::serializeUserHistory(const IdSet& id_set, char* buffer, int maxlen)
    {
      char* p = buffer;

      if (maxlen < sizeof(int32_t) + sizeof(uint64_t)*id_set.id_size()) {
        LOG(ERROR) << "not enough memory.";
        return 0;
      }

      *(int32_t*)p = id_set.id_size();
      p += sizeof(int32_t);
      for (size_t i = 0; i < id_set.id_size(); ++i) {
        *(uint64_t*)p = id_set.id(i);
        p += sizeof(uint64_t);
      }

      return p - buffer;
    }
  }; // namespace news
}; // namespace rsys

