#ifndef __ARCHETYPE_H__
#define __ARCHETYPE_H__

#include <type_traits>

//-- Utilities
namespace archetype {

  struct vtable_base 
  {
    template<typename T>
    static vtable_base * make_vtable() {
      static vtable_base vtablet;
      return &vtablet;
    }

    template <typename T>
    void bind() {}
  };

  template<typename VTableType>
  class view_base
  {
    protected:
    void * _obj;
    VTableType * _vtbl;
  };

  template <typename...> // std::void_t - pre c++17
  using void_t = void;

  template<typename Base>
  struct identity : public Base {
    using Base::Base;
  };

  template <class Archetype> 
  struct helper 
  {
    template <typename T = vtable_base>
    using vtable = typename Archetype::template vtable<T>;
    
    template<typename T = view_base<vtable<>>>
    using view_layer = typename Archetype::template view_layer<T>;
  };
} // namespace archetype


//-- API
#define ARCHETYPE_METHOD(ret, name, ...)                                       \
  (ARCH_PP_UNIQUE_NAME(name), ret, name, __VA_ARGS__)

#define ARCHETYPE_CHECK(ARCHETYPE, TYPE)\
  static_assert(ARCHETYPE::check<TYPE>::value, STRINGIFY(TYPE must satisfy ARCHETYPE::check));

#define ARCHETYPE_DEFINE(NAME, METHODS)                                        \
  struct NAME {                                                                \
    NAME() = delete;                                                           \
    ~NAME() = delete;                                                          \
    NAME & operator=(const NAME &) = delete;                                   \
                                                                               \
    friend struct archetype::helper<NAME>;                                      \
                                                                               \
    /* SFINAE based type checking against requirements */                      \
    template <typename, typename = void> struct check : std::false_type {};    \
                                                                               \
    template <typename T>                                                      \
    struct check<                                                              \
      T, archetype::void_t<decltype(ARCH_PP_EXPAND_REQUIREMENTS(METHODS))>>    \
      : std::true_type {};                                                     \
                                                                               \
    /* Internal protected vtable, and view_layer implementation */             \
    protected:                                                                 \
    template <typename BaseVTable = archetype::vtable_base>                    \
    struct vtable : public BaseVTable                                          \
    {                                                                          \
      ARCH_PP_EXPAND_CALLSTUB_MEMBERS(METHODS)                                 \
                                                                               \
      template<typename T>                                                     \
      void bind()                                                              \
      {                                                                        \
        ARCHETYPE_CHECK(NAME, T)                                               \
        this->BaseVTable::template bind<T>();                                  \
        ARCH_PP_EXPAND_CALLSTUB_ASSIGNMENTS(METHODS)                           \
      }                                                                        \
                                                                               \
      template<typename T>                                                     \
      static vtable * make_vtable()                                            \
      {                                                                        \
        static vtable<BaseVTable> vtablet;                                     \
        vtablet.bind<T>();                                                     \
        return &vtablet;                                                       \
      }                                                                        \
                                                                               \
    };                                                                         \
                                                                               \
    template<typename BaseViewLayer = archetype::view_base<vtable<>>>          \
    struct view_layer : public BaseViewLayer                                   \
    {                                                                          \
      ARCH_PP_EXPAND_METHODS(METHODS)                                          \
                                                                               \
      protected:                                                               \
      using BaseViewLayer::_obj;                                               \
      using BaseViewLayer::_vtbl;                                              \
    };                                                                         \
                                                                               \
    /* Public view, and ptr structures */                                      \
    public:                                                                    \
    ARCH_PP_COMMON_BLOCK                                                       \
  };


