#pragma once

#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <vector>

class ShardedTT {
public:
  explicit ShardedTT(size_t shards = 64) : n(shards), maps(shards), locks(shards) {}

  void insert(uint64_t key, void* ptr) {
    auto &m = maps[shardIndex(key)];
    std::lock_guard<std::mutex> lk(locks[shardIndex(key)]);
    m[key] = ptr;
  }

  void* get(uint64_t key) {
    size_t s = shardIndex(key);
    std::lock_guard<std::mutex> lk(locks[s]);
    auto it = maps[s].find(key);
    return it == maps[s].end() ? nullptr : it->second;
  }

  void erase(uint64_t key) {
    size_t s = shardIndex(key);
    std::lock_guard<std::mutex> lk(locks[s]);
    maps[s].erase(key);
  }

  void clear() {
    for (size_t i = 0; i < n; ++i) {
      std::lock_guard<std::mutex> lk(locks[i]);
      maps[i].clear();
    }
  }

private:
  size_t shardIndex(uint64_t key) const { return key % n; }
  size_t n;
  std::vector<std::unordered_map<uint64_t, void*>> maps;
  std::vector<std::mutex> locks;
};
