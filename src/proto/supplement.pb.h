// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: supplement.proto

#ifndef PROTOBUF_supplement_2eproto__INCLUDED
#define PROTOBUF_supplement_2eproto__INCLUDED

#include <string>

#include <google/protobuf/stubs/common.h>

#if GOOGLE_PROTOBUF_VERSION < 3000000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please update
#error your headers.
#endif
#if 3000000 < GOOGLE_PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/metadata.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/unknown_field_set.h>
#include "proto/message.pb.h"
// @@protoc_insertion_point(includes)

namespace module {
namespace protocol {

// Internal implementation detail -- do not call these.
void protobuf_AddDesc_supplement_2eproto();
void protobuf_AssignDesc_supplement_2eproto();
void protobuf_ShutdownFile_supplement_2eproto();

class ItemQuery;
class UserInfo;
class UserQuery;

// ===================================================================

class UserQuery : public ::google::protobuf::Message {
 public:
  UserQuery();
  virtual ~UserQuery();

  UserQuery(const UserQuery& from);

  inline UserQuery& operator=(const UserQuery& from) {
    CopyFrom(from);
    return *this;
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const UserQuery& default_instance();

  void Swap(UserQuery* other);

  // implements Message ----------------------------------------------

  inline UserQuery* New() const { return New(NULL); }

  UserQuery* New(::google::protobuf::Arena* arena) const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const UserQuery& from);
  void MergeFrom(const UserQuery& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  void InternalSwap(UserQuery* other);
  private:
  inline ::google::protobuf::Arena* GetArenaNoVirtual() const {
    return _internal_metadata_.arena();
  }
  inline void* MaybeArenaPtr() const {
    return _internal_metadata_.raw_arena_ptr();
  }
  public:

  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // optional fixed64 user_id = 1;
  void clear_user_id();
  static const int kUserIdFieldNumber = 1;
  ::google::protobuf::uint64 user_id() const;
  void set_user_id(::google::protobuf::uint64 value);

  // @@protoc_insertion_point(class_scope:module.protocol.UserQuery)
 private:

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  bool _is_default_instance_;
  ::google::protobuf::uint64 user_id_;
  mutable int _cached_size_;
  friend void  protobuf_AddDesc_supplement_2eproto();
  friend void protobuf_AssignDesc_supplement_2eproto();
  friend void protobuf_ShutdownFile_supplement_2eproto();

  void InitAsDefaultInstance();
  static UserQuery* default_instance_;
};
// -------------------------------------------------------------------

class UserInfo : public ::google::protobuf::Message {
 public:
  UserInfo();
  virtual ~UserInfo();

  UserInfo(const UserInfo& from);

