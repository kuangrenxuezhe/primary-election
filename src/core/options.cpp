#include "core/options.h"
#include "libconfig.hh"

namespace souyue {
  namespace recmd {
    using namespace libconfig;

    Options::Options()
    {
      rpc_port = 6200;
      monitor_port = 16200;
      work_path = ".";
      table_name = "level_table";
      item_hold_time = 7 * 24 * 60 * 60;
      user_hold_time = 3 * 7 * 24 * 60 * 60;
      max_table_level = 3;
      new_item_max_age = 2 * 24 * 60 * 60;
      interval_recommendation = 1 * 24 * 60 * 60;
      max_candidate_set_size = 5000;
      top_item_max_age = 1 * 24 * 60 * 60;
      flush_timer = "23/day";
      max_candidate_video_size = 10;
      max_candidate_region_size = 10;
      service_type = 0;
    }

    Status Options::fromConf(const std::string& conf, Options& opts)
    {
      Config parser;

      try {
        parser.readFile(conf.c_str());

        if (!parser.lookupValue("rpc_port", opts.rpc_port))
          opts.rpc_port = 6200;
        if (!parser.lookupValue("monitor_port", opts.monitor_port))
          opts.monitor_port = 16200;
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
        if (!parser.lookupValue("interval_recommendation", opts.interval_recommendation))
          opts.interval_recommendation = 1 * 24 * 60 * 60;
        if (!parser.lookupValue("max_candidate_set_size", opts.max_candidate_set_size))
          opts.max_candidate_set_size = 5000;
        if (!parser.lookupValue("max_candidate_video_size", opts.max_candidate_video_size))
          opts.max_candidate_video_size = 10;
        if (!parser.lookupValue("max_candidate_region_size", opts.max_candidate_region_size))
          opts.max_candidate_region_size = 10;
        if (!parser.lookupValue("top_item_max_age", opts.top_item_max_age))
          opts.top_item_max_age = 1 * 24 * 60 * 60;
        if (!parser.lookupValue("flush_timer", opts.flush_timer))
          opts.flush_timer = "23/day";
        if (!parser.lookupValue("service_type", opts.service_type))
          opts.service_type = 0;
      }
      catch(const FileIOException &oex) {
        return Status::IOError(oex.what(), ", file=", conf);
      }
      catch(const ParseException &pex) {
        return Status::Corruption(pex.getError(), ", at ", conf, ":", pex.getLine());
      }
      return Status::OK();
    }
  } // namespace recmd 
} // namespace souyue
