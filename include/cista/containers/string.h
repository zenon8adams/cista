#pragma once

#include <cinttypes>
#include <cstring>

#include <ostream>
#include <string>
#include <string_view>

#include "cista/containers/ptr.h"

namespace cista {

template <typename Ptr = char const*>
struct generic_string {
  using msize_t = std::uint32_t;
  using value_type = char;

  static msize_t mstrlen(char const* s) noexcept {
    return static_cast<msize_t>(std::strlen(s));
  }

  static constexpr struct owning_t {
  } owning{};
  static constexpr struct non_owning_t {
  } non_owning{};

  constexpr generic_string() noexcept {}
  ~generic_string() noexcept { reset(); }

  generic_string(std::string_view s, owning_t const) { set_owning(s); }
  generic_string(std::string_view s, non_owning_t const) { set_non_owning(s); }
  generic_string(std::string const& s, owning_t const) { set_owning(s); }
  generic_string(std::string const& s, non_owning_t const) {
    set_non_owning(s);
  }
  generic_string(char const* s, owning_t const) { set_owning(s, mstrlen(s)); }
  generic_string(char const* s, non_owning_t const) { set_non_owning(s); }
  generic_string(char const* s, msize_t const len, owning_t const) {
    set_owning(s, len);
  }
  generic_string(char const* s, msize_t const len, non_owning_t const) {
    set_non_owning(s, len);
  }

  char* begin() noexcept { return data(); }
  char* end() noexcept { return data() + size(); }
  char const* begin() const noexcept { return data(); }
  char const* end() const noexcept { return data() + size(); }

  friend char const* begin(generic_string const& s) { return s.begin(); }
  friend char* begin(generic_string& s) { return s.begin(); }
  friend char const* end(generic_string const& s) { return s.end(); }
  friend char* end(generic_string& s) { return s.end(); }

  bool is_short() const noexcept { return s_.is_short_; }

  void reset() noexcept {
    if (!h_.is_short_ && h_.ptr_ != nullptr && h_.self_allocated_) {
      std::free(data());
    }
    h_ = heap{};
  }

  void set_owning(std::string const& s) {
    set_owning(s.data(), static_cast<msize_t>(s.size()));
  }

  void set_owning(std::string_view s) {
    set_owning(s.data(), static_cast<msize_t>(s.size()));
  }

  void set_owning(char const* str) { set_owning(str, mstrlen(str)); }

  static constexpr msize_t short_length_limit = 15U;

  void set_owning(char const* str, msize_t const len) {
    reset();
    if (str == nullptr || len == 0U) {
      return;
    }
    s_.is_short_ = (len <= short_length_limit);
    if (s_.is_short_) {
      std::memcpy(s_.s_, str, len);
      for (auto i = len; i < short_length_limit; ++i) {
        s_.s_[i] = 0;
      }
    } else {
      h_.ptr_ = static_cast<char*>(std::malloc(len));
      if (h_.ptr_ == nullptr) {
        throw std::bad_alloc{};
      }
      h_.size_ = len;
      h_.self_allocated_ = true;
      std::memcpy(data(), str, len);
    }
  }

  void set_non_owning(std::string const& v) {
    set_non_owning(v.data(), static_cast<msize_t>(v.size()));
  }

  void set_non_owning(std::string_view v) {
    set_non_owning(v.data(), static_cast<msize_t>(v.size()));
  }

  void set_non_owning(char const* str) { set_non_owning(str, mstrlen(str)); }

  void set_non_owning(char const* str, msize_t const len) {
    reset();
    if (str == nullptr || len == 0U) {
      return;
    }

    if (len <= short_length_limit) {
      return set_owning(str, len);
    }

    h_.is_short_ = false;
    h_.self_allocated_ = false;
    h_.ptr_ = str;
    h_.size_ = len;
  }

  void move_from(generic_string&& s) noexcept {
    reset();
    std::memcpy(static_cast<void*>(this), &s, sizeof(*this));
    if constexpr (std::is_pointer_v<Ptr>) {
      std::memset(static_cast<void*>(&s), 0, sizeof(*this));
    } else {
      if (!s.is_short()) {
        h_.ptr_ = s.h_.ptr_;
        s.h_.ptr_ = nullptr;
        s.h_.size_ = 0U;
      }
    }
  }

