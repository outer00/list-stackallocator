#pragma once
#include <iterator>
#include <memory>
#include <ranges>
#include <type_traits>

template <typename T, typename Allocator = std::allocator<T>>
class List {
  struct BaseNode {
    BaseNode* prev = nullptr;
    BaseNode* next = nullptr;
    BaseNode() = default;
  };
  struct Node : BaseNode {
    T val;
    Node() = default;
    explicit Node(const T& x) : val(x) {}
  };

  using AllocTraits = std::allocator_traits<Allocator>;
  using NodeTraits = typename AllocTraits::template rebind_traits<Node>;
  using NodeAlloc = typename AllocTraits::template rebind_alloc<Node>;

  [[no_unique_address]] NodeAlloc alloc;
  BaseNode base_node;
  std::size_t sz;

 public:
  template <bool is_const>
  struct iter {
    using value_type = std::conditional_t<is_const, const T, T>;
    using reference = std::conditional_t<is_const, const T&, T&>;
    using pointer = std::conditional_t<is_const, const T*, T*>;
    using iterator_category = std::bidirectional_iterator_tag;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using node_ptr = std::conditional_t<is_const, const Node*, Node*>;
    using base_node_ptr =
        std::conditional_t<is_const, const BaseNode*, BaseNode*>;

    iter(base_node_ptr node) : node(node) {}
    iter(const iter&) = default;
    iter& operator=(const iter&) = default;

    base_node_ptr node;

    reference operator*() const { return static_cast<node_ptr>(node)->val; }

    iter& operator++() {
      node = node->next;
      return *this;
    }
    iter& operator--() {
      node = node->prev;
      return *this;
    }
    iter operator++(int) {
      iter temp = *this;
      ++(*this);
      return temp;
    }
    iter operator--(int) {
      iter temp = *this;
      --(*this);
      return temp;
    }

    bool operator==(const iter& oth) const { return node == oth.node; }
    bool operator!=(const iter& oth) const { return node != oth.node; }

    operator iter<true>() const { return iter<true>(node); }
  };

  using iterator = iter<false>;
  using const_iterator = iter<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  void clear() {
    auto& a = alloc;
    BaseNode* node = base_node.next;
    while (sz > 0) {
      BaseNode* next = node->next;
      NodeTraits::destroy(a, static_cast<Node*>(node));
      NodeTraits::deallocate(a, static_cast<Node*>(node), 1);
      node = next;
      --sz;
    }
    base_node.next = base_node.prev = &base_node;
  }

  List() : List(Allocator()) {}

  List(std::size_t n) : List(n, Allocator()) {}

  List(std::size_t n, const T& val) : List(n, val, Allocator()) {}

  List(Allocator alloca) : alloc(alloca), sz(0) {
    base_node.next = base_node.prev = &base_node;
  }

  List(std::size_t n, Allocator alloca) : List(alloca) {
    BaseNode* node = &base_node;
    auto& a = this->alloc;
    for (std::size_t i = 0; i < n; ++i) {
      node->next = static_cast<BaseNode*>(NodeTraits::allocate(a, 1));
      try {
        NodeTraits::construct(a, static_cast<Node*>(node->next));
      } catch (...) {
        NodeTraits::deallocate(a, static_cast<Node*>(node->next), 1);
        node->next = &base_node;
        base_node.prev = node;
        clear();
        throw;
      }
      node->next->prev = node;
      node = node->next;
      ++sz;
    }
    node->next = &base_node;
    base_node.prev = node;
  }

  List(std::size_t n, const T& val, Allocator alloca) : List(alloca) {
    BaseNode* node = &base_node;
    auto& a = alloc;
    for (std::size_t i = 0; i < n; ++i) {
      node->next = static_cast<BaseNode*>(NodeTraits::allocate(a, 1));
      try {
        NodeTraits::construct(a, static_cast<Node*>(node->next), val);

      } catch (...) {
        NodeTraits::deallocate(a, static_cast<Node*>(node->next), 1);
        node->next = &base_node;
        base_node.prev = node;
        clear();
        throw;
      }
      node->next->prev = node;
      node = node->next;
      ++sz;
    }
    node->next = &base_node;
    base_node.prev = node;
  }

