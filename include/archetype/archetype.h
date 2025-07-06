#ifndef __ARCHETYPE_H__
#define __ARCHETYPE_H__

#include <type_traits>

namespace archetype {
class Base {
public:
  virtual ~Base() {};

  template <typename T> void bind(T &t) { _obj = static_cast<void *>(&t); }

protected:
  void *_obj;
};

// helper to expose protected component to perform inheritance chaining
template<class C>
class helper
{
  public:
  template<typename T=archetype::Base>
  using get = typename C::template component<T>;
};

template<class BASE>
class identity : public BASE {};
} // namespace archetype



#define DEFINE_ARCHETYPE(NAME, METHODS)                                       \
  struct NAME {                                                               \
    NAME() = delete;                                                          \
                                                                              \
    public: friend class archetype::helper<NAME>;                             \
                                                                              \
    template <template <typename> class Interface>                            \
    class ptr;                                                                \
                                                                              \
                                                                              \
    /* SFINAE based type checking against requirements */                     \
    template<typename, typename = void>                                       \
    struct check : std::false_type {};                                        \
                                                                              \
    template<typename T>                                                      \
    struct check<T, std::void_t<decltype(                                     \
      EXPAND_ARCHETYPE_REQUIREMENTS(METHODS)                                  \
      )>> : std::true_type {};                                                \
                                                                              \
                                                                              \
    /* Internal protected view component implementation */                    \
    protected:                                                                \
    template <typename B = archetype::Base>                                   \
    class component : public B                                                \
    {                                                                         \
      public:                                                                 \
      template <template <typename> class Interface>                          \
      friend class ptr;                                                       \
      EXPAND_ARCHETYPE_METHODS(METHODS)                                       \
                                                                              \
      protected:                                                              \
      template <typename T> void bind(T &t) {                                 \
        this->B::bind(t);                                                     \
        EXPAND_CALLSTUB_ASSIGNMENTS(METHODS)                                  \
      }                                                                       \
                                                                              \
      using B::_obj;                                                          \
      EXPAND_CALLSTUB_MEMBERS(METHODS)                                        \
    };                                                                        \
                                                                              \
                                                                              \
    /* Public view, exposes component interface*/                             \
    public:                                                                   \
    class view : public component<archetype::Base>                            \
    {                                                                         \
      public:                                                                 \
      using component<archetype::Base>::bind;                                 \
    };                                                                        \
                                                                              \
                                                                              \
    template <template <typename> class Interface = archetype::identity>      \
    class ptr                                                                 \
    {                                                                         \
    private:                                                                  \
      using T = Interface<component<>>;                                       \
      T impl;                                                                 \
                                                                              \
    public:                                                                   \
      template <typename CONCEPT>                                             \
      void bind(CONCEPT &ref) {                                               \
        impl.bind(ref);                                                       \
      }                                                                       \
                                                                              \
      T &operator*() { return &impl; }                                        \
      const T &operator*() const { return &impl; }                            \
                                                                              \
      T *operator->() { return &impl; }                                       \
      const T *operator->() const { return &impl; }                           \
    };                                                                        \
                                                                              \
  };

#define COMPOSE_ARCHETYPE(NAME, ...)                                          \
  struct NAME {                                                               \
    NAME() = delete;                                                          \
                                                                              \
  public:                                                                     \
                                                                              \
    template <template <typename> class Interface>                            \
    class ptr;                                                                \
                                                                              \
    template<typename T>                                                      \
    struct check                                                              \
      : std::integral_constant<bool,                                          \
        EXPAND_COMPONENT_REQUIREMENTS(__VA_ARGS__)                            \
        > {};                                                                 \
                                                                              \
    protected:                                                                \
    template <typename B = archetype::Base>                                   \
    class component : public EXPAND_COMPONENT_INHERITANCE(__VA_ARGS__)        \
    {                                                                         \
      public:                                                                 \
      template <template <typename> class Interface>                          \
      friend class ptr;                                                       \
    };                                                                        \
                                                                              \
    /* Public view, exposes component interface*/                             \
    public:                                                                   \
    class view : public component<>                                           \
    {                                                                         \
      public:                                                                 \
      using component<>::bind;                                                \
    };                                                                        \
                                                                              \
    /*Convenience class, for ptr syntax */                                    \
    template <template <typename> class Interface = archetype::identity>      \
    class ptr                                                                 \
    {                                                                         \
    private:                                                                  \
      using T = Interface<component<>>;                                       \
      T impl;                                                                 \
                                                                              \
    public:                                                                   \
      template <typename CONCEPT>                                             \
      void bind(CONCEPT &ref) {                                               \
        impl.bind(ref);                                                       \
      }                                                                       \
                                                                              \
      T &operator*() { return &impl; }                                        \
      const T &operator*() const { return &impl; }                            \
                                                                              \
      T *operator->() { return &impl; }                                       \
      const T *operator->() const { return &impl; }                           \
    };                                                                        \
  };



