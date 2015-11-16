#include "core/user_table.h"

#include <unistd.h>
#include <string.h>

#include "util/crc32c.h"
#include "glog/logging.h"

#include "proto/record.pb.h"
#include "proto/service.pb.h"

namespace rsys {
  namespace news {
    static const uint32_t kUserTableMajorVersion = 0;
    static const uint32_t kUserTableMinorVersion = 1;

    // 默认同步库时间间隔为60分钟
    static const int32_t kDefaultSyncGap =  60;     

    class UserUpdater: public LevelTable<uint64_t, user_info_t>::Updater {
      public:
        UserUpdater(user_info_t* user_info)
          : user_info_(user_info) {
          }
        virtual ~UserUpdater() {
        }
        virtual bool update(user_info_t* user_info) {
          // 用户订阅信息为全量更新
          user_info->srp = user_info_->srp;
          user_info->circle = user_info_->circle;
          return true;
        }

        virtual user_info_t* clone(user_info_t* value) {
          return NULL;
        }
      private:
        user_info_t* user_info_;
    };

    class ActionUpdater: public LevelTable<uint64_t, user_info_t>::Updater {
      public:
        ActionUpdater(const action_t& user_action)
          : user_action_(user_action) {
          }
        virtual ~ActionUpdater() {
        }
        virtual bool update(user_info_t* user_info) {
          std::pair<uint64_t, int32_t> item_id = 
            std::make_pair(user_action_.item_id, time(NULL));
          if (user_action_.action == CLICK) {
            user_info->readed.insert(item_id);
          } else if (user_action_.action == DISLIKE) {
            user_info->dislike.insert(item_id);
          }
          return true;
        }
        virtual user_info_t* clone(user_info_t* value) {
          return NULL;
        }
      private:
        const action_t& user_action_;
    };

    class CandidateUpdater: public LevelTable<uint64_t, user_info_t>::Updater {
      public:
        CandidateUpdater(const id_set_t& id_set)
          : id_set_(id_set) {
          }
        virtual ~CandidateUpdater() {
        }
        virtual bool update(user_info_t* user_info) {
          int32_t ctime = time(NULL);
          id_set_t::iterator iter = id_set_.begin();
          for (; iter != id_set_.end(); ++iter) {
            user_info->recommended.insert(std::make_pair(*iter, ctime));
          }
          return true;
        }
        virtual user_info_t* clone(user_info_t* value) {
          return NULL;
        }
      private:
        const id_set_t& id_set_;
    };

    class HistoryFetcher: public LevelTable<uint64_t, user_info_t>::Updater {
      public:
        HistoryFetcher(id_set_t& id_set)
          : id_set_(id_set) {
          }
        virtual ~HistoryFetcher() {
        }
        virtual bool update(user_info_t* user_info) {
          itemid_map_t::iterator iter = user_info->readed.begin();
          for (; iter != user_info->readed.end(); ++iter) {
            //TODO: 添加过期判定
            id_set_.insert(iter->first);
          }
          return true;
        }
        virtual user_info_t* clone(user_info_t* value) {
          return NULL;
        }
      private:
        id_set_t& id_set_;
    };

    class CandidateFilter: public LevelTable<uint64_t, user_info_t>::Updater {
      public:
        CandidateFilter(candidate_set_t& cand_set)
          : cand_set_(cand_set) {
          }
        virtual ~CandidateFilter() {
        }
        virtual bool update(user_info_t* user_info) {
          candidate_set_t::iterator iter = cand_set_.begin();
          while (iter != cand_set_.end()) {
            if (user_info->readed.find(iter->item_id) != user_info->readed.end()) {
              cand_set_.erase(iter++);
              continue;
            }
            if (user_info->dislike.find(iter->item_id) != user_info->dislike.end()) {
              cand_set_.erase(iter++);
              continue;
            }
            if (user_info->recommended.find(iter->item_id) != user_info->recommended.end()) {
              cand_set_.erase(iter++);
              continue;
            }
            ++iter;
          }
          return true;
        }
        virtual user_info_t* clone(user_info_t* value) {
          return NULL;
        }
      private:
        candidate_set_t& cand_set_;
    };

