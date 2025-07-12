#include "archetype/archetype.h"
#include <iostream>
#include <vector>

ARCHETYPE_DEFINE(basic_overload, (ARCHETYPE_METHOD(int, func0, int),
                                  ARCHETYPE_METHOD(double, func0, double)))

ARCHETYPE_DEFINE(satisfies_a, (ARCHETYPE_METHOD(void, do_a)))
ARCHETYPE_DEFINE(satisfies_b, (ARCHETYPE_METHOD(int, do_b, int)))
ARCHETYPE_DEFINE(satisfies_c, (ARCHETYPE_METHOD(char, do_c, char)))
ARCHETYPE_DEFINE(satisfies_d, (ARCHETYPE_METHOD(double, do_d, double)))

ARCHETYPE_DEFINE(satisfies_ab_manual,
                 (ARCHETYPE_METHOD(void, do_a), ARCHETYPE_METHOD(int, do_b, int)))

ARCHETYPE_COMPOSE(satisfies_ab, satisfies_a, satisfies_b)
ARCHETYPE_COMPOSE(satisfies_ad, satisfies_a, satisfies_d)

ARCHETYPE_COMPOSE(satisfies_abc_alt, satisfies_ab, satisfies_c)

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

int main() {

  // A a;
  // B b;
  // C c;
  // D d;
  AB ab;
  // AC ac;
  AD ad;
  // BC bc;
  // BD bd;
  // CD cd;
  ABC abc;
  ABD abd;
  ACD acd;
  // BCD bcd;

  std::vector<satisfies_ab::view> views_ab(3);
  views_ab[0].bind(ab);
  views_ab[1].bind(abc);
  views_ab[2].bind(abd);

  for (auto &view : views_ab) {
    view.do_a();
    view.do_b(5);
  }

  std::vector<satisfies_ad::view> views_ad(3);
  views_ad[0].bind(ad);
  views_ad[1].bind(abd);
  views_ad[2].bind(acd);

  for (auto &view : views_ad) {
    view.do_a();
    view.do_d(3.0);
  }

  std::cout << "Macro expansion worked" << std::endl;
  return 0;
}