  inline UserInfo& operator=(const UserInfo& from) {
    CopyFrom(from);
    return *this;
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const UserInfo& default_instance();

  void Swap(UserInfo* other);

  // implements Message ----------------------------------------------

  inline UserInfo* New() const { return New(NULL); }

  UserInfo* New(::google::protobuf::Arena* arena) const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const UserInfo& from);
  void MergeFrom(const UserInfo& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  void InternalSwap(UserInfo* other);
  private:
  inline ::google::protobuf::Arena* GetArenaNoVirtual() const {
    return _internal_metadata_.arena();
  }
  inline void* MaybeArenaPtr() const {
    return _internal_metadata_.raw_arena_ptr();
  }
  public:

  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // optional fixed64 user_id = 1;
  void clear_user_id();
  static const int kUserIdFieldNumber = 1;
  ::google::protobuf::uint64 user_id() const;
  void set_user_id(::google::protobuf::uint64 value);

  // optional int32 last_modified = 2;
  void clear_last_modified();
  static const int kLastModifiedFieldNumber = 2;
  ::google::protobuf::int32 last_modified() const;
  void set_last_modified(::google::protobuf::int32 value);

  // repeated .module.protocol.KeyStr subscribe = 3;
  int subscribe_size() const;
  void clear_subscribe();
  static const int kSubscribeFieldNumber = 3;
  const ::module::protocol::KeyStr& subscribe(int index) const;
  ::module::protocol::KeyStr* mutable_subscribe(int index);
  ::module::protocol::KeyStr* add_subscribe();
  ::google::protobuf::RepeatedPtrField< ::module::protocol::KeyStr >*
      mutable_subscribe();
  const ::google::protobuf::RepeatedPtrField< ::module::protocol::KeyStr >&
      subscribe() const;

  // repeated .module.protocol.KeyStr dislike = 4;
  int dislike_size() const;
  void clear_dislike();
  static const int kDislikeFieldNumber = 4;
  const ::module::protocol::KeyStr& dislike(int index) const;
  ::module::protocol::KeyStr* mutable_dislike(int index);
  ::module::protocol::KeyStr* add_dislike();
  ::google::protobuf::RepeatedPtrField< ::module::protocol::KeyStr >*
      mutable_dislike();
  const ::google::protobuf::RepeatedPtrField< ::module::protocol::KeyStr >&
      dislike() const;

  // repeated .module.protocol.KeyTime readed = 5;
  int readed_size() const;
  void clear_readed();
  static const int kReadedFieldNumber = 5;
  const ::module::protocol::KeyTime& readed(int index) const;
  ::module::protocol::KeyTime* mutable_readed(int index);
  ::module::protocol::KeyTime* add_readed();
  ::google::protobuf::RepeatedPtrField< ::module::protocol::KeyTime >*
      mutable_readed();
  const ::google::protobuf::RepeatedPtrField< ::module::protocol::KeyTime >&
      readed() const;

  // repeated .module.protocol.KeyTime recommended = 6;
  int recommended_size() const;
  void clear_recommended();
  static const int kRecommendedFieldNumber = 6;
  const ::module::protocol::KeyTime& recommended(int index) const;
  ::module::protocol::KeyTime* mutable_recommended(int index);
  ::module::protocol::KeyTime* add_recommended();
  ::google::protobuf::RepeatedPtrField< ::module::protocol::KeyTime >*
      mutable_recommended();
  const ::google::protobuf::RepeatedPtrField< ::module::protocol::KeyTime >&
      recommended() const;

  // @@protoc_insertion_point(class_scope:module.protocol.UserInfo)
 private:

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  bool _is_default_instance_;
  ::google::protobuf::uint64 user_id_;
  ::google::protobuf::RepeatedPtrField< ::module::protocol::KeyStr > subscribe_;
  ::google::protobuf::RepeatedPtrField< ::module::protocol::KeyStr > dislike_;
  ::google::protobuf::RepeatedPtrField< ::module::protocol::KeyTime > readed_;
  ::google::protobuf::RepeatedPtrField< ::module::protocol::KeyTime > recommended_;
  ::google::protobuf::int32 last_modified_;
  mutable int _cached_size_;
  friend void  protobuf_AddDesc_supplement_2eproto();
  friend void protobuf_AssignDesc_supplement_2eproto();
  friend void protobuf_ShutdownFile_supplement_2eproto();

  void InitAsDefaultInstance();
  static UserInfo* default_instance_;
};
// -------------------------------------------------------------------

class ItemQuery : public ::google::protobuf::Message {
 public:
  ItemQuery();
  virtual ~ItemQuery();

  ItemQuery(const ItemQuery& from);

