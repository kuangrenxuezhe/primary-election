#include "core/options.h"

#include <sstream>
#include "libconfig.hh"

namespace rsys {
  namespace news {
    using namespace libconfig;

    Options::Options()
    {
      work_path = ".";
      table_name = "level_table";
      item_hold_time = 7 * 24 * 60 * 60;
      user_hold_time = 3 * 7 * 24 * 60 * 60;
      max_table_level = 3;
      new_item_max_age = 2 * 24 * 60 * 60;
      log_expired_days = 7 * 24 * 60 * 60; 
      flush_timer = "23/day";
    }

    Status Options::fromConf(const std::string& conf, Options& opts)
    {
      Config parser;

      try {
        parser.readFile(conf.c_str());
        if (!parser.lookupValue("work_path", opts.work_path))
          opts.work_path = ".";
        if (!parser.lookupValue("table_name", opts.table_name))
          opts.table_name = "level_table";
        if (!parser.lookupValue("item_hold_time", opts.item_hold_time))
          opts.item_hold_time = 24 * 60 * 60;
        if (!parser.lookupValue("user_hold_time", opts.user_hold_time))
          opts.user_hold_time = 3 * 7 * 24 * 60 * 60;
        if (!parser.lookupValue("max_table_level", opts.max_table_level))
          opts.max_table_level = 3;
        if (!parser.lookupValue("new_item_max_age", opts.new_item_max_age))
          opts.new_item_max_age = 2 * 24 * 60 * 60;
        if (!parser.lookupValue("log_expired_days", opts.log_expired_days))
          opts.log_expired_days = 7 * 24 * 60 * 60;
        if (!parser.lookupValue("flush_timer", opts.flush_timer))
          opts.flush_timer = "23/day";
      }
      catch(const FileIOException &oex) {
        std::ostringstream oss;
        oss<<oex.what()<<", file="<<conf;
        return Status::IOError(oss.str());
      }
      catch(const ParseException &pex) {
        std::ostringstream oss;
        oss<<pex.getError()<<", at "<<conf<<":"<<pex.getLine();
        return Status::InvalidArgument(oss.str());
      }
      return Status::OK();
    }
  } // namespace news
} // namespace rsys
