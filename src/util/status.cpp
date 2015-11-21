#include "util/status.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

namespace rsys {
  namespace news {
    const char* Status::copyState(const char* state) {
      uint32_t size;
      memcpy(&size, state, sizeof(size));

      char* result = new char[size + 5];
      memcpy(result, state, size + 5);
      return result;
    }

    Status::Status(Code code, const std::string& msg) {
      assert(code != kOk);
      const uint32_t size = msg.size();
      char* result = new char[size + 5];
      memcpy(result, &size, sizeof(size));
      result[4] = static_cast<char>(code);
      memcpy(result + 5, msg.data(), msg.length());
      state_ = result;
    }

    std::string Status::toString() const {
      if (state_ == NULL) {
        return "OK";
      }

      char tmp[30];
      const char* type;

      switch (code()) {
        case kOk:
          type = "OK";
          break;
        case kNotFound:
          type = "Not found: ";
          break;
        case kCorruption:
          type = "Corruption: ";
          break;
        case kNotSupported:
          type = "Not implemented: ";
          break;
        case kInvalidArgument:
          type = "Invalid argument: ";
          break;
        case kIOError:
          type = "IO error: ";
          break;
        default:
          snprintf(tmp, sizeof(tmp), "Unknown code(%d): ",
              static_cast<int>(code()));
          type = tmp;
          break;
      }
      std::string result(type);
      uint32_t length;

      memcpy(&length, state_, sizeof(length));
      result.append(state_ + 5, length);
      return result;
    }
  } // namespace news
} // namespace rsys
