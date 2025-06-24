![](docs/archetype.png)

# What?
Archetype allows creating "concept base pointers" to any type that implements a given concept. Providing significantly more reusable, modular, interfaces with less coupling than available
through typical inheritance. Often inheritance gets uses when composition should have been used instead. Archetype was designed to keep inheritance clean and modular without resorting to composition. 

# Features
- Single file, header only library
- zero dependencies
- C++11 compatible
- GCC, Clang, MSVC? compatible
- No dynamic memory allocation
- Fast. Uses static dispatch 
- Highly modular
- Manual vtable generation (automated through macros)
- Concept like type checking with SFINAE
- Ideal for improving flexibility of existing inheritance based interfaces without breaking APIs
  
# Why?

### Pitfalls of inheritance
Inheritance based interfaces are hard to do right. They typically define both an API (normal methods), and an interface (abstract virtuals), and then keep inheriting orthogonal APIs or interfaces. This falls apart quickly, consider the following example:

- `Class AB` should have methods `void a()`, and `int b(const char *)`
- `Class AC` should have methods `void a()`, and `double c(double, double)`
- `Class BC` should have methods `int b(const char *)`, and `double c(double, double)`

Unless we get into multiple inheritance (which has its own problems) there is no way to get
base pointers to all types with method `a()`, and all types with method `b()` etc. 


Here is how Archetype solves this problem:

```cpp
#include "archetype.h"

DEFINE_ARCHETYPE(has_a, ( DEFINE_METHOD(void, a) ))                     
DEFINE_ARCHETYPE(has_b, ( DEFINE_METHOD(int, b, const char *) ))
DEFINE_ARCHETYPE(has_c, ( DEFINE_METHOD(double, c, double, double) ))

class AB { public: void a();            int b(const char *);      };
class AC { public: void a();            double c(double, double); };
class BC { public: int b(const char *); double c(double, double); };

has_b::base<> ab_ref, bc_ref; // type erased base references
ab_ref.bind(AB);
bc_ref.bind(BC);

ab_ref.b("some string"); // call method b of AB
ab_ref.b("some string"); // call method b of AB
```

### Limitations of Static polymorphism
I generally prefer writing templated functions with concept based gatekeeping. However there are cases when there is no template auto deduction, and simple clean APIs now require template parameters to be specified. This is especially true when stuck pre C++17. This becomes heinous when many template parameters are required.

Consider the following example:

```cpp
template <typename T>
class DoTheThing
{
    public:
    void set_logger(T & t) { logger = t; valid = true; }
    void do_the_thing() { if(valid) logger.log("logging"); }

    private:
    bool valid = false;
    T & logger;
};

MyLogger my_logger;
DoTheThing<MyLogger> dtt;
dtt.set_logger(my_logger);
dtt.do_the_thing();

```

Here is how archetype solves this:

```cpp
DEFINE_ARCHETYPE(loggable, ( DEFINE_METHOD(void, log, const char *) ))

class DoTheThing
{
    public:
    template<typename T>
    void set_logger(T & t) { logger.bind(t); valid = true; }
    void do_the_thing() { if(valid) logger.log("logging"); }

    private:
    bool valid = false;
    loggable::base<> logger;
};

MyLogger my_logger;
DoTheThing dtt;             // clean non templated user interface
dtt.set_logger(my_logger);  // auto deduction works even in C++11
dtt.do_the_thing();

```

# Pitfalls of inheritance


















```cpp
#include "archetype.h"
#include "some_writer.h"

DEFINE_ARCHETYPE(writable, ( DEFINE_METHOD(int, write, const char *, size_t) ))

SomeWriter writer_impl;
writable::base<> bind_site;

bind_side.bind(writer_impl);
bind_site.write("hello world", 11);
```






# Why?

I created this library as I couldn't find an existing solution to the following problem:

Refactor this code such that:
- `LibraryClass` can use any class for printing provided they have a public `int write(const char *, size_t)` method. 
- The print API must be consistent between all classes used.
- The print API must not 
- Dynamic memory allocation is not allowed
- Don't break the library class API
- `LibraryClass` must not depend on `Print`


```cpp

// in some_print_impl.h
class Print
{
    int write(const char * buf, size_t n);
    void print_api(const char * buf);
    print_api(int num);
};


class LibraryClass
{
    set_printer(const Print & printer) {p=printer;}
    Print & p;
};

```