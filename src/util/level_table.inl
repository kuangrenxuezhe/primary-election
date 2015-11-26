#include "util/level_table.h"

namespace rsys {
  namespace news {
    static const int32_t kMinLevel = 3;

    template<typename K, typename V>
      inline const typename LevelTable<K, V>::key_t LevelTable<K, V>::Iterator::key()
      {
        return iter_->first;
      }

    template<typename K, typename V>
      inline const typename LevelTable<K, V>::value_t* LevelTable<K, V>::Iterator::value()
      {
        return iter_->second.mode&MODE_DELETED ? NULL:iter_->second.value;
      }

    template<typename K, typename V>
      inline bool LevelTable<K, V>::Iterator::hasNext()
      {
        return iter_ == hash_map_->end() ? false:true;
      }

    template<typename K, typename V>
      inline void LevelTable<K, V>::Iterator::next()
      {
        ++iter_;
      }

    // level层级必须大于３, 小于３时默认为３
    template<typename K, typename V>
      LevelTable<K, V>::LevelTable(size_t level)
        : level_size_(level), level_map_(NULL), level_lock_(NULL) {
          level_map_ = new hash_map_ptr_t[level];
          level_lock_ = new pthread_rwlock_t[level];
          for (size_t i = 0; i < level; ++i) {
            level_map_[i] = NULL;
            if (0 == i) {
              level_map_[0] = new hash_map_t();
              level_map_[0]->set_empty_key(0);
            }
            pthread_rwlock_init(&level_lock_[i], NULL);
          }
      }

    template<typename K, typename V>
      LevelTable<K, V>::~LevelTable() {
        for (size_t i = 0; i < level_size_; ++i) {
          if (level_map_[i])
            delete level_map_[i];
        }
        delete [] level_map_;

        for (size_t i = 0; i < level_size_; ++i) {
          pthread_rwlock_destroy(&level_lock_[i]);
        }
        delete [] level_lock_;
      }

    template<typename K, typename V>
      inline size_t LevelTable<K, V>::depth() {
        size_t depth = 0;

        for (size_t i = 0; i < level_size_; ++i) {
          if (NULL == level_map_[i])
            break;
          depth++;
        }
        return depth;
      }

    template<typename K, typename V>
      inline bool LevelTable<K, V>::add(const key_t& key, value_t* value) {
        bool added = false;

        pthread_rwlock_wrlock(&level_lock_[0]);
        typename hash_map_t::iterator iter = level_map_[0]->find(key);

        if (iter != level_map_[0]->end()) {
          added = true;
          if (iter->second.value)
            delete iter->second.value;
          iter->second.value = value;
        } else {
          node_t node;

          node.mode = MODE_NONE;
          node.value = value;

          typename std::pair<typename hash_map_t::iterator, bool> inserted = 
            level_map_[0]->insert(std::make_pair(key, node));
          added = inserted.second;
        } 
        pthread_rwlock_unlock(&level_lock_[0]);
        return added;
      }

    template<typename K, typename V>
      inline bool LevelTable<K, V>::find(const key_t& key) {
        for (size_t i = 0; i < level_size_; ++i) {
          pthread_rwlock_rdlock(&level_lock_[i]);
          if (NULL == level_map_[i]) {
            pthread_rwlock_unlock(&level_lock_[i]);
            break;
          }
          typename hash_map_t::iterator iter = level_map_[i]->find(key);

          if (iter != level_map_[i]->end()) {
            uint8_t mode = iter->second.mode;

            pthread_rwlock_unlock(&level_lock_[i]);
            // 判定是否删除
            return mode&MODE_DELETED ? false:true;
          }
          pthread_rwlock_unlock(&level_lock_[i]);
        }
        return false;
      }

    template<typename K, typename V>
      inline bool LevelTable<K, V>::get(const key_t& key, Getter& getter) {
        bool finded = false;

        pthread_rwlock_rdlock(&level_lock_[0]);
        typename hash_map_t::iterator iter = level_map_[0]->find(key);

        if (iter != level_map_[0]->end()) {
          bool copied = getter.copy(iter->second.value);

          finded = true;
          pthread_rwlock_rdlock(&level_lock_[0]);

          return copied;
        } 
        pthread_rwlock_rdlock(&level_lock_[0]);

        if (!finded) {
          for (size_t i = 1; i < level_size_; ++i) {
            pthread_rwlock_rdlock(&level_lock_[i]);
            if (NULL == level_map_[i]) {
              pthread_rwlock_unlock(&level_lock_[i]);
              break;
            }
            iter = level_map_[i]->find(key);

            if (iter != level_map_[i]->end()) {
              uint8_t mode = iter->second.mode;

              if (!(mode&MODE_DELETED) && getter.copy(iter->second.value)) {
                finded=true;
              }
              pthread_rwlock_unlock(&level_lock_[i]);
              break;
            }
            pthread_rwlock_unlock(&level_lock_[i]);
          }
        }
        return finded;
      }