#define ARCHETYPE_COMPOSE(NAME, ...)                                           \
  struct NAME {                                                                \
    NAME() = delete;                                                           \
    ~NAME() = delete;                                                          \
    NAME & operator=(const NAME &) = delete;                                   \
                                                                               \
    friend struct archetype::helper<NAME>;                                      \
                                                                               \
    /* SFINAE based type checking against requirements */                      \
    template <typename T>                                                      \
    struct check                                                               \
        : std::integral_constant<bool, ARCH_PP_EXPAND_COMPONENT_REQUIREMENTS(  \
                                           __VA_ARGS__)> {};                   \
                                                                               \
    protected:                                                                 \
    template<typename BaseVTable = archetype::vtable_base>                     \
    struct vtable : public ARCH_PP_EXPAND_VTABLE_INHERITANCE(__VA_ARGS__)      \
    {                                                                          \
      using this_base = ARCH_PP_EXPAND_VTABLE_INHERITANCE(__VA_ARGS__);        \
      template<typename T>                                                     \
      void bind()                                                              \
      {                                                                        \
        this->this_base::template bind<T>();                                   \
      }                                                                        \
                                                                               \
      template<typename T>                                                     \
      static vtable * make_vtable()                                            \
      {                                                                        \
        static vtable<BaseVTable> vtablet;                                     \
        vtablet.bind<T>();                                                     \
                                                                               \
        return &vtablet;                                                       \
      }                                                                        \
    };                                                                         \
                                                                               \
    template<typename BaseViewLayer = archetype::view_base<vtable<>>>          \
    struct view_layer: public ARCH_PP_EXPAND_VIEW_LAYER_INHERITANCE(__VA_ARGS__)\
    {                                                                          \
      protected:                                                               \
      using BaseViewLayer::_obj;                                               \
      using BaseViewLayer::_vtbl;                                              \
    };                                                                         \
                                                                               \
    /* Public view, and ptr structures */                                      \
    public:                                                                    \
    ARCH_PP_COMMON_BLOCK                                                       \
  };

//-- High level internal expansions
#define ARCH_PP_COMMON_BLOCK                                                   \
  struct view : public view_layer<>                                            \
  {                                                                            \
    template<typename T>                                                       \
    view(T & t)                                                                \
    {                                                                          \
      this->_obj = static_cast<void *>(&t);                                    \
      this->_vtbl = vtable<>::make_vtable<T>();                                \
    }                                                                          \
  };                                                                           \
                                                                               \
  template <template <typename> class API = archetype::identity>         \
  struct ptr                                                                   \
  {                                                                            \
    template<typename T>                                                       \
    ptr(T & t) : _view(t) {}                                                   \
                                                                               \
    API<view> &operator*() { return &_view; }                                  \
    const API<view> &operator*() const { return &_view; }                      \
    API<view> *operator->() { return &_view; }                                 \
    const API<view> *operator->() const { return &_view; }                     \
                                                                               \
    protected:                                                                 \
    API<view> _view;                                                           \
  };

#define ARCH_PP_EXPAND_METHODS(METHODS) ARCH_PP_EXPAND_METHODS_IMPL METHODS

#define ARCH_PP_EXPAND_METHODS_IMPL(...)                                       \
  ARCH_PP_FOR_EACH(ARCH_PP_METHOD, __VA_ARGS__)

#define ARCH_PP_EXPAND_CALLSTUB_ASSIGNMENTS(METHODS)                           \
  ARCH_PP_EXPAND_CALLSTUB_ASSIGNMENTS_IMPL METHODS

#define ARCH_PP_EXPAND_CALLSTUB_ASSIGNMENTS_IMPL(...)                          \
  ARCH_PP_FOR_EACH(ARCH_PP_CALLSTUB_ASSIGNMENT, __VA_ARGS__)

#define ARCH_PP_EXPAND_CALLSTUB_MEMBERS(METHODS)                               \
  ARCH_PP_EXPAND_CALLSTUB_MEMBERS_IMPL METHODS

#define ARCH_PP_EXPAND_CALLSTUB_MEMBERS_IMPL(...)                              \
  ARCH_PP_FOR_EACH(ARCH_PP_CALLSTUB_MEMBER, __VA_ARGS__)

#define ARCH_PP_EXPAND_REQUIREMENTS(METHODS)                                   \
  ARCH_PP_EXPAND_REQUIREMENTS_IMPL METHODS

