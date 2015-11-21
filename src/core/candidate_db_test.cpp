#include "catch.hpp"
#include "core/options.h"
#include "core/candidate_db.h"

using namespace rsys::news;
SCENARIO("CandidateDB测试", "[base]") {
  GIVEN("创建空的CandidateDB") {
    Options options;
    CandidateDB* candb = NULL;

    WHEN("openDB") {
      Status status = CandidateDB::openDB(options, &candb);
      THEN("CandidateDB创建成功") {
        REQUIRE(status.ok());
      }
    }
  }
}