  void copy_from(generic_string const& s) {
    reset();
    if (s.is_short()) {
      std::memcpy(static_cast<void*>(this), &s, sizeof(s));
    } else if (s.h_.self_allocated_) {
      set_owning(s.data(), s.size());
    } else {
      set_non_owning(s.data(), s.size());
    }
  }

  bool empty() const noexcept { return size() == 0U; }
  std::string_view view() const noexcept { return {data(), size()}; }
  std::string str() const { return {data(), size()}; }

  operator std::string_view() const { return view(); }

  char& operator[](std::size_t const i) noexcept { return data()[i]; }
  char const& operator[](std::size_t const i) const noexcept {
    return data()[i];
  }

  friend std::ostream& operator<<(std::ostream& out, generic_string const& s) {
    return out << s.view();
  }

  friend bool operator==(generic_string const& a,
                         generic_string const& b) noexcept {
    return a.view() == b.view();
  }

  friend bool operator!=(generic_string const& a,
                         generic_string const& b) noexcept {
    return a.view() != b.view();
  }

  friend bool operator<(generic_string const& a,
                        generic_string const& b) noexcept {
    return a.view() < b.view();
  }

  friend bool operator>(generic_string const& a,
                        generic_string const& b) noexcept {
    return a.view() > b.view();
  }

  friend bool operator<=(generic_string const& a,
                         generic_string const& b) noexcept {
    return a.view() <= b.view();
  }

  friend bool operator>=(generic_string const& a,
                         generic_string const& b) noexcept {
    return a.view() >= b.view();
  }

  friend bool operator==(generic_string const& a, std::string_view b) noexcept {
    return a.view() == b;
  }

  friend bool operator!=(generic_string const& a, std::string_view b) noexcept {
    return a.view() != b;
  }

  friend bool operator<(generic_string const& a, std::string_view b) noexcept {
    return a.view() < b;
  }

  friend bool operator>(generic_string const& a, std::string_view b) noexcept {
    return a.view() > b;
  }

  friend bool operator<=(generic_string const& a, std::string_view b) noexcept {
    return a.view() <= b;
  }

  friend bool operator>=(generic_string const& a, std::string_view b) noexcept {
    return a.view() >= b;
  }

  friend bool operator==(std::string_view a, generic_string const& b) noexcept {
    return a == b.view();
  }

  friend bool operator!=(std::string_view a, generic_string const& b) noexcept {
    return a != b.view();
  }

  friend bool operator<(std::string_view a, generic_string const& b) noexcept {
    return a < b.view();
  }

  friend bool operator>(std::string_view a, generic_string const& b) noexcept {
    return a > b.view();
  }

  friend bool operator<=(std::string_view a, generic_string const& b) noexcept {
    return a <= b.view();
  }

  friend bool operator>=(std::string_view a, generic_string const& b) noexcept {
    return a >= b.view();
  }

  friend bool operator==(generic_string const& a, char const* b) noexcept {
    return a.view() == std::string_view{b};
  }

  friend bool operator!=(generic_string const& a, char const* b) noexcept {
    return a.view() != std::string_view{b};
  }

  friend bool operator<(generic_string const& a, char const* b) noexcept {
    return a.view() < std::string_view{b};
  }

  friend bool operator>(generic_string const& a, char const* b) noexcept {
    return a.view() > std::string_view{b};
  }

  friend bool operator<=(generic_string const& a, char const* b) noexcept {
    return a.view() <= std::string_view{b};
  }

  friend bool operator>=(generic_string const& a, char const* b) noexcept {
    return a.view() >= std::string_view{b};
  }

  friend bool operator==(char const* a, generic_string const& b) noexcept {
    return std::string_view{a} == b.view();
  }

  friend bool operator!=(char const* a, generic_string const& b) noexcept {
    return std::string_view{a} != b.view();
  }

  friend bool operator<(char const* a, generic_string const& b) noexcept {
    return std::string_view{a} < b.view();
  }

  friend bool operator>(char const* a, generic_string const& b) noexcept {
    return std::string_view{a} > b.view();
  }

  friend bool operator<=(char const* a, generic_string const& b) noexcept {
    return std::string_view{a} <= b.view();
  }

  friend bool operator>=(char const* a, generic_string const& b) noexcept {
    return std::string_view{a} >= b.view();
  }

  char const* internal_data() const noexcept {
    if constexpr (std::is_pointer_v<Ptr>) {
      return is_short() ? s_.s_ : h_.ptr_;
    } else {
      return is_short() ? s_.s_ : h_.ptr_.get();
    }
  }