  Allocator get_allocator() const { return Allocator(alloc); }

  List(const List& other)
      : alloc(AllocTraits::select_on_container_copy_construction(
            other.get_allocator())),
        sz(0) {

    BaseNode* node = &base_node;
    auto& a = this->alloc;
    for (const auto& el : other) {
      node->next = static_cast<BaseNode*>(NodeTraits::allocate(a, 1));
      try {
        NodeTraits::construct(a, static_cast<Node*>(node->next), el);
      } catch (...) {
        NodeTraits::deallocate(a, static_cast<Node*>(node->next), 1);
        node->next = &base_node;
        base_node.prev = node;
        clear();
        throw;
      }
      node->next->prev = node;
      node = node->next;
      ++sz;
    }
    node->next = &base_node;
    base_node.prev = node;
  }

  List& operator=(const List& oth) {
    if (this == &oth) {
      return *this;
    }
    if constexpr (std::allocator_traits<Allocator>::
                      propagate_on_container_copy_assignment::value) {
      alloc = oth.get_allocator();
    }
    List temp(alloc);
    for (const auto& el : oth) {
      temp.push_back(el);
    }
    std::swap(base_node.prev, temp.base_node.prev);
    std::swap(base_node.next, temp.base_node.next);
    if (base_node.next != &base_node) {
      base_node.next->prev = &base_node;
      base_node.prev->next = &base_node;
    }
    if (temp.base_node.next != &temp.base_node) {
      temp.base_node.next->prev = &temp.base_node;
      temp.base_node.prev->next = &temp.base_node;
    }
    std::swap(sz, temp.sz);
    return *this;
  }

  ~List() {
    while (!empty()) {
      pop_back();
    }
  }

  iterator begin() { return iterator(base_node.next); }
  const_iterator begin() const { return const_iterator(base_node.next); }
  iterator end() { return iterator(&base_node); }
  const_iterator end() const { return const_iterator(&base_node); }
  const_iterator cbegin() const { return const_iterator(base_node.next); }
  const_iterator cend() const { return const_iterator(&base_node); }
  reverse_iterator rbegin() { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }
  reverse_iterator rend() { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }
  const_reverse_iterator crbegin() const {
    return const_reverse_iterator(end());
  }
  const_reverse_iterator crend() const {
    return const_reverse_iterator(begin());
  }

  void push_back(const T& val) { insert(end(), val); }
  void push_front(const T& val) { insert(begin(), val); }
  void pop_back() { erase(--end()); }
  void pop_front() { erase(begin()); }

  void insert(const_iterator it, const T& val) {
    BaseNode* node = const_cast<BaseNode*>(it.node);
    BaseNode* prev = node->prev;
    auto& a = this->alloc;
    BaseNode* new_node = nullptr;
    try {
      new_node = static_cast<BaseNode*>(NodeTraits::allocate(a, 1));
      NodeTraits::construct(a, static_cast<Node*>(new_node), val);
    } catch (...) {
      NodeTraits::deallocate(a, static_cast<Node*>(new_node), 1);
      throw;
    }
    new_node->prev = prev;
    prev->next = new_node;
    new_node->next = node;
    node->prev = new_node;
    ++sz;
  }

  void erase(const_iterator it) {
    BaseNode* node = const_cast<BaseNode*>(it.node);
    BaseNode* prev = node->prev;
    BaseNode* next = node->next;
    prev->next = next;
    next->prev = prev;
    auto& a = this->alloc;
    NodeTraits::destroy(a, static_cast<Node*>(node));
    NodeTraits::deallocate(a, static_cast<Node*>(node), 1);
    --sz;
  }

  std::size_t size() const { return sz; }
  bool empty() const { return sz == 0; }
};