    class EliminateUpdater: public LevelTable<uint64_t, user_info_t>::Updater {
      public:
        EliminateUpdater() {
        }
        virtual ~EliminateUpdater() {
        }
        virtual bool update(user_info_t* user_info) {
          return true;
        }
        virtual user_info_t* clone(user_info_t* value) {
          return NULL;
        }
    };


    //0 淘汰删除, 1 订阅更新，2 点击(阅读和不喜欢), 3 推荐过)
    // userid + flag
    // userid + num + list + num + list
    // userid + itemid + clicktime
    // userid + num + list*
    struct elimination_ {
      int32_t ctime; // 删除时间
    };
    typedef struct elimination_ elimination_t;
    struct subcribe_ {
      std::vector<std::string> srp;
      std::vector<std::string> circle;
    };
    typedef struct subscrib_ subscrib_t;
    struct click_ {
      uint64_t itemid;
      int32_t ctime; // 点击时间
    };
    typedef struct click_ click_t;
    struct recommend_ {
      int num;
      std::vector<uint64_t> item_list;
    };
    typedef struct recommend_ recommend_t;

    UserTable::UserTable(const char* path, const char* prefix)
      : max_level_(0), size_(0), sync_gap_()
        , path_("."), prefix_("user")
    {
      //mutex_ = new pthread_mutex_t;
      //if (NULL != path && '\0' == path[0])
      //  path_ = path;
      //if (NULL != prefix && '\0' == prefix[0])
      //  prefix_ = prefix;
      //pthread_mutex_init(mutex_, NULL);
    }

    UserTable::~UserTable()
    {
      //if (mutex_)
      //  delete mutex_;
      //if (user_map_) {
      //  for (int i=0; i<max_level_; ++i) {
      //    if (user_map_[i])
      //      delete user_map_[i];
      //  }
      //  delete[] user_map_;
      //}
      //if (rwlock_) {
      //  for (int i=0; i<max_level_; ++i) {
      //    if (rwlock_[i])
      //      delete rwlock_[i];
      //  }
      //  delete[] rwlock_;
      //}
    }

    // 加载用户表
    // 文件存储：
    //  prefix.tab: 当前库
    //  prefix.log.levelN: 滚存的log库, L0为最新
    Status UserTable::loadTable()
    {
      fver_t fver;
      std::string tab_name = options().path + "/user.tab";
      LevelFileReader reader(tab_name);

      Status status = reader.open(fver);
      if (!status.ok()) {
        return status;
      }

      if (fver.flag != kVersionFlag) {
        std::ostringstream oss;
        oss<<"Invalid file version flag: expect="<<kVersionFlag
          <<", real="<<fver.flag;
        return Status::IOError(oss.str());
      }
      // 可以做多版本处理
      std::string data;

      status = reader.read(data);
      while (status.ok()) {
        uint64_t user_id;
        user_info_t* user_info;

        if (!parseFrom(data, user_id, user_info)) {
        } else {
          if (level_table_.add(user_id, user_info)) {
            ;
          }
        }
        status = reader.read(data);
      }
      reader.close();
      level_table_.deepen();

      return recoverFromLogFile();
     /*
         char fullpath[300];

         sprintf(fullpath, "%s/%s.tab", path_.c_str(), prefix_.c_str());
         if (0 == access(fullpath, R_OK)) {
         loadTable(fullpath);
         }

      // 第一次加载L0为prefix.tab，在加载log时需要滚存都L1
      if (user_map_ && user_map_[0] && user_map_[0]->size() > 0)
      rollOverTable();

      for (int i=max_level_; i>=0; --i) {
      sprintf(fullpath, "%s/%s.level%d", path_.c_str(), prefix_.c_str(), i);
      if (access(fullpath, R_OK))
      continue;
      loadTableLog(fullpath); 
      }
      return 0; */
    }

