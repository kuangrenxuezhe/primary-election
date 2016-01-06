#include <string>
#include "utils/wal.h"
#include "utils/char_conv.h"
#include "gflags/gflags.h"
#include "proto/message.pb.h"

DEFINE_string(type, "none", "WAL type: item, user, default: none");
DEFINE_int32(monitor_port, -1, "Monitor port");

using namespace souyue::recmd;
using namespace module::protocol;

void parse_data(const std::string& data);
int main(int argc, char* argv[]) 
{
  gflags::SetUsageMessage("");
  int nflags = gflags::ParseCommandLineFlags(&argc, &argv, false);
  for (int i = nflags; i < argc; ++i) {
    std::string name(argv[i]);
    WALReader reader(name);
    fver_t fver;
    Status status = reader.open(fver);
    if (!status.ok()) {
      fprintf(stderr, "%s\n", status.toString().c_str());
      continue;
    }
    std::string data;

    for(;;) {
      status = reader.read(data);
      if (!status.ok())
        break;
      if (data.length() <= 0)
        break;
      parse_data(data);
    }
    reader.close();
  }
  gflags::ShutDownCommandLineFlags();
  return 0;
}

void parse_item_data(const std::string& data)
{
  static const char kLogTypeItem   = 'I';
  static const char kLogTypeAction = 'A';
  static const char* kActionStr[] = {
    "NONE",
    "CLICK",
    "LIKE",
    "COMMENT",
    "FAVORITE",
    "SHARE",
    "DISLIKE",
    "CLOSE_PAGE"
  };
  static const char* kItemType[] = {
    "NONE",
    "NEWS",
    "VIDEO",
    "CIRCLE"
  };

  const char* c_data = data.c_str();
  if (kLogTypeAction == data[0]) {
    Action log_action;

    if (!log_action.ParseFromArray(c_data + 1, data.length() - 1)) {
      fprintf(stderr, "Parse item action\n");
    } else {
      fprintf(stdout, "uid=%llu itemid=%llu action=%s zone=%s srp=%s dislike=%s source=%s\n", 
          log_action.user_id(), log_action.item_id(), kActionStr[log_action.action()], 
          toUTF8(log_action.zone()).c_str(), log_action.srp_id().c_str(), 
          toUTF8(log_action.dislike()).c_str(), log_action.click_source() ? "推荐频道":"其他频道");
    }
  } else if (kLogTypeItem == data[0]) {
    Item log_item;

    if (!log_item.ParseFromArray(c_data + 1, data.length() -1 )) {
      fprintf(stderr, "Parse item\n");
    } else {
      std::ostringstream oss;
      for (int i = 0; i < log_item.category_size(); i++) {
        oss << log_item.category(i).tag_id() << ":" << toUTF8(log_item.category(i).tag_name()) << ";";
      }
      std::string category = oss.str();

      oss.str("");
      for (int i = 0; i < log_item.srp_size(); i++) {
        oss << log_item.srp(i).tag_name() << ";";
      }
      std::string srp = oss.str();

      oss.str("");
      for (int i = 0; i < log_item.circle_size(); i++) {
        oss << log_item.circle(i).tag_name() << ";";
      }
      std::string circle = oss.str();

      oss.str("");
      for (int i = 0; i < log_item.zone_size(); i++) {
        oss << toUTF8(log_item.zone(i)) << ";";
      }
      std::string zone = oss.str();

      oss.str("");
      for (int i = 0; i < log_item.tag_size(); i++) {
        oss << log_item.tag(i).tag_id() << ":" << toUTF8(log_item.tag(i).tag_name()) << ";";
      }
      std::string tag = oss.str();

      fprintf(stdout, "itemid=%llu type=%s category=%s srp=%s circle=%s zone=%s source=%s tag=%s\n",
          log_item.item_id(), kItemType[log_item.item_type()], category.c_str(), srp.c_str(), 
          circle.c_str(), zone.c_str(), toUTF8(log_item.data_source().source_name().c_str()).c_str(), tag.c_str());
    }
  }
}

void parse_user_data(const std::string& data)
{
  static const char kLogTypeAction     = 'A';
  static const char kLogTypeSubscribe  = 'S';
  static const char kLogTypeFeedback   = 'F';
  static const char kLogTypeFieldByKey = 'D';
  static const char* kActionStr[] = {
    "NONE",
    "CLICK",
    "LIKE",
    "COMMENT",
    "FAVORITE",
    "SHARE",
    "DISLIKE",
    "CLOSE_PAGE"
  };
 
  const char* c_data = data.c_str();
  if (kLogTypeAction == data[0]) {
    Action log_action;

    if (!log_action.ParseFromArray(c_data + 1, data.length() - 1)) {
      fprintf(stderr, "Parse user action\n");
    } else {
      fprintf(stdout, "uid=%llu itemid=%llu action=%s zone=%s srp=%s dislike=%s source=%s\n", 
          log_action.user_id(), log_action.item_id(), kActionStr[log_action.action()], 
          toUTF8(log_action.zone()).c_str(), log_action.srp_id().c_str(), 
          toUTF8(log_action.dislike()).c_str(), log_action.click_source() ? "推荐频道":"其他频道");
    }
  } else if (kLogTypeSubscribe == data[0]) {
    Subscribe log_subscribe;

    if (!log_subscribe.ParseFromArray(c_data + 1, data.length() - 1)) {
      fprintf(stderr, "Parse user subscribe\n");
    } else {
      std::ostringstream oss;
      for (int i = 0; i < log_subscribe.srp_id_size(); i++) {
        oss << log_subscribe.srp_id(i) << ";";
      }
      std::string srp = oss.str();

      oss.str("");
      for (int i = 0; i < log_subscribe.circle_id_size(); i++) {
        oss << log_subscribe.circle_id(i) << ";";
      }
      std::string circle = oss.str();

      fprintf(stdout, "uid=%llu srp=%s circle=%s\n", log_subscribe.user_id(), srp.c_str(), circle.c_str());
    }
  } else if (kLogTypeFeedback == data[0]) {
    Feedback log_feedback;

    if (!log_feedback.ParseFromArray(c_data + 1, data.length() - 1)) {
      fprintf(stderr, "Parse recommend feedback\n");
    } else {
      std::ostringstream oss;
      for (int i = 0; i < log_feedback.item_id_size(); i++) {
        oss << log_feedback.item_id(i) << ";";
      }
      std::string item_id = oss.str();
      fprintf(stdout, "uid=%llu itemid=%s\n", log_feedback.user_id(), item_id.c_str());
    }
  } else if (kLogTypeFieldByKey == data[0]) {
    UserProfileFieldKey log_field_key;

    if (!log_field_key.ParseFromArray(c_data + 1, data.length() - 1)) {
      fprintf(stderr, "Parse user profile field key\n");
    } else {
    }
  }
}

void parse_data(const std::string& data)
{
  if (FLAGS_type == "item")
    parse_item_data(data);
  else if (FLAGS_type == "user") 
    parse_user_data(data);
}