#define ARCH_PP_EXPAND_REQUIREMENTS_IMPL(...)                                  \
  ARCH_PP_FOR_EACH_SEP(ARCH_PP_REQUIREMENT, __VA_ARGS__)

#define ARCH_PP_EXPAND_VTABLE_INHERITANCE(...)                              \
  ARCH_PP_EXPAND_VTABLE_INHERITANCE_IMPL(                                   \
      ARCH_PP_FOR_EACH_SEP_CALL(ARCH_PP_APPLY_VTABLE_HELPER, __VA_ARGS__))

#define ARCH_PP_EXPAND_VTABLE_INHERITANCE_IMPL(...)                         \
  ARCH_PP_TEMPLATE_CHAIN(__VA_ARGS__ ARCH_PP_COMMA_IF_ARGS(__VA_ARGS__) BaseVTable)

#define ARCH_PP_EXPAND_VIEW_LAYER_INHERITANCE(...)                              \
  ARCH_PP_EXPAND_VIEW_LAYER_INHERITANCE_IMPL(                                   \
      ARCH_PP_FOR_EACH_SEP_CALL(ARCH_PP_APPLY_VIEW_LAYER_HELPER, __VA_ARGS__))

#define ARCH_PP_EXPAND_VIEW_LAYER_INHERITANCE_IMPL(...)                         \
  ARCH_PP_TEMPLATE_CHAIN(__VA_ARGS__ ARCH_PP_COMMA_IF_ARGS(__VA_ARGS__) BaseViewLayer)

#define ARCH_PP_EXPAND_COMPONENT_REQUIREMENTS(...)                             \
  ARCH_PP_FOR_EACH_SEPX_CALL(ARCH_PP_APPEND_CHECK, &&, __VA_ARGS__)

//-- Low level internal expressions
#define ARCH_PP_METHOD(ARCH_PP_UNIQUE_NAME, ret, name, ...)                    \
public:                                                                        \
  ret name(TYPED_ARGS(M_NARGS(__VA_ARGS__), __VA_ARGS__)) {                    \
    return _vtbl->_##ARCH_PP_UNIQUE_NAME##_stub(_obj ARCH_PP_COMMA_IF_ARGS(           \
        __VA_ARGS__) ARCH_PP_ARG_NAMES(M_NARGS(__VA_ARGS__), __VA_ARGS__));    \
  }

#define ARCH_PP_CALLSTUB_ASSIGNMENT(ARCH_PP_UNIQUE_NAME, ret, name, ...)       \
  _##ARCH_PP_UNIQUE_NAME##_stub =                                              \
      [](void *obj ARCH_PP_COMMA_IF_ARGS(__VA_ARGS__)                          \
             TYPED_ARGS(M_NARGS(__VA_ARGS__), __VA_ARGS__)) -> ret {           \
    return static_cast<T *>(obj)->name(                                        \
        ARCH_PP_ARG_NAMES(M_NARGS(__VA_ARGS__), __VA_ARGS__));                 \
  };

#define ARCH_PP_CALLSTUB_MEMBER(ARCH_PP_UNIQUE_NAME, ret, name, ...)           \
  ret (*_##ARCH_PP_UNIQUE_NAME##_stub)(                                        \
      void *obj ARCH_PP_COMMA_IF_ARGS(__VA_ARGS__) __VA_ARGS__);

#define ARCH_PP_UNIQUE_NAME(base)                                              \
  ARCH_PP_CAT(ARCH_PP_CAT(ARCH_PP_CAT(ARCH_PP_CAT(base, _), __LINE__), _),     \
              __COUNTER__)

#define ARCH_PP_REQUIREMENT(ARCH_PP_UNIQUE_NAME, ret, name, ...)               \
  static_cast<ret (T::*)(TYPED_ARGS(M_NARGS(__VA_ARGS__), __VA_ARGS__))>(      \
      &T::name)