  char* data() noexcept { return const_cast<char*>(internal_data()); }
  char const* data() const noexcept { return internal_data(); }

  msize_t size() const noexcept {
    if (is_short()) {
      auto const pos =
          static_cast<char const*>(std::memchr(s_.s_, 0, short_length_limit));
      return (pos != nullptr) ? static_cast<msize_t>(pos - s_.s_)
                              : short_length_limit;
    }
    return h_.size_;
  }

  struct heap {
    bool is_short_{false};
    bool self_allocated_{false};
    std::uint16_t __fill__{0};
    std::uint32_t size_{0};
    Ptr ptr_{nullptr};
  };

  struct stack {
    bool is_short_{true};
    char s_[short_length_limit]{0};
  };

  union {
    heap h_{};
    stack s_;
  };
};

template <typename Ptr>
struct basic_string : public generic_string<Ptr> {
  using base = generic_string<Ptr>;

  using base::base;
  using base::operator std::string_view;

  friend std::ostream& operator<<(std::ostream& out, basic_string const& s) {
    return out << s.view();
  }

  explicit operator std::string() const { return {base::data(), base::size()}; }

  basic_string(std::string_view s) : base{s, base::owning} {}
  basic_string(std::string const& s) : base{s, base::owning} {}
  basic_string(char const* s) : base{s, base::owning} {}
  basic_string(char const* s, typename base::msize_t const len)
      : base{s, len, base::owning} {}

  basic_string(basic_string const& o) : base{o.view(), base::owning} {}
  basic_string(basic_string&& o) { base::move_from(std::move(o)); }

  basic_string& operator=(basic_string const& o) {
    base::set_owning(o.data(), o.size());
    return *this;
  }

  basic_string& operator=(basic_string&& o) {
    base::move_from(std::move(o));
    return *this;
  }

  basic_string& operator=(char const* s) {
    base::set_owning(s);
    return *this;
  }
  basic_string& operator=(std::string const& s) {
    base::set_owning(s);
    return *this;
  }
  basic_string& operator=(std::string_view s) {
    base::set_owning(s);
    return *this;
  }
};

template <typename Ptr>
struct basic_string_view : public generic_string<Ptr> {
  using base = generic_string<Ptr>;

  using base::base;
  using base::operator std::string_view;

  friend std::ostream& operator<<(std::ostream& out,
                                  basic_string_view const& s) {
    return out << s.view();
  }

  basic_string_view(std::string_view s) : base{s, base::non_owning} {}
  basic_string_view(std::string const& s) : base{s, base::non_owning} {}
  basic_string_view(char const* s) : base{s, base::non_owning} {}
  basic_string_view(char const* s, typename base::msize_t const len)
      : base{s, len, base::non_owning} {}

  basic_string_view(basic_string_view const& o) {
    base::set_non_owning(o.data(), o.size());
  }
  basic_string_view(basic_string_view&& o) {
    base::set_non_owning(o.data(), o.size());
  }
  basic_string_view& operator=(basic_string_view const& o) {
    base::set_non_owning(o.data(), o.size());
    return *this;
  }
  basic_string_view& operator=(basic_string_view&& o) {
    base::set_non_owning(o.data(), o.size());
    return *this;
  }

  basic_string_view& operator=(char const* s) {
    base::set_non_owning(s);
    return *this;
  }
  basic_string_view& operator=(std::string_view s) {
    base::set_non_owning(s);
    return *this;
  }
  basic_string_view& operator=(std::string const& s) {
    base::set_non_owning(s);
    return *this;
  }
};

template <typename Ptr>
struct is_string_helper : std::false_type {};

template <typename Ptr>
struct is_string_helper<generic_string<Ptr>> : std::true_type {};

template <typename Ptr>
struct is_string_helper<basic_string<Ptr>> : std::true_type {};

template <typename Ptr>
struct is_string_helper<basic_string_view<Ptr>> : std::true_type {};

template <class T>
constexpr bool is_string_v = is_string_helper<std::remove_cv_t<T>>::value;

namespace raw {
using generic_string = generic_string<ptr<char const>>;
using string = basic_string<ptr<char const>>;
using string_view = basic_string_view<ptr<char const>>;
}  // namespace raw

namespace offset {
using generic_string = generic_string<ptr<char const>>;
using string = basic_string<ptr<char const>>;
using string_view = basic_string_view<ptr<char const>>;
}  // namespace offset

}  // namespace cista