    bool UserTable::parseFrom(const std::string& data, uint64_t* user_id, user_info_t* user_info)
    {
      UserRecord user_record;
      if (!user_record.ParseFromString(data))
        return false;

      *user_id = user_record.user_id();
      user_info->ctime = user_record.ctime();
      for (int i = 0; i < user_record.srp_id_size(); ++i) {
        user_info->srp.insert(user_record.srp_id(i));
      }
      for (int i = 0; i < user_record.circle_id_size(); ++i) {
        user_info->circle.insert(user_record.circle_id(i));
      }
      for (int i = 0; i < user_record.dislike_size(); ++i) {
        const ItemPair& pair  = user_record.dislike(i);
        user_info->dislike.insert(std::make_pair(pair.item_id(), pair.ctime()));
      }
      for (int i = 0; i < user_record.readed(); ++i) {
        const ItemPair& pair  = user_record.readed(i);
        user_info->readed.insert(std::make_pair(pair.item_id(), pair.ctime()));
      }
      for (int i = 0; i < user_record.recommended(); ++i) {
        const ItemPair& pair  = user_record.recommended(i);
        user_info->recommended.insert(std::make_pair(pair.item_id(), pair.ctime()));
      }
      return true;
    }

    bool UserTable::serializeTo(uint64_t user_id, const user_info_t* user_info, std::string& data)
    {
      UserRecord user_record;

      user_record.set_user_id(user_id);
      for (id_set_t::const_iterator citer = user_info->srp.begin();
          citer != user_info->srp.end(); ++citer) {
        user_record.add_srp_id(*citer);
      }
      for (id_set_t::const_iterator citer = user_info->circle.begin();
          citer != user_info->circle.end(); ++citer) {
        user_record.add_circle_id(*citer);
      }
      for (itemid_map_t::const_iterator citer = user_info->dislike.begin();
          citer != user_info->dislike.end(); ++citer) {
        ItemPair* pair = user_record.add_dislike();
        pair->set_item_id(citer->first);
        pair->set_ctime(citer->second);
      }
      for (itemid_map_t::const_iterator citer = user_info->readed.begin();
          citer != user_info->readed.end(); ++citer) {
        ItemPair* pair = user_record.add_readed();
        pair->set_item_id(citer->first);
        pair->set_ctime(citer->second);
      }
      for (itemid_map_t::const_iterator citer = user_info->recommended.begin();
          citer != user_info->recommended.end(); ++citer) {
        ItemPair* pair = user_record.add_recommended();
        pair->set_item_id(citer->first);
        pair->set_ctime(citer->second);
      }
      data = user_record.SerializeAsString();
      return true;
    }

    /*
    // 库文件格式:
    //   Flag + Version: 4Bytes
    //   Total: 4Bytes UserInfo个数
    //      Length: UserInfo长度
    //      <UserID: 8Bytes
    //       Circle&SRP Num: 圈子和SRP格式
    //       Circle&SRP*
    //       Dislike Num: 不喜欢新闻个数
    //       Dislike*
    //       Readed Num: 阅读过的新闻个数
    //       Readed*
    //       Recommended Num: 推荐过的新闻个数
    //       Recommended*>*
    //   CRC： CRC验证码
    Status UserTable::loadTable(const char* fullpath)
    {
    uint32_t version;
    int32_t total, length;
    uint32_t valid_crc = 0, crc = 0;

    int buflen = (1<<20);
    char* buffer = new char[buflen];

    FILE* rfd = fopen(fullpath, "rb");
    if (NULL == rfd) {
    LOG(ERROR) << "open file " << fullpath << " failed: " << strerror(errno);
    goto FAILED;
    }
    if (!fread(&version, sizeof(uint32_t), 1, rfd)) {
    LOG(ERROR) << "read file " << fullpath << " failed: " << strerror(errno);
    goto FAILED;
    }
    // 验证flag
    if ((version&0xFF000000) != (kVersion&0xFF000000)) {
    LOG(ERROR) << std::hex << "file flag failed: expect=" << ((kVersion&0xFF000000)>>24)
    << ", real=" << ((version&0xFF000000)>>24);
    goto FAILED;
    }
    // 可分版本处理
    if (!fread(&total, sizeof(int32_t), 1, rfd)) {
    LOG(ERROR) << "read file " << fullpath << " failed: " << strerror(errno);
    goto FAILED;
    }
    crc = crc::extend(crc, (char*)&total, sizeof(int32_t));

    for (int i = 0; i<total; ++i) {
    if (!fread(&length, sizeof(int32_t), 1, rfd)) {
    LOG(ERROR) << "read file " << fullpath << " failed: " << strerror(errno);
    goto FAILED;
    }
    crc = crc::extend(crc, (char*)&length, sizeof(int32_t));

    if (length > buflen)
    buffer = growBuffer(buffer, length, buflen);

    if (!fread(buffer, length, 1, rfd)) {
    LOG(ERROR) << "read file " << fullpath << " failed: " << strerror(errno);
    goto FAILED;
    }
    crc = crc::extend(crc, (char*)&buffer, length);

    uint64_t userid = *(uint64_t*)buffer;
    user_info_t* user_info = parseUserInfo(buffer+sizeof(uint64_t), 
    length-sizeof(uint64_t), NULL);

    if (addUser(userid, user_info)) {
    LOG(ERROR) << std::hex << "add user: " << userid << " failed.";
    goto FAILED;
    }
    }

    if (!fread(&valid_crc, sizeof(uint32_t), 1, rfd)) {
      LOG(ERROR) << "read file " << fullpath << " failed: " << strerror(errno);
      goto FAILED;
    }
    if (valid_crc != crc::mask(crc)) {
      LOG(ERROR) << std::hex << "invalid crc: expect=" << valid_crc 
        << ", real=" << crc::mask(crc);
    }
    delete[] buffer;
    fclose(rfd);
    return 0;
FAILED:
    delete[] buffer;
    if (rfd)
      fclose(rfd);
    return -1;
  }
  */

