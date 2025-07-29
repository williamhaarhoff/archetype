# Archetype

[![GCC Build](https://github.com/williamhaarhoff/archetype/actions/workflows/ubuntu.yml/badge.svg)](https://github.com/williamhaarhoff/archetype/actions/workflows/ubuntu.yml)
[![Clang Build](https://github.com/williamhaarhoff/archetype/actions/workflows/macos.yml/badge.svg)](https://github.com/williamhaarhoff/archetype/actions/workflows/macos.yml)
[![MSVC Build](https://github.com/williamhaarhoff/archetype/actions/workflows/windows.yml/badge.svg)](https://github.com/williamhaarhoff/archetype/actions/workflows/windows.yml)

![](docs/archetype.png)


> **Concept based type erasure for C++11 without inheritance or dynamic
> allocation.**

**Archetype** is a lightweight header only library for creating **type erased**,
**concept driven interfaces** without traditional inheritance, or dynamic
memory. It enables **modular**, **reusable** and **low coupling** APIs by
letting you bind objects to **archetype** _views_ which are compile time verified
interface specifications. Internally **Archetype** automatically generates manual vtables, which allow binding to any object meeting the interface specification. 

This makes **Archetype** a practical alternative to **inheritance**,
**std::function**, or **CRTP**, especially when refactoring legacy systems or
building portable, flexible libraries.


## Features

- Single file, header only library
- Zero dependencies
- No dynamic memory allocation
- No virtuals
- C++11 Compatible
- GCC 4.8.1+, Clang 3.3+, MSVC 16.5+(/Zc:preprocessor) compatible
- SFINAE based concept checking
- Works with existing types (no base class required)
- Composable interfaces (build _views_ from parts)
- Great for embedded, and plugins


## Why Archetype?
Archetype provides a simple, clean, and extremely flexible way to define type erased interfaces, giving you a common way to interact with polymorphic types. 


Inheritance is often misused when **composition** or **concepts** would have
been more appropriate. However, alternatives to inheritance often mean losing
the ability to use clean, type erased interfaces.

Archetype fills this gap:

- Like inheritance, it allows **clean interface substitution**
- Like composition, it avoids rigid hierarchies
- Like concepts, it expresses **interface intent** clearly
- Like std::function it allows **type erased binding**


## The Inheritance Problem

Inheritance gives you base pointers and dynamic dispatch but at a cost. 

### Example

Suppose you want to reuse parts of classes `A`, `B`, and `C`.

```cpp
class A { void a(); };
class B { int b(int); };
class C { double c(double); };

class AB : public A, public B {};
class AC : public A, public C {};
class BC : public B, public C {};
```
We can refer `AB` and `AC` with an `A` base pointer (common interface). 
Or `AC` and `BC` with a `C`base pointer. 

But if we want to refer to any object that implements both `A` and
`C` like `ABC` or `ACD`, there isn't a common interface.

With inheritance you:
- Lose composability
- Struggle to find a common base
- Risk coupling, and rigid hierarchies
- Risk diamond problems (in multiple inheritance)

With composition:
- You can't refer to composites polymorphically
- There's no base interface, unless you add one manually

With other techiques like **CRTP** or **concepts** we still loose the ability to refer
to different types polymorphically ie: we can't create `std::vector<common_interface>`

## Archetypes/Quickstart

**Archetype** gives you **type erased** views over objects that _implement_ a
particular interface without requiring them to inherit from anything.

### Define an interface:

```cpp
#include "archetype.h"
ARCHETYPE_DEFINE(archetype_a, ( ARCHETYPE_METHOD(void, a) ))
ARCHETYPE_DEFINE(archetype_b, ( ARCHETYPE_METHOD(int, b, int) ))
ARCHETYPE_DEFINE(archetype_c, ( ARCHETYPE_METHOD(double, c, double) ))
```

### Compose multiple interfaces:

```cpp
ARCHETYPE_COMPOSE(archetype_ab, archetype_a, archetype_b)
ARCHETYPE_COMPOSE(archetype_ac, archetype_a, archetype_c)
```

### Bind any compatible object:

```cpp
ABC abc;
ABD abd;

archetype_ab::view abc_view(abc);
archetype_ab::view abd_view(abd);
```

### Use the interface:
```cpp
abc_view.a();
abd_view.b(5);
```

### Alternativley use a pointer style view:
```cpp
archetype_ab::ptr<> abc_view_ptr(abc);
abc_view_ptr->a();
abc_view_ptr->b(5);
```

You now have a **type erased**, **composable**, **low overhead** reference to
any object that implements* the interface regardless of its type or hierarchy. 
In this case we created views that can view the common `AB` parts of `ABC` and `ABD`

## How Archetype Compares

| Feature                          | Inheritance  | CRTP | std::function | Archetype        |
| -------------------------------- | ------------ | ---- | ------------- | ---------------- |
| **Type-erased interface**        | ✅           | ❌   | ✅            | ✅               |
| **Zero dynamic allocation**      | ✅           | ✅   | ❌            | ✅               |
| **Works with existing types**    | ❌           | ❌   | ✅            | ✅               |
| **Composable interfaces**        | ❌           | ✅   | ❌            | ✅               |
| **No base class required**       | ❌           | ❌   | ✅            | ✅               |
| **Runtime polymorphism**         | ✅           | ❌   | ✅            | ✅ (type-erased) |
| **Compile-time safety (SFINAE)** | ❌           | ✅   | ❌            | ✅               |
| **Supports mixin views**         | ❌           | ✅   | ❌            | ✅               |
| **Header-only**                  | ✅           | ✅   | ✅            | ✅               |


## Patterns That Work Well With Archetype

### 1. Split APIs and Dependencies

Define APIs that depend on interfaces, not on implementations.

```cpp
ARCHETYPE_DEFINE(loggable, ( ARCHETYPE_METHOD(void, log, const char *) ))

class DoTheThing 
{
  bool valid = false;
  loggable::view logger;
  
  public:
    void do_the_thing() { 
      if(valid) logger.log("doing the thing"); 
    }

  template<typename T>
  void set_logger(T & t) { 
    valid = loggable::check<T>::value;
    logger = loggable::view(t);
  } 
};
```

Now `DoTheThing` doesn't need to be templated, or depend on a base class, or
invoke dynamic memory allocation, and can work with any _logger_

### 2. Composable Stateless Mixin Views

Mixins allow modular, composable extension of classes by deriving from them.
Their biggest disadvantage is naming conflicts, and unexpected behaviour due to
collisions with their base class. Archetype views isolate the bound object from
the mixin, and only allow interaction through the specified interface.

```cpp
ARCHETYPE_DEFINE(writable, ( ARCHETYPE_METHOD(int, write, const char *, int)))
ARCHETYPE_DEFINE(readable, ( ARCHETYPE_METHOD(int, read, char *, int)))

template <class W> class WriteAPI : public W {
public:
  ARCHETYPE_CHECK(writable, W)
  using W::W;
  using W::write;
  void write_api(const char *buf) {
    this->write(buf, size);
  }
};

template <class R> class ReadAPI : public R {
public:
  ARCHETYPE_CHECK(readable, R)
  using R::R;
  using R::read;
  int read_api(char *buf, int size) {
    return this->read(buf, size);
  }
};
```

Then use archetypes to bind bindable types, to create type erased views of other
objects.

```cpp
ARCHETYPE_COMPOSE(readwritable, writable, readable)

class MyReadWriter { 
  public: 
  int write(const char *, int);
  int read(char *, int); 
};

MyReadWriter my_read_writer;
WriteAPI<ReadAPI<readwritable::view>> my_view;
my_view(my_read_writer);
my_view.write_api("using the write api on my_read_writer");
char buf[4096];
my_view.read_api(buf, sizeof(buf));
```

Or sateless mixins can also be reused directly without instantiating an
archetype view.

```cpp
writable::assert(my_read_writer);
using WriteView = WriteAPI<MyReadWriter>;
WriteView * write_view = static_cast<WriteView*>(&my_read_writer);
write_view->write("no view objects were instantiated");
```

_Caveat_: In doing this, the mixins must not have virtuals, member variables, or
conflict with the base.

### 3. Stateful Mixin Views

Since `archetype::view` is a real object, you can create stateful views.

```cpp
template <class W> class StateFullWriteAPI : public W {
public:
  using W::W;
  using W::write;
  int write_api(char *buf, int size) {
    this->write(buf, size);
    count++;
    return count;
  }
private:
  int count = 0;
};

StateFullWriteAPI<writable::view> stateful_ref(rw_instance);

int num_writes = stateful_ref.write_api("stateful writing");
```

# What Happens On Error?

If a type bound to an archetype doesn’t implement all required methods, the code
will fail at compile time with a clear SFINAE-based error. Archetype provides 
compile time checking on `bind()` but also provides an `ARCHETYPE_CHECK(archetype, T)` macro for verifying type `T`, and a templated check<T> struct that can be leveraged for SFINAE based type checking. 

# Install

Just drop `archetype.h` into your project. Note that if you are compiling with MSVC, you will need to use the `/Zc:preprocessor` compiler options to use c99 compliant preprocessing.

# Philosophy

Archetype doesn’t force you to change how you build your types. Instead it lets
you view them through modular, lightweight interfaces.


# Who is this for?

If you're building portable libraries, embedded systems, plugin based
frameworks, or trying to untangle brittle inheritance hierarchies Archetype is
for you.


# How does it work:

Internally archetype creates manual vtables. The manual part is automated through the macro API. 

For indepth details on how Archetype was implemented [how_it_works.md](docs/how_it_works.md)
