#define CATCH_CONFIG_RUNNER
#include "catch.hpp"
#include <string.h>
#include <iostream>

int main( int argc, char* const argv[] )
{
  char p[] = "5_后";
  char* p1 = strchr(p, '_');
  fprintf(stdout, "strtok: %s", p1);
  return Catch::Session().run(argc, argv);
}

//#include "glog/logging.h"
//#include "util/table_base.h"
//
//using namespace rsys::news;
//class MyAheadLog;
//class MyTableBase: public TableBase<uint64_t> {
//  public:
//    MyTableBase(const std::string& path, const std::string& name, const fver_t& ver, size_t level)
//      : TableBase(path, name, ver, level) {
//    }
//    virtual ~MyTableBase() {
//    }
//
//  public:
//    bool deepenTable() {
//      return level_table()->deepen();
//    }
//    bool addKeyValue(uint64_t key, uint64_t* value) {
//      return level_table()->add(key, value);
//    }
//    // 创建value对象 
//    virtual uint64_t* newValue() {
//      return new uint64_t;
//    }
//    // 创建AheadLog对象
//    virtual AheadLog* createAheadLog();
//  public:
//    // 序列化和反序列话对象
//    virtual bool parseFrom(const std::string& data, uint64_t* key, uint64_t* value) {
//      const char* p = data.c_str();
//      char* pend = NULL;
//
//      *key = strtoul(p, &pend, 10);
//      pend++;
//      p = pend;
//      *value  = strtoul(pend, NULL, 10);
//      return true;
//    }
//    virtual bool serializeTo(uint64_t key, const uint64_t* value,  std::string& data) {
//      std::ostringstream oss;
//      oss<<key<<"|"<<*value;
//      data = oss.str();
//      return true;
//    }
//};
//
//class MyAheadLog: public AheadLog {
//  public:
//    MyAheadLog(MyTableBase* table, const std::string& path, const fver_t& fver)
//      : AheadLog(path, fver) {
//        table_ = table;
//      }
//    virtual ~MyAheadLog() {
//    }
//    // ahead log滚存触发器
//    virtual bool trigger() {
//      return table_->deepenTable();
//    }
//    // 处理数据回滚
//    virtual bool rollback(const std::string& data) {
//      uint64_t key;
//      uint64_t* value = new uint64_t;
//
//      if (!table_->parseFrom(data, &key, value))
//        return false;
//
//      return table_->addKeyValue(key, value);
//    }
//  public:
//    MyTableBase* table_;
//};
//
//inline AheadLog* MyTableBase::createAheadLog()
//{
//  fver_t fver(1, 8);
//  return new MyAheadLog(this, ".", fver);
//}
//
//int main() 
//{
//    fver_t fver(1,8);
//    MyTableBase table(".", "test_table_base", fver, 3);
//
//    Status status = table.loadTable();
//
////  Options options;
////  options.work_path = "./data";
////  options.table_name = "test";
////  options.max_table_level = 3;
////  CandidateDB* candb = NULL;
////  Status status = CandidateDB::openDB(options, &candb);
////  if (!status.ok()) {
////    LOG(ERROR) << status.toString();
////  } else {
////    if (!candb->findUser(1)) {
////      LOG(INFO) << "user 1 not found";
////    }
////    ItemInfo item;
////
////    item.set_item_id(10);
////    item.set_publish_time(time(NULL));
////    status = candb->addItem(item);
////    if (!status.ok())
////      LOG(ERROR) << status.toString();
////
////    status = candb->flush();
////    if (!status.ok()) {
////      LOG(ERROR) << status.toString();
////    }
////  }
////
////  return 0;
//}