    /*
       int UserTable::addUser(uint64_t userid, user_info_t* user_info)
       {
       pthread_rwlock_wrlock(rwlock_[0]);
       user_map_t::iterator iter = user_map_[0]->find(userid);
       if (iter != user_map_[0]->end()) {
       LOG(WARNING) << std::hex << "user has exist: " << userid;
       pthread_rwlock_unlock(rwlock_[0]);
       return -1;
       }
       std::pair<user_map_t::iterator, bool> ret 
       = user_map_[0]->insert(std::make_pair(userid, user_info));
       if (!ret.second) {
       LOG(ERROR) << std::hex << "insert user: " << userid;
       pthread_rwlock_unlock(rwlock_[0]);
       return -1;
       }
       pthread_rwlock_unlock(rwlock_[0]);
       return 0;
       }
       */

    /*
    // UserInfo 序列化格式:
    //       Circle&SRP Num: 圈子和SRP格式
    //       Circle&SRP*
    //       Dislike Num: 不喜欢新闻个数
    //       Dislike*
    //       Readed Num: 阅读过的新闻个数
    //       Readed*
    //       Recommended Num: 推荐过的新闻个数
    //       Recommended*>*
    user_info_t* UserTable::parseUserInfo(const char* buffer, int len, user_info_t* user_info)
    {
    user_info_t* ret_user_info = user_info;
    if (NULL == ret_user_info)
    ret_user_info = new user_info_t;

    const char* p = buffer;
    // 解析圈子和SRP
    int32_t num = *(int32_t*)p;
    p += sizeof(int32_t);
    uint64_t* ids = (uint64_t*)p;
    p += sizeof(uint64_t)*num;
    for (int32_t i = 0; i<num; ++i) {
    user_info->circle_and_srp.insert(ids[i]);
    }
    // 解析不喜欢新闻
    num = *(int32_t*)p;
    p += sizeof(int32_t);
    itemid_t* item_ids = (itemid_t*)p;
    p += sizeof(itemid_t)*num;
    for (int32_t i = 0; i<num; ++i) {
    user_info->dislike.insert(item_ids[i]);
    }
    // 解析阅读过新闻
    num = *(int32_t*)p;
    p += sizeof(int32_t);
    item_ids = (itemid_t*)p;
    p += sizeof(itemid_t)*num;
    for (int32_t i = 0; i<num; ++i) {
    user_info->readed.insert(item_ids[i]);
    }
    // 解析推荐过新闻
    num = *(int32_t*)p;
    p += sizeof(int32_t);
    item_ids = (itemid_t*)p;
    p += sizeof(itemid_t)*num;
    for (int32_t i = 0; i<num; ++i) {
    user_info->recommended.insert(item_ids[i]);
    }

    if (p - buffer != len) {
    LOG(ERROR) << "parse overflow: expect length=" << len << 
    ", real length=" << p - buffer;
    if (NULL == user_info)
    delete ret_user_info;
    return NULL;
    }
    return ret_user_info;
    }

    int UserTable::serializeUserInfo(user_info_t* user_info, char* buffer, int len)
    {

    }
    // LOG文件格式:
    //   Flag + Version: 4Bytes
    //    <Length: 数据长度
    //     DataType: 数据类型(0 淘汰删除, 1 订阅更新，2 点击(阅读和不喜欢), 3 推荐过)
    //     
    int UserTable::loadTableLog(const char* fullpath)
    {
    uint32_t version;
  int32_t length;

  int buflen = (1<<20);
  char* buffer = new char[buflen];

  FILE* rfd = fopen(fullpath, "rb");
  if (NULL == rfd) {
    LOG(ERROR) << "open file " << fullpath << " failed: " << strerror(errno);
    goto FAILED;
  }
  if (!fread(&version, sizeof(uint32_t), 1, rfd)) {
    LOG(ERROR) << "read file " << fullpath << " failed: " << strerror(errno);
    goto FAILED;
  }
  // 验证flag
  if ((version&0xFF000000) != (kVersion&0xFF000000)) {
    LOG(ERROR) << std::hex << "file flag failed: expect=" << ((kVersion&0xFF000000)>>24)
      << ", real=" << ((version&0xFF000000)>>24);
    goto FAILED;
  }
  // 可分版本处理
  while (fread(&length, sizeof(int32_t), 1, rfd)) {
    if (length > buflen)
      buffer = growBuffer(buffer, length, buflen);

    if (!fread(buffer, length, 1, rfd)) {
      LOG(ERROR) << "read file " << fullpath << " failed: " << strerror(errno);
      goto FAILED;
    }
    //TODO:     
  }

  fclose(rfd);
  return 0;
FAILED:
  if (rfd)
    fclose(rfd);
  return -1;
  }

  int UserTable::dumpTable(user_map_t* l1, user_map_t* l2, const char* fullpath)
  {
    user_map_t::iterator iter;
    FILE* wfd = fopen(fullpath, "wb");
    if (NULL == wfd) {
      LOG(ERROR) << "open file " << fullpath << " failed: " << strerror(errno);
      goto FAILED;
    }
    for (iter = l2->begin(); iter != l2->end(); ++iter) {
      user_map_t::iterator iter_find = l1->find(iter->first);
      if (iter_find == l1->end()) {

      }
    }
    return 0;
FAILED:
    if (wfd)
      fclose(wfd);
    return -1;
  }
  */

