#include "core/options.h"
#include <sstream>
#include "libconfig.hh"

namespace rsys {
  namespace news {
    using namespace libconfig;
    Status Options::fromConf(const std::string& conf, Options& opts)
    {
      Config parser;

      try {
        parser.readFile(conf.c_str());
      }
      catch(const FileIOException &oex) {
        std::stringstream ss;
        ss << "read file " << conf << " failed: " << oex.what();
        return Status::IOError(ss.str());
      }
      catch(const ParseException &pex) {
        std::stringstream ss;
        ss << "parse error at " << pex.getFile() << ":" << pex.getLine()
          << " - " << pex.getError();
        return Status::InvalidArgument(ss.str());
      }
      // parser.lookupValue();
      return Status::OK();
    }
  } // namespace news
} // namespace rsys
