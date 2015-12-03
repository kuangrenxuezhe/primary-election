#ifndef RSYS_NEWS_CANDIDATE_DB_H
#define RSYS_NEWS_CANDIDATE_DB_H

#include "status.h"
#include "core/options.h"
#include "core/user_table.h"
#include "core/item_table.h"
#include "proto/record.pb.h"
#include "proto/service.pb.h"

namespace rsys {
  namespace news {
    class CandidateDB {
      public:
        CandidateDB(const Options& opts);
        ~CandidateDB();

      public:
        const Options& options() const { return options_; }
        static Status openDB(const Options& opts, CandidateDB** dbptr);

      public:
        // 可异步方式，线程安全
        Status flush();
        Status reload();

      public:
        // 查询用户是否存在
        Status findUser(const User& user);
        // 添加新闻数据
        Status addItem(const Item& item);
        // 全局更新用户订阅信息
        Status updateSubscribe(const Subscribe& subscribe);
        // 更新用户候选集合
        Status updateFeedback(const Feedback& feedback);
        // 用户操作状态更新
        Status updateAction(const Action& action, Action& updated);
        // 获取待推荐的候选集
        Status queryCandidateSet(const Recommend& query, CandidateSet& candidate_set);

      public:
        // 查询用户是否在用户表中
        Status queryItemInfo(const proto::ItemQuery& query, proto::ItemInfo& item_info);
        // 查询用户是否在用户表中
        Status queryUserInfo(const proto::UserQuery& query, proto::UserInfo& user_info);

      protected:
        Status lock(); // 单进程锁定

      private:
        Options       options_;
        FileWriter  singleton_; // 保证一个库目录只能一个进程打开
        UserTable* user_table_;
        ItemTable* item_table_;
    };
  }; // namespace news
}; // namespace rsys
#endif // #define RSYS_NEWS_CANDIDATE_DB_H