#define ARCH_PP_APPEND_CHECK(x) x::check<T>::value
#define ARCH_PP_APPLY_VTABLE_HELPER(x) archetype::helper<x>::vtable
#define ARCH_PP_APPLY_VIEW_LAYER_HELPER(x) archetype::helper<x>::view_layer

//-- Foundational macro utilities
#define ARCH_PP_EXPAND(x) x

#define ARCH_PP_FOR_EACH(M, ...)                                               \
  ARCH_PP_EXPAND(ARCH_PP_GET_MACRO(__VA_ARGS__, FE10, FE9, FE8, FE7, FE6, FE5, FE4,    \
                           FE3, FE2, FE1)(M, __VA_ARGS__))

#define ARCH_PP_GET_MACRO(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, NAME, ...) NAME

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

#define ARCH_PP_FOR_EACH_SEP(M, ...)                                           \
  ARCH_PP_EXPAND(ARCH_PP_GET_MACRO(__VA_ARGS__, FES10, FES9, FES8, FES7, FES6, FES5,   \
                           FES4, FES3, FES2, FES1)(M, __VA_ARGS__))

#define FES1(M, x) M x
#define FES2(M, x, ...) M x, FE1(M, __VA_ARGS__)
#define FES3(M, x, ...) M x, FE2(M, __VA_ARGS__)
#define FES4(M, x, ...) M x, FE3(M, __VA_ARGS__)
#define FES5(M, x, ...) M x, FE4(M, __VA_ARGS__)
#define FES6(M, x, ...) M x, FE5(M, __VA_ARGS__)
#define FES7(M, x, ...) M x, FE6(M, __VA_ARGS__)
#define FES8(M, x, ...) M x, FE7(M, __VA_ARGS__)
#define FES9(M, x, ...) M x, FE8(M, __VA_ARGS__)
#define FES10(M, x, ...) M x, FE9(M, __VA_ARGS__)

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

#define ARCH_PP_FOR_EACH_CALL_1(M, a1) M(a1)
#define ARCH_PP_FOR_EACH_CALL_2(M, a1, a2) M(a1) M(a2)
#define ARCH_PP_FOR_EACH_CALL_3(M, a1, a2, a3) M(a1) M(a2) M(a3)
#define ARCH_PP_FOR_EACH_CALL_4(M, a1, a2, a3, a4) M(a1) M(a2) M(a3) M(a4)
#define ARCH_PP_FOR_EACH_CALL_5(M, a1, a2, a3, a4, a5)                         \
  M(a1) M(a2) M(a3) M(a4) M(a5)
#define ARCH_PP_FOR_EACH_CALL_6(M, a1, a2, a3, a4, a5, a6)                     \
  M(a1) M(a2) M(a3) M(a4) M(a5) M(a6)
#define ARCH_PP_FOR_EACH_CALL_7(M, a1, a2, a3, a4, a5, a6, a7)                 \
  M(a1) M(a2) M(a3) M(a4) M(a5) M(a6) M(a7)
#define ARCH_PP_FOR_EACH_CALL_8(M, a1, a2, a3, a4, a5, a6, a7, a8)             \
  M(a1) M(a2) M(a3) M(a4) M(a5) M(a6) M(a7) M(a8)

#define ARCH_PP_FOR_EACH_CALL(M, ...)                                          \
  ARCH_PP_GET_FOR_EACH_CALL(M_NARGS(__VA_ARGS__))(M, __VA_ARGS__)

#define ARCH_PP_GET_FOR_EACH_CALL(N) ARCH_PP_CAT(ARCH_PP_FOR_EACH_CALL_, N)

#define ARCH_PP_FOR_EACH_SEP_CALL_1(M, a1) M(a1)
#define ARCH_PP_FOR_EACH_SEP_CALL_2(M, a1, a2) M(a1), M(a2)
#define ARCH_PP_FOR_EACH_SEP_CALL_3(M, a1, a2, a3) M(a1), M(a2), M(a3)
#define ARCH_PP_FOR_EACH_SEP_CALL_4(M, a1, a2, a3, a4)                         \
  M(a1), M(a2), M(a3), M(a4)
