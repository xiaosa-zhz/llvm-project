// RUN: %clang_cc1 -std=c++2c -fsyntax-only -verify %s

// Test that the captures of a generic lambda whose body instantiation is
// deferred (P0588R1) are still computed, and recorded with the right capture
// kind, when the closure type is instantiated.

namespace byref {
template <class F> int f(F fn) {
  int local = 40;
  auto l = [&]<int I>() { local += I; return local + fn(); };
  // 'local' and 'fn' are both captured by reference.
  static_assert(sizeof(l) == 2 * sizeof(void *));
  return l.template operator()<2>();
}
int g() { return 0; }
int use1() { return f(g); }
} // namespace byref

namespace bycopy {
template <class T> int f(T t) {
  auto l = [=](auto x) mutable { t += (int)x; return t; };
  static_assert(sizeof(l) == sizeof(T));
  return l(1);
}
int use2() { return f(1); }
} // namespace bycopy

namespace this_capture {
struct S {
  int v = 41;
  template <class T> int f(T) {
    auto l = [&, this](auto x) { return v + (int)x; };
    // Only 'this' is captured, by reference.
    static_assert(sizeof(l) == sizeof(void *));
    return l(0);
  }
};
int use3() { return S{}.f(0); }

template <class T> struct S2 {
  T v;
  template <class U> int f(U) {
    auto l = [this](auto x) { return (int)v + (int)x; };
    static_assert(sizeof(l) == sizeof(void *));
    return l(0);
  }
};
int use4() { return S2<int>{41}.f(0); }
} // namespace this_capture

namespace nested {
template <class T> int f(T t) {
  auto outer = [&](auto x) {
    auto inner = [&](auto y) { return (int)t + (int)x + (int)y; };
    // The inner lambda captures 't' and 'x' by reference; 'y' is its own
    // parameter.
    static_assert(sizeof(inner) == 2 * sizeof(void *));
    return inner(1);
  };
  return outer(2);
}
int use5() { return f(3); }
} // namespace nested

namespace pack_capture {
template <class... Ts> int f(Ts... xs) {
  auto l = [=]() { return (int)(xs + ...); };
  static_assert(sizeof(l) == sizeof(int) * sizeof...(Ts));
  return l();
}
template <class... Ts> int g(Ts... xs) {
  auto l = [=]() { return (int)(xs...[0]); };
  static_assert(sizeof(l) == sizeof(int) * sizeof...(Ts));
  return l();
}
int use6() { return f(1, 2, 3) + g(1, 2, 3); }
} // namespace pack_capture

namespace init_capture {
template <class T> int f(T t) {
  auto l = [value = t](auto x) { return (int)value + (int)x; };
  static_assert(sizeof(l) == sizeof(T));
  return l(1);
}
int use7() { return f(1); }
} // namespace init_capture

namespace diagnostics {
template <class T> int captured_by_copy(T t) {
  int local = 0;
  auto l = [=](auto x) {
    local += (int)x; // expected-error {{cannot assign to a variable captured by copy in a non-mutable lambda}}
    return local;
  };
  return l(1);
}
int use8() { return captured_by_copy(0); }

struct S {
  int v;
  template <class T> int no_this_capture(T) {
    // 'this' is not captured, but is used only in a discarded constexpr-if
    // branch, so this is valid.
    auto l = [](auto x) {
      if constexpr (sizeof(T) == 8)
        return 0;
      else
        return (int)x;
    };
    return l(1);
  }
};
int use9() { return S{0}.no_this_capture(0); }
} // namespace diagnostics