    template<typename K, typename V>
      inline bool LevelTable<K, V>::update(const key_t& key, Updater& updater) {
        bool updated = false;

        pthread_rwlock_wrlock(&level_lock_[0]);
        typename hash_map_t::iterator iter = level_map_[0]->find(key);

        if (iter != level_map_[0]->end()) {
          updated = updater.update(iter->second.value);
        } else {
          bool finded = false;

          for (size_t i = 1; i < level_size_; ++i) {
            pthread_rwlock_rdlock(&level_lock_[i]);
            if (NULL == level_map_[i]) {
              pthread_rwlock_unlock(&level_lock_[i]);
              break;
            }
            iter = level_map_[i]->find(key);

            if (iter != level_map_[i]->end()) {
              uint8_t mode = iter->second.mode;

              pthread_rwlock_unlock(&level_lock_[i]);
              mode&MODE_DELETED ? finded = false:finded = true;
              break;
            }
            pthread_rwlock_unlock(&level_lock_[i]);
          }

          if (finded) {
            node_t node;

            node.mode = MODE_NONE;
            node.value = updater.clone(iter->second.value);

            typename std::pair<typename hash_map_t::iterator, bool> inserted = 
              level_map_[0]->insert(std::make_pair(key, node));
            if (!inserted.second) {
              delete node.value;
            } else {
              updated = updater.update(inserted.first->second.value);
            }
          }
        }
        pthread_rwlock_unlock(&level_lock_[0]);
        return updated;
      }

    template<typename K, typename V>
      inline bool LevelTable<K, V>::erase(const key_t& key) {
        bool erased = false;

        pthread_rwlock_wrlock(&level_lock_[0]);
        typename hash_map_t::iterator iter = level_map_[0]->find(key);

        if (iter != level_map_[0]->end()) {
          erased = true;
          iter->second.mode |= MODE_DELETED;
          delete iter->second.value;
          iter->second.value = NULL;
        } else {
          bool finded = false;

          for (size_t i = 1; i < level_size_; ++i) {
            pthread_rwlock_rdlock(&level_lock_[i]);
            if (NULL == level_map_[i]) {
              pthread_rwlock_unlock(&level_lock_[i]);
              break;
            }
            iter = level_map_[i]->find(key);

            if (iter != level_map_[i]->end()) {
              uint8_t mode = iter->second.mode;

              pthread_rwlock_unlock(&level_lock_[i]);

              mode&MODE_DELETED ? finded = false:finded = true;
            }
            pthread_rwlock_unlock(&level_lock_[i]);
          }

          if (finded) {
            node_t node;

            node.mode = MODE_DELETED;
            node.value = NULL;

            typename std::pair<typename hash_map_t::iterator, bool> inserted = 
              level_map_[0]->insert(std::make_pair(key, node));
            if (inserted.second) 
              erased = true;
          }
        }
        pthread_rwlock_unlock(&level_lock_[0]);

        return erased;
      }

    // 合并Ln->L1不可变库
    template<typename K, typename V>
      inline bool LevelTable<K, V>::merge() {
        size_t depth_size = depth();
        
        if (depth_size < kMinLevel)
          return false;

        for (; depth_size >= kMinLevel; depth_size--) {
          size_t j = depth_size - 1, i = j - 1;

          typename hash_map_t::iterator iter = level_map_[i]->begin();
          for (; iter != level_map_[i]->end(); ++iter) {
            typename hash_map_t::iterator iter_find;

            pthread_rwlock_wrlock(&level_lock_[j]);
            iter_find = level_map_[j]->find(iter->first);
            if (iter_find == level_map_[j]->end()) {
              if (!level_map_[j]->insert(*iter).second) {
                //TODO: 错误处理
                fprintf(stderr, "merge failed: %llx\n", iter->first);
              }
            } else {
              // 回收邻接表内存
              delete iter_find->second.value;

              iter_find->second.mode = iter->second.mode;
              iter_find->second.value = iter->second.value;
            }
            pthread_rwlock_unlock(&level_lock_[j]);
          }

          pthread_rwlock_wrlock(&level_lock_[i]);
          pthread_rwlock_wrlock(&level_lock_[j]);
          hash_map_ptr_t obsolete_map = level_map_[i];

          level_map_[i] = level_map_[j];
          level_map_[j] = NULL;
          delete obsolete_map;
          pthread_rwlock_unlock(&level_lock_[j]);
          pthread_rwlock_unlock(&level_lock_[i]);
        }
        return true;
      }

    template<typename K, typename V>
      inline bool LevelTable<K, V>::deepen() {
        size_t depth_size = depth();

        if (depth_size + 1 > level_size_)
          return false;

        if (depth_size > 0) {
          for (size_t i = depth_size;  i > 0; --i) {
            pthread_rwlock_wrlock(&level_lock_[i]);
            pthread_rwlock_wrlock(&level_lock_[i - 1]);
            level_map_[i] = level_map_[i - 1];
            pthread_rwlock_unlock(&level_lock_[i - 1]);
            pthread_rwlock_unlock(&level_lock_[i]);
          }
        }
        pthread_rwlock_wrlock(&level_lock_[0]);
        level_map_[0] = new hash_map_t();
        level_map_[0]->set_empty_key(0);
        pthread_rwlock_unlock(&level_lock_[0]);

        return true;
      }

    // 获取访问不可变库迭代器
    template<typename K, typename V>
      inline typename LevelTable<K, V>::Iterator LevelTable<K, V>::snapshot()
      {
        size_t depth_size = depth();
        if (depth_size != kMinLevel - 1) {
          if (!merge())
            return Iterator(NULL, false);
        }
        return Iterator(level_map_[1], true);
      }
  } // namespace news
} // namespace rsys
