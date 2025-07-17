#include "archetype/archetype.h"
#include <iostream>

// Compose basic types that get composed through multi inheritance
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

// Define interfaces for each
ARCHETYPE_DEFINE(satisfies_a, (ARCHETYPE_METHOD(void, do_a)))
ARCHETYPE_DEFINE(satisfies_b, (ARCHETYPE_METHOD(int, do_b, int)))
ARCHETYPE_DEFINE(satisfies_c, (ARCHETYPE_METHOD(char, do_c, char)))
ARCHETYPE_DEFINE(satisfies_d, (ARCHETYPE_METHOD(double, do_d, double)))

ARCHETYPE_COMPOSE(satisfies_ab, satisfies_a, satisfies_b)

int main()
{

  if (__cplusplus == 202302L)
    std::cout << "C++23";
  else if (__cplusplus == 202002L)
    std::cout << "C++20";
  else if (__cplusplus == 201703L)
    std::cout << "C++17";
  else if (__cplusplus == 201402L)
    std::cout << "C++14";
  else if (__cplusplus == 201103L)
    std::cout << "C++11";
  else if (__cplusplus == 199711L)
    std::cout << "C++98";
  else
    std::cout << "pre-standard C++." << __cplusplus;
  std::cout << "\n";

  std::cout << "creating views to anything that is both an A and a B" << std::endl;

  std::cout << "ab view will" << (satisfies_ab::check<A>::value ? "" : " not")   << " bind to: " << "A" << std::endl;
  std::cout << "ab view will" << (satisfies_ab::check<B>::value ? "" : " not")   << " bind to: " << "B" << std::endl;
  std::cout << "ab view will" << (satisfies_ab::check<C>::value ? "" : " not")   << " bind to: " << "C" << std::endl;
  std::cout << "ab view will" << (satisfies_ab::check<D>::value ? "" : " not")   << " bind to: " << "D" << std::endl;
  std::cout << "ab view will" << (satisfies_ab::check<AB>::value ? "" : " not")  << " bind to: " << "AB" << std::endl;
  std::cout << "ab view will" << (satisfies_ab::check<AC>::value ? "" : " not")  << " bind to: " << "AC" << std::endl;
  std::cout << "ab view will" << (satisfies_ab::check<AD>::value ? "" : " not")  << " bind to: " << "AD" << std::endl;
  std::cout << "ab view will" << (satisfies_ab::check<BC>::value ? "" : " not")  << " bind to: " << "BC" << std::endl;
  std::cout << "ab view will" << (satisfies_ab::check<BD>::value ? "" : " not")  << " bind to: " << "BD" << std::endl;
  std::cout << "ab view will" << (satisfies_ab::check<CD>::value ? "" : " not")  << " bind to: " << "CD" << std::endl;
  std::cout << "ab view will" << (satisfies_ab::check<ABC>::value ? "" : " not") << " bind to: " << "ABC" << std::endl;
  std::cout << "ab view will" << (satisfies_ab::check<ABD>::value ? "" : " not") << " bind to: " << "ABD" << std::endl;
  std::cout << "ab view will" << (satisfies_ab::check<ACD>::value ? "" : " not") << " bind to: " << "ACD" << std::endl;
  std::cout << "ab view will" << (satisfies_ab::check<BCD>::value ? "" : " not") << " bind to: " << "BCD" << std::endl;

  return 0;
}