#define ARCH_PP_FOR_EACH_SEP_CALL_5(M, a1, a2, a3, a4, a5)                     \
  M(a1), M(a2), M(a3), M(a4), M(a5)
#define ARCH_PP_FOR_EACH_SEP_CALL_6(M, a1, a2, a3, a4, a5, a6)                 \
  M(a1), M(a2), M(a3), M(a4), M(a5), M(a6)
#define ARCH_PP_FOR_EACH_SEP_CALL_7(M, a1, a2, a3, a4, a5, a6, a7)             \
  M(a1), M(a2), M(a3), M(a4), M(a5), M(a6), M(a7)
#define ARCH_PP_FOR_EACH_SEP_CALL_8(M, a1, a2, a3, a4, a5, a6, a7, a8)         \
  M(a1), M(a2), M(a3), M(a4), M(a5), M(a6), M(a7), M(a8)

#define ARCH_PP_FOR_EACH_SEP_CALL(M, ...)                                      \
  ARCH_PP_GET_FOR_EACH_SEP_CALL(M_NARGS(__VA_ARGS__))(M, __VA_ARGS__)

#define ARCH_PP_GET_FOR_EACH_SEP_CALL(N)                                       \
  ARCH_PP_CAT(ARCH_PP_FOR_EACH_SEP_CALL_, N)

#define ARCH_PP_FOR_EACH_SEPX_CALL_1(M, X, a1) M(a1)
#define ARCH_PP_FOR_EACH_SEPX_CALL_2(M, X, a1, a2) M(a1) X M(a2)
#define ARCH_PP_FOR_EACH_SEPX_CALL_3(M, X, a1, a2, a3)                         \
  M(a1) X M(a2)                                                                \
  X M(a3)
#define ARCH_PP_FOR_EACH_SEPX_CALL_4(M, X, a1, a2, a3, a4)                     \
  M(a1) X M(a2)                                                                \
  X M(a3)                                                                      \
  X M(a4)
#define ARCH_PP_FOR_EACH_SEPX_CALL_5(M, X, a1, a2, a3, a4, a5)                 \
  M(a1) X M(a2)                                                                \
  X M(a3)                                                                      \
  X M(a4)                                                                      \
  X M(a5)
#define ARCH_PP_FOR_EACH_SEPX_CALL_6(M, X, a1, a2, a3, a4, a5, a6)             \
  M(a1) X M(a2)                                                                \
  X M(a3)                                                                      \
  X M(a4)                                                                      \
  X M(a5)                                                                      \
  X M(a6)
#define ARCH_PP_FOR_EACH_SEPX_CALL_7(M, a1, a2, a3, a4, a5, a6, a7)            \
  M(a1) X M(a2)                                                                \
  X M(a3)                                                                      \
  X M(a4)                                                                      \
  X M(a5)                                                                      \
  X M(a6)                                                                      \
  X M(a7)
#define ARCH_PP_FOR_EACH_SEPX_CALL_8(M, a1, a2, a3, a4, a5, a6, a7, a8)        \
  M(a1) X M(a2)                                                                \
  X M(a3)                                                                      \
  X M(a4)                                                                      \
  X M(a5)                                                                      \
  X M(a6)                                                                      \
  X M(a7)                                                                      \
  X M(a8)

#define ARCH_PP_FOR_EACH_SEPX_CALL(M, X, ...)                                  \
  ARCH_PP_GET_FOR_EACH_SEPX_CALL(M_NARGS(__VA_ARGS__))(M, X, __VA_ARGS__)

#define ARCH_PP_GET_FOR_EACH_SEPX_CALL(N)                                      \
  ARCH_PP_CAT(ARCH_PP_FOR_EACH_SEPX_CALL_, N)