    // 保存用户表
    Status UserTable::flushTable()
    {
      Status status = rollOverLogFile();
      if (!status.ok()) {
        return status;
      }

      level_table_t::Iterator iter = level_table_.snapshot();
      if (!iter.valid()) {
        return Status::Corruption("table depth not enough");
      }
      fver_t fver;
      std::string tab_name = options().path + "/user.tab.tmp";
      LevelFileWriter writer(tab_name);

      fver.flag = kVersionFlag;
      fver.major = kUserTableMajorVersion;
      fver.minor = kUserTableMinorVersion;

      status = writer.create(fver);
      if (!status.ok()) {
        return status;
      }
      std::string serialized_str;

      for (; iter.hasNext(); iter.next()) {
        if (!serializeTo(iter.key(), iter.value(), serialized_str)) {
        } else {
          status = writer.write(serialized_str);
          if (!status.ok()) {
            writer.close();
            return status;
          }
        }
      }
      writer.close();

      return applyTableFile(tab_name);
 
      return Status::OK();
      /*
         if (size_ <= 2) {
         LOG(INFO) << "no level table to sync: size=" << size_;
         return 0;
         }
         char fullpath[300];

         sprintf(fullpath, "%s/%s.tab.dump", path_.c_str(), prefix_.c_str());
         pthread_mutex_lock(mutex_);
         if (dumpTable(user_map_[size_-2], user_map_[size_-1], fullpath)) {
         pthread_mutex_unlock(mutex_);
         return -1;
         }

         user_map_t::iterator iter_from = user_map_[size_ - 2]->begin();
         for (; iter_from != user_map_[size_ - 2]->end(); ++iter_from) {
         pthread_rwlock_wrlock(rwlock_[size_ - 1]);

         user_info_t* user_info = NULL;
         user_map_t::iterator iter_to = user_map_[size_ - 1]->find(iter_from->first);

         if (iter_to != user_map_[size_ - 1]->end())
         user_info = iter_to->second;

      // user_info_t* == NULL时表示删除user_info_t
      if (NULL != iter_from->second) {
      iter_to->second = iter_from->second; 
      } else {
      user_map_[size_ - 1]->set_deleted_key(iter_from->first);
      user_map_[size_ - 1]->erase(iter_from->first);
      }
      if (user_info)
      delete user_info;
      pthread_rwlock_unlock(rwlock_[size_ - 1]);
      }
      pthread_rwlock_wrlock(rwlock_[size_ - 2]);
      user_map_t* user_map = user_map_[size_ - 1];
      user_map_[size_ - 2] = user_map_[size_ - 1];
      pthread_rwlock_unlock(rwlock_[size_ - 2]);
      delete user_map;
      pthread_mutex_unlock(mutex_);

      return 0;
      */
    }

