# Archetype

[![GCC Build](https://github.com/williamhaarhoff/archetype/actions/workflows/nix.yml/badge.svg)](https://github.com/williamhaarhoff/archetype/actions/workflows/nix.yml)
[![MSVC Build](https://github.com/williamhaarhoff/archetype/actions/workflows/windows.yml/badge.svg)](https://github.com/williamhaarhoff/archetype/actions/workflows/windows.yml)

![](docs/archetype.png)

## WARNING -- STILL UNDER CONSTRUCTION

> **Concept based type erasure for C++11 without inheritance or dynamic
> allocation.**

**Archetype** is a lightweight header only library for creating **type erased**,
**concept driven interfaces** without traditional inheritance, or dynamic
memory. It enables **modular**, **reusable** and **low coupling** APIs by
letting you bind objects to **archetypes** which are compile time verified
interface specifications.

This makes **Archetype** a practical alternative to **inheritance**,
**std::function**, or **CRTP**, especially when refactoring legacy systems or
building portable, flexible libraries.

## Features

- Single file, header only library
- Zero dependencies
- No dynamic memory allocation
- Static dispatch (no virtuals)
- C++11 Compatible
- GCC, Clang, MSVC compatible
- SFINAE based concept checking
- Works with existing types (no base class required)
- Composable interfaces (build _views_ from parts)
- Great for embedded, plugin, and systems-level code

## Why Archetype?

Inheritance is often overused when **composition** or **concepts** would have
been more appropriate. However, alternatives to inheritance often mean losing
the ability to use clean, type erased interfaces.

Archetype fills this gap:

- Like inheritance, it allows **clean interface substitution**
- Like composition, it avoids rigid hierarchies
- Like concepts, it expresses **interface intent** clearly
- Like std::function it allows **type erased binding**
- But unlike any of these **TODO**

## The Inheritance Problem

Inheritance gives you base pointers and dynamic dispatch, but at a cost:

### Example

Suppose you want to reuse parts of `A`, `B`, `C`.

```cpp
class A { void a(); };
class B { int b(int); };
class C { double c(double); };

class AB : public A, public B {};
class AC : public A, public C {};
class BC : public B, public C {};
```

We can refer `AB` and `AC` with an `A` base pointer. Or `AC` and `BC` with a `C`
base pointer. But if we want to refer to any object that implements both `A` and
`C` like `ABC` or `ACD`?

With inheritance you:

- Lose composability
- Struggle to find a common base
- Risk coupling, and rigid hierarchies
- Risk diamond problems (in multiple inheritance)

With composition:

- You can't refer to composites polymorphically
- There's no base interface, unless you add one manually

## Archetypes

**Archetype** gives you **type erased** views over objects that _implement_ a
particular interface without requiring them to inherit from anything.

### Define an interface:

```cpp
#include "archetype.h"
DEFINE_ARCHETYPE(archetype_a, ( DEFINE_METHOD(void, a) ))
DEFINE_ARCHETYPE(archetype_b, ( DEFINE_METHOD(int, b, int) ))
DEFINE_ARCHETYPE(archetype_c, ( DEFINE_METHOD(double, c, double) ))
```

### Compose multiple interfaces:

```cpp
COMPOSE_ARCHETYPE(archetype_ab, archetype_a, archetype_b)
COMPOSE_ARCHETYPE(archetype_ac, archetype_a, archetype_c)
```

### Bind any compatible object:

```cpp
ABC abc;
ABD abd;

archetype_ab::view abc_view, abd_view;
abc_view.bind(abc);
abd_view.bind(abd);

abc_view.a();
abd_view.b(5);
```

You now have a **type erased**, **composable**, **low overhead** reference to
any object that implements* the interface regardless of its type or hierarchy.

## How Archetype Compares

| Feature                          | Inheritance  | CRTP | std::function | Archetype        |
| -------------------------------- | ------------ | ---- | ------------- | ---------------- |
| **Type-erased interface**        | ✅           | ❌   | ✅            | ✅               |
| **Zero dynamic allocation**      | ✅           | ✅   | ❌            | ✅               |
| **Works with existing types**    | ❌           | ❌   | ✅            | ✅               |
| **Composable interfaces**        | ❌           | ✅   | ❌            | ✅               |
| **No base class required**       | ❌           | ❌   | ✅            | ✅               |
| **Static dispatch**              | ❌ (virtual) | ✅   | ❌            | ✅               |
| **Runtime polymorphism**         | ✅           | ❌   | ✅            | ✅ (type-erased) |
| **Compile-time safety (SFINAE)** | ❌           | ✅   | ❌            | ✅               |
| **Supports mixin views**         | ❌           | ✅   | ❌            | ✅               |
| **Header-only**                  | ✅           | ✅   | ✅            | ✅               |

TODO - double check this table

## Patterns That Work Well With Archetype

### 1. Split APIs and Dependencies

Define APIs that depend on interfaces, not on implementations.

```cpp
DEFINE_ARCHETYPE(loggable, ( DEFINE_METHOD(void, log, const char *) ))

class DoTheThing 
{
  bool valid = false;
  loggable::view logger;
  
  public:
    void do_the_thing() { 
      if(valid) logger.log("doing the thing"); 
    }

  template<typename T>
  void set_logger(T & t) { valid = logger.bind(t); } //TODO - check this
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
DEFINE_ARCHETYPE(writable, ( DEFINE_METHOD(int, write, const char *, int)))
DEFINE_ARCHETYPE(readable, ( DEFINE_METHOD(int, read, char *, int)))

template <class W> class WriteAPI : public W {
public:
  using W::W;
  using W::write;
  void write_api(const char *buf) {
    this->write(buf, size);
  }
};

template <class R> class ReadAPI : public R {
public:
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
COMPOSE_ARCHETYPE(readwritable, writable, readable)

class MyReadWriter { 
  public: 
  int write(const char *, int);
  int read(char *, int); 
};

MyReadWriter my_read_writer;
WriteAPI<ReadAPI<readwritable::view>> my_view;
my_view.bind(my_read_writer);
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
write_view.write("no view objects were instantiated");
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

StateFullWriteAPI<writable::view> stateful_ref;
stateful_ref.bind(rw_instance);

int num_writes = stateful_ref.write_api("stateful writing");
```

# What Happens On Error?

If a type bound to an archetype doesn’t implement all required methods, the code
will fail at compile time with a clear SFINAE-based error. No runtime surprises.

# Install

Just drop `archetype.h` into your project.

# Philosophy

Archetype doesn’t force you to change how you build your types. Instead it lets
you view them through modular, lightweight interfaces.

Think of it as:

- Static, type safe std::function
- Header only type erased concept
- A better way to do plugin interfaces
- Virtual interfaces without virtual inheritance

# Who is this for?

If you're building portable libraries, embedded systems, plugin based
frameworks, or trying to untangle brittle inheritance hierarchies Archetype is
for you.

It brings modern modularity to C++11, without the baggage.
