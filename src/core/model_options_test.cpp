#include "catch.hpp"
#include <string.h>
#include "core/model_options.h"

using namespace souyue::recmd;
TEST_CASE("ModelOptions", "[base]") {
  FILE* wfd = fopen("test.conf", "w");
  if (!wfd)
    FAIL(strerror(errno));

  // options
  fprintf(wfd, "rpc_port: 43221\n");
  fprintf(wfd, "monitor_port: 766444\n");
  fprintf(wfd, "work_path: \"mypath\"\n");
  fprintf(wfd, "table_name: \"faefth\"\n");
  fprintf(wfd, "item_hold_time: 11222\n");
  fprintf(wfd, "user_hold_time: 22233\n");
  fprintf(wfd, "max_table_level: 23\n");
  fprintf(wfd, "new_item_max_age: 333\n");
  fprintf(wfd, "interval_recommendation: 24444\n");
  fprintf(wfd, "profile_expired_time: 477644\n");
  fprintf(wfd, "top_item_max_age: 2323\n");
  fprintf(wfd, "max_candidate_set_size: 98789\n");
  fprintf(wfd, "max_candidate_video_size: 73642\n");
  fprintf(wfd, "max_candidate_region_size: 563523\n");
  fprintf(wfd, "flush_timer: \"fffff\"\n");
  fprintf(wfd, "service_type: 83743\n");
  fclose(wfd);

  ModelOptions opts;
  Status status = ModelOptions::fromConf("test.conf", opts);
  if (!status.ok()) {
    FAIL(status.toString());
  }
  CHECK(opts.rpc_port == 43221);
  CHECK(opts.monitor_port == 766444);
  CHECK(opts.work_path == "mypath");
  CHECK(opts.table_name == "faefth");
  CHECK(opts.item_hold_time == 11222);
  CHECK(opts.user_hold_time == 22233);
  CHECK(opts.max_table_level == 23);
  CHECK(opts.new_item_max_age == 333);
  CHECK(opts.interval_recommendation == 24444);
  CHECK(opts.profile_expired_time == 477644);
  CHECK(opts.top_item_max_age == 2323);
  CHECK(opts.max_candidate_set_size == 98789);
  CHECK(opts.max_candidate_video_size == 73642);
  CHECK(opts.max_candidate_region_size == 563523);
  CHECK(opts.flush_timer == "fffff");
  CHECK(opts.service_type == 83743);
  CHECK(opts.top_item_max_age == 2323);

  remove("test.conf");
}