  /*
     int UserTable::rollOverTable()
     {
     if (size_ >= max_level_) {
     LOG(WARNING) << "roll over table trigger syncTable.";
     syncTable();
     }

     pthread_mutex_lock(mutex_);
     if (size_ >= max_level_) {
     LOG(ERROR) << "no enough level to roll over: size=" << size_
     << ", max_level=" << max_level_;
     pthread_mutex_unlock(mutex_);
     return -1;
     }
     user_map_t* new_user_map = new user_map_t();

     pthread_rwlock_wrlock(rwlock_[0]);
     for (int i = size_ - 1; i > 0; --i)
     user_map_[i + 1] = user_map_[i];
     user_map_[0] = new_user_map;
     pthread_rwlock_unlock(rwlock_[0]);

     pthread_mutex_unlock(mutex_);
     return 0;
     }
     */

  void UserTable::set_max_level(int level)
  {
    if (level <= max_level_)
      return;

    //user_map_t** new_user_map;
    //pthread_rwlock_t** new_rwlock;

    //pthread_mutex_lock(mutex_);
    //new_user_map = new user_map_t*[level];
    //memset(new_user_map, 0, sizeof(user_map_t*)*level);
    //new_rwlock = new pthread_rwlock_t*[level];
    //memset(new_rwlock, 0, sizeof(pthread_rwlock_t*)*level);
    //if (max_level_ > 0) {
    //  memcpy(new_user_map, user_map_, sizeof(user_map_t*)*max_level_);
    //  memcpy(new_rwlock, rwlock_, sizeof(pthread_rwlock_t*)*max_level_);
    //}
    //user_map_t** old_user_map = user_map_;
    //pthread_rwlock_t** old_rwlock = rwlock_;

    //max_level_ = level;
    //user_map_ = new_user_map;
    //rwlock_ = new_rwlock;

    //delete[] old_user_map;
    //delete[] old_rwlock;
    //pthread_mutex_unlock(mutex_);
  }

  /*
  // 全局更新订阅
  int UserTable::updateSubscribe(uint64_t* casid, int num)
  {
  }
  */

  // 增量更新已读新闻
  Status UserTable::updateReaded(uint64_t user_id, const action_t& user_action)
  {
    if (level_table_.find(user_id)) {
      std::ostringstream oss;

      // 点击了已淘汰的数据则不记录用户点击
      oss << "Obsolete user: " << std::hex << user_id;
      return Status::InvalidArgument(oss.str());
    } 
    ActionUpdater updater(user_action);

    if (level_table_.update(user_id, updater)) {
      return Status::OK();
    }
    std::stringstream oss;

    oss << "Cann`t update user action: user_id=" << user_id
      << ", item_id=" << user_action.item_id;
    return Status::InvalidArgument(oss.str());
  }