// count arguments - ##__VA_ARGS__ is not portable
#define M_NARGS(...)                                                           \
  M_NARGS_(dummy, ##__VA_ARGS__, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define M_NARGS_(_10, _9, _8, _7, _6, _5, _4, _3, _2, _1, N, ...) N

// has arguments - ##__VA_ARGS__ is not portable
#define HAS_ARGS(...)                                                          \
  HAS_ARGS_IMPL(dummy, ##__VA_ARGS__, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0)
#define HAS_ARGS_IMPL(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...) N
#define ARCH_PP_COMMA_IF_ARGS(...)                                             \
  ARCH_PP_COMMA_IF_ARGS_IMPL(HAS_ARGS(__VA_ARGS__))
#define ARCH_PP_COMMA_IF_ARGS_IMPL(has_args)                                   \
  M_CONC(ARCH_PP_COMMA_IF_ARGS_, has_args)
#define ARCH_PP_COMMA_IF_ARGS_1 ,
#define ARCH_PP_COMMA_IF_ARGS_0

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

#define ARCH_PP_CAT(a, b) ARCH_PP_CAT_IMPL(a, b)
#define ARCH_PP_CAT_IMPL(a, b) a##b

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

#define TYPED_ARGS(count, ...) ARCH_PP_CAT(TYPED_ARG_, count)(__VA_ARGS__)

#define ARCH_PP_ARG_NAMES_0()
#define ARCH_PP_ARG_NAMES_1(t0) arg0
#define ARCH_PP_ARG_NAMES_2(t0, t1) arg0, arg1
#define ARCH_PP_ARG_NAMES_3(t0, t1, t2) arg0, arg1, arg2
#define ARCH_PP_ARG_NAMES_4(t0, t1, t2, t3) arg0, arg1, arg2, arg3

#define ARCH_PP_TEMPLATE_CHAIN(...)                                            \
  ARCH_PP_TEMPLATE_CHAIN_DISPATCH(M_NARGS(__VA_ARGS__), __VA_ARGS__)
#define ARCH_PP_TEMPLATE_CHAIN_DISPATCH(N, ...)                                \
  ARCH_PP_CAT(ARCH_PP_TEMPLATE_CHAIN_, N)(__VA_ARGS__)
#define ARCH_PP_TEMPLATE_CHAIN_1(t0) t0
#define ARCH_PP_TEMPLATE_CHAIN_2(t0, t1) t0<t1>
#define ARCH_PP_TEMPLATE_CHAIN_3(t0, t1, t2) t0<t1<t2>>
#define ARCH_PP_TEMPLATE_CHAIN_4(t0, t1, t2, t3) t0<t1<t2<t3>>>
#define ARCH_PP_TEMPLATE_CHAIN_5(t0, t1, t2, t3, t4) t0<t1<t2<t3<t4>>>>
#define ARCH_PP_TEMPLATE_CHAIN_6(t0, t1, t2, t3, t4, t5) t0<t1<t2<t3<t4<t5>>>>>
#define ARCH_PP_TEMPLATE_CHAIN_7(t0, t1, t2, t3, t4, t5, t6)                   \
  t0<t1<t2<t3<t4<t5<t6>>>>>>
#define ARCH_PP_TEMPLATE_CHAIN_8(t0, t1, t2, t3, t4, t5, t6, t7)               \
  t0<t1<t2<t3<t4<t5<t6<t7>>>>>>>
#define ARCH_PP_TEMPLATE_CHAIN_9(t0, t1, t2, t3, t4, t5, t6, t7, t8)           \
  t0<t1<t2<t3<t4<t5<t6<t7<t8>>>>>>>>
#define ARCH_PP_TEMPLATE_CHAIN_10(t0, t1, t2, t3, t4, t5, t6, t7, t8, t9)      \
  t0<t1<t2<t3<t4<t5<t6<t7<t8<t9>>>>>>>>>

#define ARCH_PP_ARG_NAMES(count, ...)                                          \
  ARCH_PP_CAT(ARCH_PP_ARG_NAMES_, count)(__VA_ARGS__)

#endif //__ARCHETYPE_H__
