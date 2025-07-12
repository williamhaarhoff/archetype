#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "archetype/archetype.h"
#include <doctest/doctest.h>

// Test fixtures for basic checks
struct noarg_func {
  void func0() {}
};

struct noarg_func1 {
  void func1() {}
};

struct arg_func {
  int func0(int a) { return a + 5; }
};

struct arg_func_double {
  double func0(double a) { return a + 5.3; }
};

struct multifunc {
  int func0(int a) { return a + 5; }
  double func1(double a) { return a + 5.3; }
};

struct overloaded_func {
  int func0(int a) { return a + 5; }
  double func0(double a) { return a + 5.3; }
};

// Test fixtures for finding common bases
struct A {
  void do_a(void) {}
};
struct B {
  int do_b(int b) { return b + 5; }
};
struct C {
  char do_c(char c) { return c + 3; }
};
struct D {
  double do_d(double d) { return d + 3.4; }
};

struct AB : public A, public B {};
struct AC : public A, public C {};
struct AD : public A, public D {};
struct BC : public B, public C {};
struct BD : public B, public D {};
struct CD : public C, public D {};

struct ABC : public A, public B, public C {};
struct ABD : public A, public B, public D {};
struct ACD : public A, public C, public D {};
struct BCD : public B, public C, public D {};

// Archetypes for use in tests
ARCHETYPE_DEFINE(basic_void, (ARCHETYPE_METHOD(void, func0)))

ARCHETYPE_DEFINE(basic_int, (ARCHETYPE_METHOD(int, func0, int)))

ARCHETYPE_DEFINE(basic_double, (ARCHETYPE_METHOD(double, func0, double)))

ARCHETYPE_DEFINE(basic_multifunc, (ARCHETYPE_METHOD(int, func0, int),
                                   ARCHETYPE_METHOD(double, func1, double)))

ARCHETYPE_DEFINE(basic_overload, (ARCHETYPE_METHOD(int, func0, int),
                                  ARCHETYPE_METHOD(double, func0, double)))

ARCHETYPE_DEFINE(satisfies_a, (ARCHETYPE_METHOD(void, do_a)))
ARCHETYPE_DEFINE(satisfies_b, (ARCHETYPE_METHOD(int, do_b, int)))
ARCHETYPE_DEFINE(satisfies_c, (ARCHETYPE_METHOD(char, do_c, char)))
ARCHETYPE_DEFINE(satisfies_d, (ARCHETYPE_METHOD(double, do_d, double)))

ARCHETYPE_DEFINE(satisfies_ab_manual,
                 (ARCHETYPE_METHOD(void, do_a), ARCHETYPE_METHOD(int, do_b, int)))

TEST_CASE("ARCHETYPE_DEFINE") {

  noarg_func naf;
  arg_func af;
  arg_func_double afd;
  multifunc m;
  overloaded_func of;

  SUBCASE("no arguments") {
    CHECK(basic_void::check<noarg_func>::value == true);
    CHECK(basic_void::check<noarg_func1>::value == false);
    CHECK(basic_void::check<arg_func>::value == false);
    CHECK(basic_void::check<arg_func_double>::value == false);

    basic_void::view bvv;
    bvv.bind(naf);
    bvv.func0();
  }

  SUBCASE("single int argument") {
    CHECK(basic_int::check<noarg_func>::value == false);
    CHECK(basic_int::check<noarg_func1>::value == false);
    CHECK(basic_int::check<arg_func>::value == true);
    CHECK(basic_int::check<arg_func_double>::value == false);

    basic_int::view biv;
    biv.bind(af);
    biv.func0(5);
  }

  SUBCASE("single double argument") {
    CHECK(basic_double::check<noarg_func>::value == false);
    CHECK(basic_double::check<noarg_func1>::value == false);
    CHECK(basic_double::check<arg_func>::value == false);
    CHECK(basic_double::check<arg_func_double>::value == true);

    basic_double::view bdv;
    bdv.bind(afd);
    bdv.func0(5.4);
  }

  SUBCASE("multi functions") {
    CHECK(basic_multifunc::check<arg_func>::value == false);
    CHECK(basic_multifunc::check<arg_func_double>::value == false);
    CHECK(basic_multifunc::check<multifunc>::value == true);
    CHECK(basic_multifunc::check<overloaded_func>::value == false);

    basic_multifunc::view bmfv;
    bmfv.bind(m);
    bmfv.func0(5);
    bmfv.func1(5.4);
  }

  SUBCASE("overloaded functions") {
    CHECK(basic_overload::check<arg_func>::value == false);
    CHECK(basic_overload::check<arg_func_double>::value == false);
    CHECK(basic_overload::check<multifunc>::value == false);
    CHECK(basic_overload::check<overloaded_func>::value == true);

    basic_overload::view bov;
    bov.bind(of);
    bov.func0(5);
    bov.func0(5.4);
  }

  SUBCASE("common bases of AB") {
    CHECK(satisfies_ab_manual::check<A>::value == false);
    CHECK(satisfies_ab_manual::check<B>::value == false);
    CHECK(satisfies_ab_manual::check<C>::value == false);
    CHECK(satisfies_ab_manual::check<D>::value == false);
    CHECK(satisfies_ab_manual::check<AB>::value == true);
    CHECK(satisfies_ab_manual::check<AC>::value == false);
    CHECK(satisfies_ab_manual::check<AD>::value == false);
    CHECK(satisfies_ab_manual::check<BC>::value == false);
    CHECK(satisfies_ab_manual::check<BD>::value == false);
    CHECK(satisfies_ab_manual::check<CD>::value == false);
    CHECK(satisfies_ab_manual::check<ABC>::value == true);
    CHECK(satisfies_ab_manual::check<ABD>::value == true);
    CHECK(satisfies_ab_manual::check<ACD>::value == false);
    CHECK(satisfies_ab_manual::check<BCD>::value == false);

    basic_overload::view bov;
    bov.bind(of);
    bov.func0(5);
    bov.func0(5.4);
  }
}

// Compose them
ARCHETYPE_COMPOSE(satisfies_ab, satisfies_a, satisfies_b)
ARCHETYPE_COMPOSE(satisfies_ac, satisfies_a, satisfies_c)
ARCHETYPE_COMPOSE(satisfies_ad, satisfies_a, satisfies_d)

// Multilevel composition
ARCHETYPE_COMPOSE(satisfies_abc, satisfies_ab,
                  satisfies_c) // compose invoked twice
ARCHETYPE_COMPOSE(satisfies_abcd, satisfies_abc,
                  satisfies_d) // compose invoked three times

TEST_CASE("ARCHETYPE_COMPOSE") {
  SUBCASE("common bases of AB") {
    CHECK(satisfies_ab::check<A>::value == false);
    CHECK(satisfies_ab::check<B>::value == false);
    CHECK(satisfies_ab::check<C>::value == false);
    CHECK(satisfies_ab::check<D>::value == false);
    CHECK(satisfies_ab::check<AB>::value == true);
    CHECK(satisfies_ab::check<AC>::value == false);
    CHECK(satisfies_ab::check<AD>::value == false);
    CHECK(satisfies_ab::check<BC>::value == false);
    CHECK(satisfies_ab::check<BD>::value == false);
    CHECK(satisfies_ab::check<CD>::value == false);
    CHECK(satisfies_ab::check<ABC>::value == true);
    CHECK(satisfies_ab::check<ABD>::value == true);
    CHECK(satisfies_ab::check<ACD>::value == false);
    CHECK(satisfies_ab::check<BCD>::value == false);
  }
}