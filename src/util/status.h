#ifndef RSYS_NEWS_STATUS_H
#define RSYS_NEWS_STATUS_H

#include <string>
#include <iostream>

namespace rsys {
  namespace news {
    class Status {
      public:
        Status() : state_(NULL) {
        }
        ~Status() {
          delete[] state_;
        }
        Status(const Status& s);
        void operator=(const Status& s);

        // Return error status of an appropriate type.
        static Status OK() {
          return Status(); 
        }
        static Status IOError(const std::string& msg) {
          return Status(kIOError, msg);
        }
        static Status Corruption(const std::string& msg) {
          return Status(kCorruption, msg);
        }
        static Status NotFound(const std::string& msg) {
          return Status(kNotFound, msg);
        }
        static Status NotSupported(const std::string& msg) {
          return Status(kNotSupported, msg);
        }
        static Status InvalidArgument(const std::string& msg) {
          return Status(kInvalidArgument, msg);
        }
        static Status InvalidData(const std::string& msg) {
          return Status(kInvalidData, msg);
        }

        // Returns true iff the status indicates an appropriate type.
        bool ok() const { 
          return (state_ == NULL);
        }
        bool isIOError() const {
          return code() == kIOError; 
        }
        bool isCorruption() const {
          return code() == kCorruption; 
        }
        bool isNotFound() const {
          return code() == kNotFound; 
        }
        bool isNotSupported() const {
          return code() == kNotSupported;
        }
        bool isInvalidArgument() const {
          return code() == kInvalidArgument; 
        }
        bool isInvalidData() const {
          return code() == kInvalidData; 
        }

        // Return a string representation of this status suitable for printing.
        // Returns the string "OK" for success.
        std::string toString() const;

      private:
        // OK status has a NULL state_.  Otherwise, state_ is a new[] array
        // of the following form:
        //    state_[0..3] == length of message
        //    state_[4]    == code
        //    state_[5..]  == message
        const char* state_;

        enum Code {
          kOk = 0,
          kIOError = 1,
          kCorruption = 2,
          kNotFound = 3,
          kNotSupported = 4,
          kInvalidArgument = 5,
          kInvalidData = 6
        };

        Code code() const {
          return (state_ == NULL) ? kOk : static_cast<Code>(state_[4]);
        }

        Status(Code code, const std::string& msg);
        static const char* copyState(const char* s);
    };

    inline Status::Status(const Status& s) {
      state_ = (s.state_ == NULL) ? NULL : copyState(s.state_);
    }
    inline void Status::operator=(const Status& s) {
      // The following condition catches both aliasing (when this == &s),
      // and the common case where both s and *this are ok.
      if (state_ != s.state_) {
        delete[] state_;
        state_ = (s.state_ == NULL) ? NULL : copyState(s.state_);
      }
    }
  } // namespace news
} // namespace rsys 

#endif  // #define RSYS_NEWS_STATUS_H

