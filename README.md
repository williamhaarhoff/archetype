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
I stumbled across this pattern while refactoring inheritance based interfaces. I believe its powerful tool, keeping the benefits of inheritance, and bypassing its issues. Ultimately leading to much more modular, and flexible inheritance like interfaces. 

# Problems with Inheritance
### What inheritance promises
Inheritance is apealling for creating interfaces because it promises us:
 - A common way of inerfacing with many different derived types, using base pointers/references.
 - Clean type erased interfaces. As long as your class derives from the base, you can substitute it into the interface. No templates, adaptors, are required to specialise the api to your derived type. 

### Where inheritance fails
Unfortunately good intentions often turn to poor compromises, and rigid inheritance hierachies. I find the following example to highlight where inheritance fails us:

- `class AB` should have methods `void a()`, and `int b(int)`
- `class AC` should have methods `void a()`, and `double c(double)`
  
We can do something like:
```cpp
class A { public: void a(); };
class AB : public A { public: int b(int); };        // inherits from A
class AC : public A { public: double c(double); };  // inherits from A
```
We now need to introduce `class BC` which should use the same `int b(int)`
method and `double c(double)` methods as class `AB` and class `AC` use. Solutions are:
1. **Compromise**: Duplicate methods `int b(int)` and `double c(double)` into a new class with no relation to either `AB` or `AC`.
   ```cpp
   class BC { public: int b(int); double c(double); }; // gross
   ```

2. **Use Composition**: Define objects `A` `B` and `C` with their respective methods. Compose classes `AB`, `AC`, and `BC` with their respective members:
   ```cpp
   class A {public: void a(); };
   class B {public: int b(int); };
   class C {public: double c(double); };
   class AB {public: A a; B b; };           // composed from A and B
   class AC {public: A a; C c; };           // composed from A and C
   class BC {public: B b; C c; };           // composed from B and C
   ```
   We regain flexibility, and decrese coupling. This is still a compromise, as we loose all common bases and way to refer to objects that contain A's or B's or C's. 

3. **Use Multiple Inheritance**: Digging the hole deeper is always an option. We can use multiple inheritance to "compose" multiple orthogonal classes into a single class much like using a mixin pattern. 

    ```cpp
    class A {public: void a(); };
    class B {public: int b(int); };
    class C {public: double c(double); };
    class AB : public A, public B {};       // inherits from A and B
    class AC : public A, public C {};       // inherits from A and C
    class BC : public B, public C {};       // inherits from B and C
    ```
    We can now use base pointers to refer to AB and AC through their common interfaces. For example:

    ```cpp
    AB ab;
    AC ac;
    A & ab_ref = static_cast<A&>(ab);
    A & ac_ref = static_cast<A&>(ac);
    ```
    While this works for these simple types, multiple inheritance has not solved the problem, but rather **deferred** it, and has introduced some new problems too. If we add `class D` and want common base of `AB` between `ABC` and `ABD` we have to start inheriting composites ie: `AB`

    ```cpp
    class ABC : public AB, public C {};    // inherits common base AB
    class ABD : public AB, public D {};    // inherits common base AB
    ```
    We now run into exactly the same problem as before when we want an common interface for `class ABC` and `class ACD`. 


# Archetypes
The archetype pattern makes it possible to get references to composite classes.The archetype pattern creates views to Lets consider the previous example that multiple inheritance failed to solve. We would like a common interface between `class ABC` and `class ABD` and a common interface between `class ABC` and `class ACD`. I'm assuming that these classes have already been defined and implement their respective methods. 

With inheritance, the derived class has an **is-a** relationship to the base. In composition the composite class has a **has-a** relationship to its constituents. With the archetype pattern, the class being bound must have a **implements** relationship with respect to the archetype requirements.

We define archetypes in a manner similar to how we define C++20 concepts. We define the function name and signature of the methods that are required. Below we define `archetype_a` whose base can bind to any type that **implements** a member function `void a()`. 

```cpp
#include "archetype.h"
DEFINE_ARCHETYPE(archetype_a, ( DEFINE_METHOD(void, a) ))
DEFINE_ARCHETYPE(archetype_b, ( DEFINE_METHOD(int, b, int) )) 
DEFINE_ARCHETYPE(archetype_c, ( DEFINE_METHOD(double, c, double) )) 
```
In our case we are only requiring one function per archetype, but more are possible, and function overloading is supported.
If we wish we can compose primitive archetypes into more complex types.

