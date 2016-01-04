#include "core/model_options.h"
#include "libconfig.hh"

namespace souyue {
  namespace recmd {
    using namespace libconfig;

    ModelOptions::ModelOptions()
    {
      rpc_port = 6200;
      monitor_port = 16200;
      table_name = "level_table";
      interval_recommendation = 1 * 24 * 60 * 60;
      profile_expired_time = 1 * 24 * 60 * 60;
      max_candidate_set_size = 5000;
      flush_timer = "23/day";
      max_candidate_video_size = 10;
      max_candidate_region_size = 10;
      service_type = 0;
    }

    Status ModelOptions::fromConf(const std::string& conf, ModelOptions& opts)
    {
      Config parser;

      Status status = Options::fromConf(conf, opts);
      if (!status.ok()) {
        return status;
      }

      try {
        parser.readFile(conf.c_str());

        if (!parser.lookupValue("rpc_port", opts.rpc_port))
          opts.rpc_port = 6200;
        if (!parser.lookupValue("monitor_port", opts.monitor_port))
          opts.monitor_port = 16200;
        if (!parser.lookupValue("table_name", opts.table_name))
          opts.table_name = "level_table";
        if (!parser.lookupValue("max_table_level", opts.max_table_level))
          opts.max_table_level = 3;
        if (!parser.lookupValue("interval_recommendation", opts.interval_recommendation))
          opts.interval_recommendation = 1 * 24 * 60 * 60;
        if (!parser.lookupValue("profile_expired_time", opts.profile_expired_time))
          opts.profile_expired_time = 1 * 24 * 60 * 60;
        if (!parser.lookupValue("max_candidate_set_size", opts.max_candidate_set_size))
          opts.max_candidate_set_size = 5000;
        if (!parser.lookupValue("max_candidate_video_size", opts.max_candidate_video_size))
          opts.max_candidate_video_size = 10;
        if (!parser.lookupValue("max_candidate_region_size", opts.max_candidate_region_size))
          opts.max_candidate_region_size = 10;
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
