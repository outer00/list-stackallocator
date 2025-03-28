#pragma once
#include <iterator>
#include <memory>
#include <ranges>
#include <type_traits>

template <std::size_t N>
class StackStorage {
  std::byte data[N];
  std::size_t space = N;

 public:
  StackStorage() = default;

  StackStorage(const StackStorage&) = delete;

  StackStorage& operator=(const StackStorage&) = delete;

  void* alloc(std::size_t cnt, std::size_t align) {
    void* ptr = static_cast<void*>(data + (N - space));
    space -= cnt;
    return std::align(align, cnt, ptr, space);
  }
};

template <typename T, std::size_t N>
class StackAllocator {
  StackStorage<N>* storage;

 public:
  using value_type = T;

  StackAllocator() = default;

  StackAllocator(StackStorage<N>& storage) : storage(&storage) {}

  template <typename U>
  StackAllocator(const StackAllocator<U, N>& oth)
      : storage(oth.get_storage()) {}

  ~StackAllocator() = default;

  template <typename U>
  StackAllocator& operator=(const StackAllocator<U, N>& oth) {
    storage = oth.get_storage();
    return *this;
  }

  void deallocate(T* ptr, std::size_t n) {
    (void)ptr;
    (void)n;
  }

  T* allocate(std::size_t cnt) {
    void* ptr = storage->alloc(cnt * sizeof(T), alignof(T));
    return reinterpret_cast<T*>(ptr);
  }

  template <typename U>
  struct rebind {
    using other = StackAllocator<U, N>;
  };

  StackStorage<N>* get_storage() const { return storage; }
};

template <typename T, std::size_t N>
bool operator!=(const StackAllocator<T, N>& a, const StackAllocator<T, N>& b) {
  return !(a == b);
}

template <typename T, std::size_t N>
bool operator==(const StackAllocator<T, N>& a, const StackAllocator<T, N> b) {
  return a.get_storage() == b.get_storage();
}