```cpp
COMPOSE_ARCHETYPE(archetype_ab, archetype_a, archetype_b)
COMPOSE_ARCHETYPE(archetype_ac, archetype_b, archetype_c)
```
We can now create base references that bind to any class that **implements** the requirements of the archetype. 

```cpp
// Common AB interfaces to ABC and ABD
ABC abc;
ABD abd;
has_ab::base<> abc_ref, abd_ref; // type erased base references
abc_ref.bind(abc);
abd_ref.bind(abd);
abc_ref.a();    // call a() of ABC type
abd_ref.b(5);   // call method b of ABD
```
 We can do exatly the same for anything with a comonality of `AC`

```cpp
// Common AC interfaces to ABC and ACD
ACD acd;
has_ac::base<> abc_ref2, acd_ref; // type erased base references
abc_ref2.bind(abc);
acd_ref.bind(acd);
abc_ref2.c(13.5);       // call c() of ABC type
acd_ref.a();            // call method a of ACD
```

So Archetype solves the common base interface problem. It allows you to create base references to anything that implements the archetype requirements. But it does this without any coupling to the types it can bind to.

Archetype also solves the clean type erased interface problem. The archetype base type does not depend on the type being bound. This becomes really important if refactoring existing libraries and helps to avoid breaking the API.

# How should we build our type hierarchies?
Archetpye doesn't presecribe any particular way of building type hierarchies. It will work with the types you have, and will allow you to create views to them.

There are however some patterns that I think work very well with archetype. 

### Split APIs and Dependencies
I'm a huge fan of highly portable libraries. The goal of the library may be to implement algorithms, or a particular generic API. But to be portable we need to keep the surface area of user facing dependencies as small as possible.

Being very concept like supports this idea quite nicely. The library should depend on "concepts" rather than specific implementations. Now the user does not have to depend on these specific implementatons eiher. Provided the user can supply or replace default implementations with their own then we have a very portable library. 

In this case archetypes are not used to implement the library API but are used as the "concepts" on which the library depends.

```cpp
DEFINE_ARCHETYPE(loggable, ( DEFINE_METHOD(void, log, const char *) ))

class DoTheThing 
{
    public:
    void do_the_thing() { if(valid) logger.log("doing the thing"); }
    template<typename T>
    void set_logger(T & t) { valid = logger.bind(t); }
    private:
    bool valid = false;
    loggable::base<> logger;
};
```

In this case we have created a library class `DoTheThing` which has no dependency on the type used for logging.

### Maximally Composable APIs
I prefer to take this a bit further and make my APIs maximally resuable and composable, while maintaining minimal dependecies on implementations. Here we use a mixin pattern to define a `WriteAPI` and a `ReadAPI`. The `WriteAPI` will derive from class `W` and provide a `write_api` function. It's only dependency is that class `W` implements a public `write()` function. 

```cpp
template <class W> class WriteAPI : public W {
public:
  using W::W;
  using W::write;
  void write_api(const char *buf) {
    this->write(buf, size);
  }
};
```
Below we extend class `MyReadWriter` by deriving from it and adding the write API functionality in typical mixin fashion. 

```cpp
class MyReadWriter; // A class that implements read and write functions
WriteAPI<MyWriter> extended;
extended.write_api("hello");
```

Similarly we can also define a `ReadAPI` which will can also extend the `MyReadWriter` class. 
```cpp
template <class R> class ReadAPI : public R {
public:
  using R::R;
  using R::read;
  int read_api(char *buf, int size) {
    return this->read(buf, size);
  }
};
```


A much more interesting case that doesn't involve instantiating an object is to only create a reference to `WriteAPI<MyReadWriter>` and cast an object of `MyWriter`, as shown below. 

```cpp
MyWriter my_writer;
using WriteAPI<MyWriter> = ExtendedWriter;
ExtendedWriter * ptr = static_cast<ExtendedWriter*>(&my_writer);
ptr.write_api("hello");
```
Here we created a new type, but never instantiated it. Nonetheless we were able to view the instance of `MyWriter` through the lens of our `WriteAPI`. This is only okay because deriving our `ExtendedWriter` didn't add any member variables, or virtual functions to the class. Mixin pointer based views like this cannot keep track of state. 