  inline ItemQuery& operator=(const ItemQuery& from) {
    CopyFrom(from);
    return *this;
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const ItemQuery& default_instance();

  void Swap(ItemQuery* other);

  // implements Message ----------------------------------------------

  inline ItemQuery* New() const { return New(NULL); }

  ItemQuery* New(::google::protobuf::Arena* arena) const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const ItemQuery& from);
  void MergeFrom(const ItemQuery& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  void InternalSwap(ItemQuery* other);
  private:
  inline ::google::protobuf::Arena* GetArenaNoVirtual() const {
    return _internal_metadata_.arena();
  }
  inline void* MaybeArenaPtr() const {
    return _internal_metadata_.raw_arena_ptr();
  }
  public:

  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // optional fixed64 item_id = 1;
  void clear_item_id();
  static const int kItemIdFieldNumber = 1;
  ::google::protobuf::uint64 item_id() const;
  void set_item_id(::google::protobuf::uint64 value);

  // @@protoc_insertion_point(class_scope:module.protocol.ItemQuery)
 private:

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  bool _is_default_instance_;
  ::google::protobuf::uint64 item_id_;
  mutable int _cached_size_;
  friend void  protobuf_AddDesc_supplement_2eproto();
  friend void protobuf_AssignDesc_supplement_2eproto();
  friend void protobuf_ShutdownFile_supplement_2eproto();

  void InitAsDefaultInstance();
  static ItemQuery* default_instance_;
};
// ===================================================================


// ===================================================================

#if !PROTOBUF_INLINE_NOT_IN_HEADERS
// UserQuery

// optional fixed64 user_id = 1;
inline void UserQuery::clear_user_id() {
  user_id_ = GOOGLE_ULONGLONG(0);
}
inline ::google::protobuf::uint64 UserQuery::user_id() const {
  // @@protoc_insertion_point(field_get:module.protocol.UserQuery.user_id)
  return user_id_;
}
inline void UserQuery::set_user_id(::google::protobuf::uint64 value) {
  
  user_id_ = value;
  // @@protoc_insertion_point(field_set:module.protocol.UserQuery.user_id)
}

// -------------------------------------------------------------------

// UserInfo

// optional fixed64 user_id = 1;
inline void UserInfo::clear_user_id() {
  user_id_ = GOOGLE_ULONGLONG(0);
}
inline ::google::protobuf::uint64 UserInfo::user_id() const {
  // @@protoc_insertion_point(field_get:module.protocol.UserInfo.user_id)
  return user_id_;
}
inline void UserInfo::set_user_id(::google::protobuf::uint64 value) {
  
  user_id_ = value;
  // @@protoc_insertion_point(field_set:module.protocol.UserInfo.user_id)
}

// optional int32 last_modified = 2;
inline void UserInfo::clear_last_modified() {
  last_modified_ = 0;
}
inline ::google::protobuf::int32 UserInfo::last_modified() const {
  // @@protoc_insertion_point(field_get:module.protocol.UserInfo.last_modified)
  return last_modified_;
}
inline void UserInfo::set_last_modified(::google::protobuf::int32 value) {
  
  last_modified_ = value;
  // @@protoc_insertion_point(field_set:module.protocol.UserInfo.last_modified)
}

// repeated .module.protocol.KeyStr subscribe = 3;
inline int UserInfo::subscribe_size() const {
  return subscribe_.size();
}
inline void UserInfo::clear_subscribe() {
  subscribe_.Clear();
}
inline const ::module::protocol::KeyStr& UserInfo::subscribe(int index) const {
  // @@protoc_insertion_point(field_get:module.protocol.UserInfo.subscribe)
  return subscribe_.Get(index);
}
inline ::module::protocol::KeyStr* UserInfo::mutable_subscribe(int index) {
  // @@protoc_insertion_point(field_mutable:module.protocol.UserInfo.subscribe)
  return subscribe_.Mutable(index);
}
inline ::module::protocol::KeyStr* UserInfo::add_subscribe() {
  // @@protoc_insertion_point(field_add:module.protocol.UserInfo.subscribe)
  return subscribe_.Add();
}
inline ::google::protobuf::RepeatedPtrField< ::module::protocol::KeyStr >*
UserInfo::mutable_subscribe() {
  // @@protoc_insertion_point(field_mutable_list:module.protocol.UserInfo.subscribe)
  return &subscribe_;
}
inline const ::google::protobuf::RepeatedPtrField< ::module::protocol::KeyStr >&
UserInfo::subscribe() const {
  // @@protoc_insertion_point(field_list:module.protocol.UserInfo.subscribe)
  return subscribe_;
}

// repeated .module.protocol.KeyStr dislike = 4;
inline int UserInfo::dislike_size() const {
  return dislike_.size();
}
inline void UserInfo::clear_dislike() {
  dislike_.Clear();
}
inline const ::module::protocol::KeyStr& UserInfo::dislike(int index) const {
  // @@protoc_insertion_point(field_get:module.protocol.UserInfo.dislike)
  return dislike_.Get(index);
}
inline ::module::protocol::KeyStr* UserInfo::mutable_dislike(int index) {
  // @@protoc_insertion_point(field_mutable:module.protocol.UserInfo.dislike)
  return dislike_.Mutable(index);
}
inline ::module::protocol::KeyStr* UserInfo::add_dislike() {
  // @@protoc_insertion_point(field_add:module.protocol.UserInfo.dislike)
  return dislike_.Add();
}
inline ::google::protobuf::RepeatedPtrField< ::module::protocol::KeyStr >*
UserInfo::mutable_dislike() {
  // @@protoc_insertion_point(field_mutable_list:module.protocol.UserInfo.dislike)
  return &dislike_;
}
inline const ::google::protobuf::RepeatedPtrField< ::module::protocol::KeyStr >&
UserInfo::dislike() const {
  // @@protoc_insertion_point(field_list:module.protocol.UserInfo.dislike)
  return dislike_;
}

// repeated .module.protocol.KeyTime readed = 5;
inline int UserInfo::readed_size() const {
  return readed_.size();
}
inline void UserInfo::clear_readed() {
  readed_.Clear();
}
inline const ::module::protocol::KeyTime& UserInfo::readed(int index) const {
  // @@protoc_insertion_point(field_get:module.protocol.UserInfo.readed)
  return readed_.Get(index);
}
inline ::module::protocol::KeyTime* UserInfo::mutable_readed(int index) {
  // @@protoc_insertion_point(field_mutable:module.protocol.UserInfo.readed)
  return readed_.Mutable(index);
}
inline ::module::protocol::KeyTime* UserInfo::add_readed() {
  // @@protoc_insertion_point(field_add:module.protocol.UserInfo.readed)
  return readed_.Add();
}
inline ::google::protobuf::RepeatedPtrField< ::module::protocol::KeyTime >*
UserInfo::mutable_readed() {
  // @@protoc_insertion_point(field_mutable_list:module.protocol.UserInfo.readed)
  return &readed_;
}
inline const ::google::protobuf::RepeatedPtrField< ::module::protocol::KeyTime >&
UserInfo::readed() const {
  // @@protoc_insertion_point(field_list:module.protocol.UserInfo.readed)
  return readed_;
}

// repeated .module.protocol.KeyTime recommended = 6;
inline int UserInfo::recommended_size() const {
  return recommended_.size();
}
inline void UserInfo::clear_recommended() {
  recommended_.Clear();
}
inline const ::module::protocol::KeyTime& UserInfo::recommended(int index) const {
  // @@protoc_insertion_point(field_get:module.protocol.UserInfo.recommended)
  return recommended_.Get(index);
}
inline ::module::protocol::KeyTime* UserInfo::mutable_recommended(int index) {
  // @@protoc_insertion_point(field_mutable:module.protocol.UserInfo.recommended)
  return recommended_.Mutable(index);
}
inline ::module::protocol::KeyTime* UserInfo::add_recommended() {
  // @@protoc_insertion_point(field_add:module.protocol.UserInfo.recommended)
  return recommended_.Add();
}
inline ::google::protobuf::RepeatedPtrField< ::module::protocol::KeyTime >*
UserInfo::mutable_recommended() {
  // @@protoc_insertion_point(field_mutable_list:module.protocol.UserInfo.recommended)
  return &recommended_;
}
inline const ::google::protobuf::RepeatedPtrField< ::module::protocol::KeyTime >&
UserInfo::recommended() const {
  // @@protoc_insertion_point(field_list:module.protocol.UserInfo.recommended)
  return recommended_;
}

// -------------------------------------------------------------------

// ItemQuery

// optional fixed64 item_id = 1;
inline void ItemQuery::clear_item_id() {
  item_id_ = GOOGLE_ULONGLONG(0);
}
inline ::google::protobuf::uint64 ItemQuery::item_id() const {
  // @@protoc_insertion_point(field_get:module.protocol.ItemQuery.item_id)
  return item_id_;
}
inline void ItemQuery::set_item_id(::google::protobuf::uint64 value) {
  
  item_id_ = value;
  // @@protoc_insertion_point(field_set:module.protocol.ItemQuery.item_id)
}

#endif  // !PROTOBUF_INLINE_NOT_IN_HEADERS
// -------------------------------------------------------------------

// -------------------------------------------------------------------


// @@protoc_insertion_point(namespace_scope)

}  // namespace protocol
}  // namespace module

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_supplement_2eproto__INCLUDED