#define EXPAND_ARCHETYPE_METHODS(METHODS) EXPAND_ARCHETYPE_METHODS_IMPL METHODS

#define EXPAND_ARCHETYPE_METHODS_IMPL(...)                                     \
  FOR_EACH(ARCHETYPE_METHOD, __VA_ARGS__)

#define EXPAND_CALLSTUB_ASSIGNMENTS(METHODS)                                   \
  EXPAND_CALLSTUB_ASSIGNMENTS_IMPL METHODS

#define EXPAND_CALLSTUB_ASSIGNMENTS_IMPL(...)                                  \
  FOR_EACH(CALLSTUB_ASSIGNMENT, __VA_ARGS__)

#define EXPAND_CALLSTUB_MEMBERS(METHODS) EXPAND_CALLSTUB_MEMBERS_IMPL METHODS

#define EXPAND_CALLSTUB_MEMBERS_IMPL(...) FOR_EACH(CALLSTUB_MEMBER, __VA_ARGS__)

#define EXPAND_CONCEPT_REQUIREMENTS(METHODS)                                   \
  EXPAND_CONCEPT_REQUIREMENTS_IMPL METHODS

#define EXPAND_CONCEPT_REQUIREMENTS_IMPL(...)                                  \
  FOR_EACH(CONCEPT_REQUIREMENT, __VA_ARGS__)

#define EXPAND_ARCHETYPE_REQUIREMENTS(METHODS)                                   \
  EXPAND_ARCHETYPE_REQUIREMENTS_IMPL METHODS

#define EXPAND_ARCHETYPE_REQUIREMENTS_IMPL(...)                                  \
  FOR_EACH_SEP(ARCHETYPE_REQUIREMENT, __VA_ARGS__)

#define EXPAND_CONCEPT_ASSERTIONS(METHODS)                                     \
  EXPAND_CONCEPT_ASSERTIONS_IMPL METHODS

#define EXPAND_CONCEPT_ASSERTIONS_IMPL(...)                                    \
  FOR_EACH(CONCEPT_ASSERTION, __VA_ARGS__)

#define EXPAND_COMPONENT_ASSERTIONS(...) FOR_EACH_CALL(DO_ASSERT, __VA_ARGS__)

#define APPEND_BASE(x) x::component

#define APPEND_CHECK(x) x::check<T>::value

#define APPLY_HELPER(x) archetype::helper<x>::get

#define EXPAND_COMPONENT_INHERITANCE_IMPL(...)                                 \
  TEMPLATE_CHAIN(__VA_ARGS__ COMMA_IF_ARGS(__VA_ARGS__) B)


#define EXPAND_COMPONENT_INHERITANCE(...)                                      \
  EXPAND_COMPONENT_INHERITANCE_IMPL(FOR_EACH_SEP_CALL(APPLY_HELPER, __VA_ARGS__))



#define EXPAND_COMPONENT_REQUIREMENTS(...)                                      \
  FOR_EACH_SEPX_CALL(APPEND_CHECK, &&, __VA_ARGS__)


#define EXPAND(x) x
#define EXPAND2(x) EXPAND(EXPAND(x))

#define FOR_EACH(M, ...)                                                       \
  EXPAND(GET_MACRO(__VA_ARGS__, FE10, FE9, FE8, FE7, FE6, FE5, FE4, FE3, FE2,  \
                   FE1)(M, __VA_ARGS__))

#define GET_MACRO(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, NAME, ...) NAME

