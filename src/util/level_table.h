#ifndef RSYS_NEWS_LEVEL_TABLE_H
#define RSYS_NEWS_LEVEL_TABLE_H

#include "level_file.h"
#include "sparsehash/dense_hash_map"

namespace rsys {
  namespace news {
    template<typename K, typename V>
      class LevelTable {
        public:
          typedef K key_t;
          typedef V value_t;

          enum mode_ {
            MODE_NONE = 0x00,
            MODE_DELETED = 0x01
          };
          typedef enum mode_ mode_t;
#pragma pack(1)
          struct node_ {
            uint8_t mode;
            value_t* value;
          };
          typedef struct node_ node_t;
#pragma pack()
          typedef google::dense_hash_map<key_t, node_t> hash_map_t;
          typedef hash_map_t* hash_map_ptr_t;

          class Updater {
            public:
              Updater() {};
              virtual ~Updater() {};
              virtual bool update(value_t* value) = 0;
              virtual value_t* clone(value_t* value) = 0;
          };

          class Iterator {
            public:
              Iterator(hash_map_ptr_t hash_map, bool valid=true)
                : hash_map_(hash_map), valid_(valid) {
                  if (hash_map) iter_ = hash_map->begin();
                };
              ~Iterator() {};

            public:
              bool valid() { return valid_; }

            public:
              const key_t key();
              const value_t* value();

              bool hasNext();
              void next();

            private:
              bool valid_;
              hash_map_ptr_t hash_map_;
              typename hash_map_t::iterator iter_;
         };

        public:
          // level层级必须大于３, 小于３时默认为３
          LevelTable(){};//size_t level);
          virtual ~LevelTable() {
          }

        public:
          bool add(const key_t& key, value_t* value);
          bool find(const key_t& key);

          bool update(const key_t& key, Updater& updater);
          bool erase(const key_t& key);

        public:
          // 注：deepen/depth/merge/snapshot有限支持线程安全
          // 其可与add/find/update/erase并行执行，但其之间不可并行运行
         
          // 增加层级
          void deepen();

          // 返回当前层级表的深度
          size_t depth();

          // 合并Ln->L1不可变库
          bool merge();

          // 获取访问不可变库迭代器, 有限支持线程安全
          Iterator snapshot();

        private:
          size_t level_size_;
          hash_map_ptr_t* level_map_;
          pthread_rwlock_t* level_lock_;
      };
  }; // namespace news
}; // namespace rsys

#include "util/level_table.inl"
#endif // #define RSYS_NEWS_LEVEL_TABLE_H