  // 增量更新不喜欢新闻
  int UserTable::updateDislike(uint64_t itemid)
  {
    return 0;
  }

  // 增量更新已推荐新闻
  Status UserTable::updateCandidateSet(uint64_t user_id, const id_set_t& id_set)
  {
    if (!level_table_.find(user_id)) {
      std::ostringstream oss;

      // 点击了已淘汰的数据则不记录用户点击
      oss << "Obsolete user: " << std::hex << user_id;
      return Status::InvalidArgument(oss.str());
    } 
    CandidateUpdater updater(id_set);

    if (level_table_.update(user_id, updater)) {
      return Status::OK();
    }
    std::stringstream oss;

    oss << "Cann`t update user candidate set: user_id=" << user_id;
    return Status::InvalidArgument(oss.str());
  }

  bool UserTable::isObsoleteUser(const user_info_t* user_info, int32_t hold_time)
  {
    return false;
  }

  bool UserTable::isObsoleteUserInfo(const user_info_t* user_info, int32_t hold_time)
  {
    itemid_map_t::const_iterator iter = user_info->readed.begin();
    for (; iter != user_info->readed.end(); ++iter) {

    }

    iter = user_info->dislike.begin();
    for (; iter != user_info->dislike.end(); ++iter) {

    }

    iter = user_info->recommended.begin();
    for (; iter != user_info->recommended.end(); ++iter) {

    }
  }

  // 淘汰用户，包括已读，不喜欢和推荐信息
  Status UserTable::eliminate(int32_t hold_time)
  {
    // LevelTable<uint64_t, user_info_t>::Iterator 
    //   iter = level_table_.snapshot();

    // while (iter.hasNext()) {
    //   user_info_t* user_info = iter.value();

    //   if (isObsoleteUser(user_info, hold_time))
    //     level_table_.erase(iter.key());

    //   // 淘汰用户
    //   if (isObsoleteUserInfo(user_info, hold_time))  {
    //     EliminateUpdater updater;

    //     level_table_.update(iter.key(), updater);
    //   }
    //   iter.next();
    // }
    return Status::OK();
  }

  bool UserTable::findUser(uint64_t user_id)
  {
    return level_table_.find(user_id);
  }

  Status UserTable::updateUser(uint64_t user_id, user_info_t* user_info)
  {
    if (!level_table_.find(user_id)) {
      if (level_table_.add(user_id, user_info)) {
        return Status::OK();
      }
    } 
    UserUpdater updater(user_info);

    if (level_table_.update(user_id, updater)) {
      return Status::OK();
    }
    std::stringstream oss;

    oss << "Cann`t update user info: user_id=" << user_id;
    return Status::InvalidArgument(oss.str());
  }

  Status UserTable::queryHistory(uint64_t user_id, id_set_t& id_set)
  {
    if (!level_table_.find(user_id)) {
      std::ostringstream oss;

      oss << "Obsolete user: " << std::hex << user_id;
      return Status::InvalidArgument(oss.str());
    } 
    HistoryFetcher fetcher(id_set);

    if (level_table_.update(user_id, fetcher)) {
      return Status::OK();
    }
    std::stringstream oss;

    oss << "Cann`t fetch user history: user_id=" << user_id;
    return Status::InvalidArgument(oss.str());
  }

  Status UserTable::filterCandidateSet(uint64_t user_id, candidate_set_t& candset)
  {
    if (!level_table_.find(user_id)) {
      std::ostringstream oss;

      oss << "Obsolete user: " << std::hex << user_id;
      return Status::InvalidArgument(oss.str());
    } 
    CandidateFilter filter(candset);

    if (level_table_.update(user_id, filter)) {
      return Status::OK();
    }
    std::stringstream oss;

    oss << "Cann`t filter candidate set: user_id=" << user_id;
    return Status::InvalidArgument(oss.str());
  }
  }; // namespace news
}; // namespace rsys