#define FE1(M, x) M x
#define FE2(M, x, ...) M x FE1(M, __VA_ARGS__)
#define FE3(M, x, ...) M x FE2(M, __VA_ARGS__)
#define FE4(M, x, ...) M x FE3(M, __VA_ARGS__)
#define FE5(M, x, ...) M x FE4(M, __VA_ARGS__)
#define FE6(M, x, ...) M x FE5(M, __VA_ARGS__)
#define FE7(M, x, ...) M x FE6(M, __VA_ARGS__)
#define FE8(M, x, ...) M x FE7(M, __VA_ARGS__)
#define FE9(M, x, ...) M x FE8(M, __VA_ARGS__)
#define FE10(M, x, ...) M x FE9(M, __VA_ARGS__)

#define FE1_2(M, T, x) M(T, x)
#define FE2_2(M, T, x, ...) M(T, x) FE1_2(M, T, __VA_ARGS__)
#define FE3_2(M, T, x, ...) M(T, x) FE2_2(M, T, __VA_ARGS__)
#define FE4_2(M, T, x, ...) M(T, x) FE3_2(M, T, __VA_ARGS__)
#define FE5_2(M, T, x, ...) M(T, x) FE4_2(M, T, __VA_ARGS__)
#define FE6_2(M, T, x, ...) M(T, x) FE5_2(M, T, __VA_ARGS__)
#define FE7_2(M, T, x, ...) M(T, x) FE6_2(M, T, __VA_ARGS__)
#define FE8_2(M, T, x, ...) M(T, x) FE7_2(M, T, __VA_ARGS__)
#define FE9_2(M, T, x, ...) M(T, x) FE8_2(M, T, __VA_ARGS__)
#define FE10_2(M, T, x, ...) M(T, x) FE9_2(M, T, __VA_ARGS__)


#define FOR_EACH_SEP(M, ...)                                                       \
  EXPAND(GET_MACRO(__VA_ARGS__, FES10, FES9, FES8, FES7, FES6, FES5, FES4, FES3, FES2,  \
                   FES1)(M, __VA_ARGS__))

#define FES1(M, x) M x
#define FES2(M, x, ...) M x ,FE1(M, __VA_ARGS__)
#define FES3(M, x, ...) M x ,FE2(M, __VA_ARGS__)
#define FES4(M, x, ...) M x ,FE3(M, __VA_ARGS__)
#define FES5(M, x, ...) M x ,FE4(M, __VA_ARGS__)
#define FES6(M, x, ...) M x ,FE5(M, __VA_ARGS__)
#define FES7(M, x, ...) M x ,FE6(M, __VA_ARGS__)
#define FES8(M, x, ...) M x ,FE7(M, __VA_ARGS__)
#define FES9(M, x, ...) M x ,FE8(M, __VA_ARGS__)
#define FES10(M, x, ...) M x ,FE9(M, __VA_ARGS__)

#define FES1_2(M, T, x) M(T, x)
#define FES2_2(M, T, x, ...) M(T, x), FE1_2(M, T, __VA_ARGS__)
#define FES3_2(M, T, x, ...) M(T, x), FE2_2(M, T, __VA_ARGS__)
#define FES4_2(M, T, x, ...) M(T, x), FE3_2(M, T, __VA_ARGS__)
#define FES5_2(M, T, x, ...) M(T, x), FE4_2(M, T, __VA_ARGS__)
#define FES6_2(M, T, x, ...) M(T, x), FE5_2(M, T, __VA_ARGS__)
#define FES7_2(M, T, x, ...) M(T, x), FE6_2(M, T, __VA_ARGS__)
#define FES8_2(M, T, x, ...) M(T, x), FE7_2(M, T, __VA_ARGS__)
#define FES9_2(M, T, x, ...) M(T, x), FE8_2(M, T, __VA_ARGS__)
#define FES10_2(M, T, x, ...) M(T, x), FE9_2(M, T, __VA_ARGS__)



