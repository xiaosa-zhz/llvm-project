// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %s

// Test that the body of a generic lambda is only instantiated when its call
// operator template is instantiated (P0588R1, CWG1632, GH61426).

namespace basic {
template <typename T> void discarded(T t) {
  auto l = [](auto x) {
    if constexpr (sizeof(T) != sizeof(int)) {
      typename T::this_type_does_not_exist bad;
      return bad;
    } else {
      return x;
    }
  };
  (void)l(t);
}
void use() { discarded(0); }

template <typename T> void instantiated(T t) {
  auto l = [](auto x) {
    if constexpr (sizeof(T) == sizeof(int)) {
      typename T::this_type_does_not_exist bad; // expected-error {{type 'int' cannot be used prior to '::'}}
      return bad;
    } else {
      return x;
    }
  };
  (void)l(t); // expected-note {{in instantiation of function template specialization 'basic::instantiated(int)::(lambda)::operator()<int>' requested here}}
}
void use2() { instantiated(0); } // expected-note {{in instantiation of function template specialization 'basic::instantiated<int>' requested here}}

template <typename T> void never_called(T t) {
  auto l = [](auto x) { return x.this_member_does_not_exist(); };
  (void)sizeof(T);
}
void use3() { never_called(0); }

template <typename T> constexpr int discarded_constexpr(T t) {
  auto l = [](auto x) {
    if constexpr (sizeof(T) != sizeof(int))
      return sizeof(typename T::nope);
    else
      return sizeof(x);
  };
  return l(t);
}
static_assert(discarded_constexpr(1) == sizeof(int));
} // namespace basic

namespace P0588R0 {
template <typename T> struct fake_unique_ptr {
  fake_unique_ptr() = default;
  fake_unique_ptr(fake_unique_ptr&&) = default;
  fake_unique_ptr& operator=(fake_unique_ptr&&) = default;

  fake_unique_ptr(const fake_unique_ptr&) = delete;
  fake_unique_ptr& operator=(const fake_unique_ptr&) = delete;

  T* ptr = nullptr;
};

template <typename T> void f(T t) {
  auto lambda = [&](auto a) {
    if constexpr (requires { (void)static_cast<T>(t); } && sizeof(a) == sizeof(void*)) {
      T u = t;
    } else {
      // ... do not use t ...
      return;
    }
  };
  (void)lambda;
}

void use() { f(fake_unique_ptr<int>{}); }
} // namespace P0588R0