#define FOR_EACH_CALL_1(M, a1) M(a1)
#define FOR_EACH_CALL_2(M, a1, a2) M(a1) M(a2)
#define FOR_EACH_CALL_3(M, a1, a2, a3) M(a1) M(a2) M(a3)
#define FOR_EACH_CALL_4(M, a1, a2, a3, a4) M(a1) M(a2) M(a3) M(a4)
#define FOR_EACH_CALL_5(M, a1, a2, a3, a4, a5) M(a1) M(a2) M(a3) M(a4) M(a5)
#define FOR_EACH_CALL_6(M, a1, a2, a3, a4, a5, a6)                             \
  M(a1) M(a2) M(a3) M(a4) M(a5) M(a6)
#define FOR_EACH_CALL_7(M, a1, a2, a3, a4, a5, a6, a7)                         \
  M(a1) M(a2) M(a3) M(a4) M(a5) M(a6) M(a7)
#define FOR_EACH_CALL_8(M, a1, a2, a3, a4, a5, a6, a7, a8)                     \
  M(a1) M(a2) M(a3) M(a4) M(a5) M(a6) M(a7) M(a8)

#define FOR_EACH_CALL(M, ...)                                                  \
  GET_FOR_EACH_CALL(M_NARGS(__VA_ARGS__))(M, __VA_ARGS__)

#define GET_FOR_EACH_CALL(N) CAT(FOR_EACH_CALL_, N)

#define FOR_EACH_SEP_CALL_1(M, a1) M(a1)
#define FOR_EACH_SEP_CALL_2(M, a1, a2) M(a1), M(a2)
#define FOR_EACH_SEP_CALL_3(M, a1, a2, a3) M(a1), M(a2), M(a3)
#define FOR_EACH_SEP_CALL_4(M, a1, a2, a3, a4) M(a1), M(a2), M(a3), M(a4)
#define FOR_EACH_SEP_CALL_5(M, a1, a2, a3, a4, a5)                             \
  M(a1), M(a2), M(a3), M(a4), M(a5)
#define FOR_EACH_SEP_CALL_6(M, a1, a2, a3, a4, a5, a6)                         \
  M(a1), M(a2), M(a3), M(a4), M(a5), M(a6)
#define FOR_EACH_SEP_CALL_7(M, a1, a2, a3, a4, a5, a6, a7)                     \
  M(a1), M(a2), M(a3), M(a4), M(a5), M(a6), M(a7)
#define FOR_EACH_SEP_CALL_8(M, a1, a2, a3, a4, a5, a6, a7, a8)                 \
  M(a1), M(a2), M(a3), M(a4), M(a5), M(a6), M(a7), M(a8)

#define FOR_EACH_SEP_CALL(M, ...)                                              \
  GET_FOR_EACH_SEP_CALL(M_NARGS(__VA_ARGS__))(M, __VA_ARGS__)

#define GET_FOR_EACH_SEP_CALL(N) CAT(FOR_EACH_SEP_CALL_, N)


#define FOR_EACH_SEPX_CALL_1(M, X, a1) M(a1)
#define FOR_EACH_SEPX_CALL_2(M, X, a1, a2) M(a1) X M(a2)
#define FOR_EACH_SEPX_CALL_3(M, X, a1, a2, a3) M(a1) X M(a2) X M(a3)
#define FOR_EACH_SEPX_CALL_4(M, X, a1, a2, a3, a4) M(a1) X M(a2) X M(a3) X M(a4)
#define FOR_EACH_SEPX_CALL_5(M, X, a1, a2, a3, a4, a5)                             \
  M(a1) X M(a2) X M(a3) X M(a4) X M(a5)
#define FOR_EACH_SEPX_CALL_6(M,X, a1, a2, a3, a4, a5, a6)                         \
  M(a1) X M(a2) X M(a3) X M(a4) X M(a5) X M(a6)
#define FOR_EACH_SEPX_CALL_7(M, a1, a2, a3, a4, a5, a6, a7)                     \
  M(a1) X M(a2) X M(a3) X M(a4) X M(a5) X M(a6) X M(a7)
#define FOR_EACH_SEPX_CALL_8(M, a1, a2, a3, a4, a5, a6, a7, a8)                 \
  M(a1) X M(a2) X M(a3) X M(a4) X M(a5) X M(a6) X M(a7) X M(a8)

#define FOR_EACH_SEPX_CALL(M, X, ...)                                              \
  GET_FOR_EACH_SEPX_CALL(M_NARGS(__VA_ARGS__))(M, X, __VA_ARGS__)

#define GET_FOR_EACH_SEPX_CALL(N) CAT(FOR_EACH_SEPX_CALL_, N)


// count arguments - ##__VA_ARGS__ is not portable
#define M_NARGS(...)                                                           \
  M_NARGS_(dummy, ##__VA_ARGS__, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define M_NARGS_(_10, _9, _8, _7, _6, _5, _4, _3, _2, _1, N, ...) N

// has arguments - ##__VA_ARGS__ is not portable
#define HAS_ARGS(...)                                                          \
  HAS_ARGS_IMPL(dummy, ##__VA_ARGS__, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0)
#define HAS_ARGS_IMPL(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...) N
#define COMMA_IF_ARGS(...) COMMA_IF_ARGS_IMPL(HAS_ARGS(__VA_ARGS__))
#define COMMA_IF_ARGS_IMPL(has_args) M_CONC(COMMA_IF_ARGS_, has_args)
#define COMMA_IF_ARGS_1 ,
#define COMMA_IF_ARGS_0

// utility (concatenation)
#define M_CONC(A, B) M_CONC_(A, B)
#define M_CONC_(A, B) A##B

#define M_GET_ELEM(N, ...) M_CONC(M_GET_ELEM_, N)(__VA_ARGS__)
#define M_GET_ELEM_0(_0, ...) _0
#define M_GET_ELEM_1(_0, _1, ...) _1
#define M_GET_ELEM_2(_0, _1, _2, ...) _2
#define M_GET_ELEM_3(_0, _1, _2, _3, ...) _3
#define M_GET_ELEM_4(_0, _1, _2, _3, _4, ...) _4
#define M_GET_ELEM_5(_0, _1, _2, _3, _4, _5, ...) _5
#define M_GET_ELEM_6(_0, _1, _2, _3, _4, _5, _6, ...) _6
#define M_GET_ELEM_7(_0, _1, _2, _3, _4, _5, _6, _7, ...) _7
#define M_GET_ELEM_8(_0, _1, _2, _3, _4, _5, _6, _7, _8, ...) _8
#define M_GET_ELEM_9(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, ...) _9
#define M_GET_ELEM_10(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, ...) _10

// Get last argument - placeholder decrements by one
#define M_GET_LAST(...)                                                        \
  M_GET_ELEM(M_NARGS(__VA_ARGS__), _, __VA_ARGS__, , , , , , , , , , , )

#define CAT(a, b) CAT_IMPL(a, b)
#define CAT_IMPL(a, b) a##b

#define STRINGIFY(x) STRINGIFY_IMPL(x)
#define STRINGIFY_IMPL(x) #x

#define FUNC_SIGNATURE(ret, name, ...) ret name(__VA_ARGS__)

#define NUM_ARGS(...) NUM_ARGS_IMPL(__VA_ARGS__, 5, 4, 3, 2, 1, 0)
#define NUM_ARGS_IMPL(_1, _2, _3, _4, _5, N, ...) N

#define TYPED_ARG_0()
#define TYPED_ARG_1(t0) t0 arg0
#define TYPED_ARG_2(t0, t1) t0 arg0, t1 arg1
#define TYPED_ARG_3(t0, t1, t2) t0 arg0, t1 arg1, t2 arg2
#define TYPED_ARG_4(t0, t1, t2, t3) t0 arg0, t1 arg1, t2 arg2, t3 arg3
// Extend as needed...

#define TYPED_ARGS(count, ...) CAT(TYPED_ARG_, count)(__VA_ARGS__)

#define ARG_NAMES_0()
#define ARG_NAMES_1(t0) arg0
#define ARG_NAMES_2(t0, t1) arg0, arg1
#define ARG_NAMES_3(t0, t1, t2) arg0, arg1, arg2
#define ARG_NAMES_4(t0, t1, t2, t3) arg0, arg1, arg2, arg3

#define TEMPLATE_CHAIN(...)                                                    \
  TEMPLATE_CHAIN_DISPATCH(M_NARGS(__VA_ARGS__), __VA_ARGS__)
#define TEMPLATE_CHAIN_DISPATCH(N, ...) CAT(TEMPLATE_CHAIN_, N)(__VA_ARGS__)
#define TEMPLATE_CHAIN_1(t0) t0
#define TEMPLATE_CHAIN_2(t0, t1) t0<t1>
#define TEMPLATE_CHAIN_3(t0, t1, t2) t0<t1<t2>>
#define TEMPLATE_CHAIN_4(t0, t1, t2, t3) t0<t1<t2<t3>>>
#define TEMPLATE_CHAIN_5(t0, t1, t2, t3, t4) t0<t1<t2<t3<t4>>>>
#define TEMPLATE_CHAIN_6(t0, t1, t2, t3, t4, t5) t0<t1<t2<t3<t4<t5>>>>>
#define TEMPLATE_CHAIN_7(t0, t1, t2, t3, t4, t5, t6) t0<t1<t2<t3<t4<t5<t6>>>>>>
#define TEMPLATE_CHAIN_8(t0, t1, t2, t3, t4, t5, t6, t7)                       \
  t0<t1<t2<t3<t4<t5<t6<t7>>>>>>>
#define TEMPLATE_CHAIN_9(t0, t1, t2, t3, t4, t5, t6, t7, t8)                   \
  t0<t1<t2<t3<t4<t5<t6<t7<t8>>>>>>>>
#define TEMPLATE_CHAIN_10(t0, t1, t2, t3, t4, t5, t6, t7, t8, t9)              \
  t0<t1<t2<t3<t4<t5<t6<t7<t8<t9>>>>>>>>>

// Extend as needed...

#define ARG_NAMES(count, ...) CAT(ARG_NAMES_, count)(__VA_ARGS__)

#define ARCHETYPE_METHOD(unique_name, ret, name, ...)                          \
public:                                                                        \
  ret name(TYPED_ARGS(M_NARGS(__VA_ARGS__), __VA_ARGS__)) {                    \
    return _##unique_name##_stub(_obj COMMA_IF_ARGS(__VA_ARGS__) ARG_NAMES(    \
        M_NARGS(__VA_ARGS__), __VA_ARGS__));                                   \
  }

#define CALLSTUB_ASSIGNMENT(unique_name, ret, name, ...)                       \
  _##unique_name##_stub = [](void *obj COMMA_IF_ARGS(__VA_ARGS__) TYPED_ARGS(  \
                              M_NARGS(__VA_ARGS__), __VA_ARGS__)) -> ret {     \
    return static_cast<T *>(obj)->name(                                        \
        ARG_NAMES(M_NARGS(__VA_ARGS__), __VA_ARGS__));                         \
  };

// uses comma swallowing trick
#define CALLSTUB_MEMBER(unique_name, ret, name, ...)                           \
  ret (*_##unique_name##_stub)(void *obj COMMA_IF_ARGS(__VA_ARGS__) __VA_ARGS__);

#define CONCEPT_REQUIREMENT(unique_name, ret, name, ...)                       \
  template <typename, typename = void>                                         \
  struct CAT(has_, unique_name) : std::false_type {};                          \
  template <typename T>                                                        \
  struct CAT(has_, unique_name)<                                               \
      T, std::void_t<decltype(static_cast<ret (T::*)(TYPED_ARGS(               \
                                  M_NARGS(__VA_ARGS__), __VA_ARGS__))>(        \
             &T::name))>> : std::true_type {};

#define CONCEPT_ASSERTION(unique_name, ret, name, ...)                         \
  static_assert(                                                               \
      has_##unique_name<T>::value,                                             \
      STRINGIFY(T must have a method FUNC_SIGNATURE(ret, name, __VA_ARGS__)));

#define DO_ASSERT(x) x::assert<T>();

#define UNIQUE_NAME(base) CAT(CAT(CAT(CAT(base, _), __LINE__), _), __COUNTER__)

#define DEFINE_METHOD(ret, name, ...)                                          \
  (UNIQUE_NAME(name), ret, name, __VA_ARGS__)




#define ARCHETYPE_REQUIREMENT(unique_name, ret, name, ...)\
  static_cast<ret (T::*)(TYPED_ARGS(M_NARGS(__VA_ARGS__), __VA_ARGS__))>(&T::name)


// DEFINE_ARCHETYPE(basic0, (
//   DEFINE_METHOD(void, func0)
// ))





#endif //__ARCHETYPE_H__
