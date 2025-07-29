
# How Archetype Works:

Because Archetype is a macro library, it can be difficult to understand how the macros expand, the intent, and the overall architecture. This document reflects the thought process, concerns, principles, used in creating archetype. Without delving into any macro expansion / macro techniques. This document is a mix between tutorial and explanation around the development of Archetype.
 
Underneath the hood `Archetype` uses manual vtables to acheive type erasure and run time polymorphism. This is not dissimilar from how virtual functions in traditional inheritance work. Vtables and surrounding infrastructure require a lot of boiler plate. The point of `Archetype` is to automate manual `vtable` generation, in a modular/composable way, while providing a simple user facing interface. 

### The basic vtable 
A `vtable` is a structure containing free function pointers (non member function pointers). For example, this is a `vtable` structure containing function pointers for `func_a` and `func_b` as member variables.

```cpp
struct vtable
{
  int (*func_a)(int);
  float (*func_b)(float);
};
```
We can now assign any function that matches this signature to the vtable. For example:

```cpp
int do_a(int a) { return a + 5; }
float do_b(float b) { return b + 5.3; }

vtable mytable;
mytable.func_a = &do_a;
mytable.func_b = &do_b;
```
we can now call these functions through the vtable:
```cpp
mytable.func_a(5);    //returns 10;
mytable.func_b(5.f);  //returns 10.3;
```

### The object facing vtable
In Archetype the goal is binding objects/classes, which means calling member functions, not regular functions. The problem with member function pointers is that they are type dependent, and to be able to store and call them from our vtable, our vtable would have to depend on the class we are binding to.

Lets say we want a vtable that can call objects of the following type:
```cpp
struct A
{
  int do_a(int a) { return a + internal_int++; }
  float do_b(float b) { return b + (internal_float += 1.3f); }
  int internal_int = 5;
  int internal_float = 5.3;
};
```

We can get around requiring member function pointers by storing free functions that can be called with an object pointer. For example our vtable becomes:

```cpp
struct vtable
{
  int (*func_a)(A *, int);
  float (*func_b)(A *, float);
};
```
We can then assign this vtable to call an object of type `A's` functions. 
```cpp
A obj;
vtable vtbl;
vtbl.func_a = [](A * ptr, int a) { return ptr->do_a(a); };
vtbl.func_b = [](A * ptr, float b) { return ptr->do_b(b); };

vtbl.func_a(obj, 5);    // call func_a on obj
vtbl.func_b(obj, 6.4);  // call func_b on obj
```

This implementation is not very generic. As our vtable still depends on type A. We can make the vtable type agnostic by passing in the object as a void pointer instead. We can push the type specific handling into the lambda. 

```cpp
struct vtable
{
  int (*func_a)(void *, int);
  float (*func_b)(void *, float);
};

vtbl.func_a = [](void * ptr, int a) { 
  return static_cast<A*>(ptr)->func_a(a); 
};

vtbl.func_a(static_cast<void*>(obj), 5);
```

### The view
So far the vtable itself has given us a type erased way to call functions on arbitrary types, provided we can assign lambdas to out vtable function pointers. However, its still very awkward, and manul to setup and call. The `view` is an object that carries around the object `void *` and can pass this into the vtable functions.
```cpp
struct view
{
  int func_a(int a) { return vtbl_->func_a(obj_, a); }
  float func_b(float b) { return vtbl_->func_b(obj_, b); }
  void * obj_;
  vtable * vtbl_;
};
```
Provided our vtable pointer is pointing to a vtable that is correct for the current type, then we can assign the object and call like so:

```cpp
A obj;
view myview;
myview.obj_ = static_cast<void*>(&obj);
myview.vtbl_ = vtbl; // vtable for A

myview.func_a(5);
myview.func_b(3.2);
```

This is a little cleaner. We have one vtable instance per bound type. And we have one view instance per object that we bind to. By keeping the vtable and view separate we keep memory usage lower that combined vtable/view implementations. 

To ensure we are only creating on vtable per type, we can make use of a static vtable variable within a templated function. Every time we call `make_vtable<A>()` we are using a pointer to the `A` `vtable`.

```cpp
template <typename T>
static vtable * make_vtable()
{
  static vtable vtablet;
  vtablet.func_a = [](void * obj, int arg0) -> int {
    return static_cast<T*>(obj)->write(arg0);
  };
  ...
  return &vtablet;
}
```
To handle function overloading we need to make sure that the `vtable` uses unique names for its function pointers. We can use normal function overloading in the view to resolve the correct function call. In Archetype I've done this by using the `__LINE__` and `__COUNTER__` macros to generate unique names.

### The Archetype
Taking inspiration from the way that C++20 concepts can be defined and composed together, I wanted to do something similar. The idea being that you could define interface specifications, and then compose these together. Below is the rough idea for a structure that defines the `writable` "concept". I ended up calling these containing structures Archetypes.  

```cpp
struct writable
{
  struct vtable
  {
    int (*write)(void * obj, const char *, int);
  };

  template <typename T>
  static vtable * make_vtable()
  {
    static vtable vtablet;
    vtablet.write = [](void * obj, const char * arg0, int arg1) -> int {
      return static_cast<T*>(obj)->write(arg0, arg1);
    };
    return &vtablet;
  }

  struct view
  {
    void * obj;
    vtable * vtbl;
    int write(const char * arg0, int arg1) { return vtbl->write(obj, arg0, arg1); }
  };

  template<typename T>
  static view make_view(T & t)
  {
    view v;
    v.obj = static_cast<void *>(&t);
    v.vtbl = make_vtable<T>();
    return v;
  }
};
```

To create views to types `A` and `B` we can do the following:

```cpp
A a; B b; B b2; 
writable::view view_a = writable::make_view(a);
writable::view view_b = writable::make_view(b);
writable::view view_b2 = writable::make_view(b2);
```
Both `view_b` and `view_b2` are using the same vtable to type `B`.


### The composable view

The view is a little easier to make composable than the vtable so I'll start here. Lets say we also have a `readable` `Archetype`, and we would like to compose this together with `writable` to create `readwritable`. The view should end up being:

```cpp
struct readwritable
{
  struct view
  {
    void * obj;                                  // common to both
    vtable * vtbl;                               // common to both
    int write(const char * arg0, int arg1) { 
      return vtbl->write(obj, arg0, arg1);       // from writable
    }
    int read(char * arg0, int arg1) { 
      return vtbl->read(obj, arg0, arg1);        // from readable
    }
  };
  ...
};
```

In Archetype I did this by orthogonalising the structures, and then composing them through inheritance of orthogonal parts. The orthogonal parts come from each of the existing archetypes, while the common parts can come from a common base. `readable` and `writable` views define the `read()` and `write()` functions respectively. But they will need to share a common vtable, and void object pointer. We will see how to define this vtable later. Both the object pointer and the vtable pointer get placed in the common base. 

```cpp
template<typename VTableType>
struct view_base
{
  void * _obj;
  VTableType * _vtbl;
};
```

The readable and writable views are orthogonalised into layers which can be composed in an inheritance chain. For now you can ignore the default assignment of `BaseViewLayer = view_base<vtable<>>`, `vtable<>` will be discussed in the composable vtable section. 
```cpp
struct writable
{
  template<typename BaseViewLayer = view_base<vtable<>>>
  struct view_layer : public BaseViewLayer
  {
    using BaseViewLayer::_obj;
    using BaseViewLayer::_vtbl;

    int write(const char * arg0, int arg1) { 
      return _vtbl->write(_obj, arg0, arg1); 
    }
  };
  ...
};

struct readable
{
  template<typename BaseViewLayer = view_base<vtable<>>>
  struct view_layer : public BaseViewLayer
  {
    using BaseViewLayer::_obj;
    using BaseViewLayer::_vtbl;

    int read(char * arg0, int arg1) { 
      return _vtbl->read(_obj, arg0, arg1); 
    }
  };
  ...
};

struct readwritable
{
  template<typename BaseViewLayer = view_base<vtable<>>>
  struct view_layer : public writable::view_layer<readable::view_layer<BaseViewLayer>>
  {
    using BaseViewLayer::_obj;
    using BaseViewLayer::_vtbl;
  };
  ...
};
```

The default parameterisation of the view layers with `view_base<vtable<>>` will set the `BaseViewLayer::_vtbl` to use the correct `vtable` type when no parameters are passed in. For example, here is how we can instantiate concrete views for each Archetype.  

```cpp
writable::view_layer<> write_view;
readable::view_layer<> readable_view;
readwritable::view_layer<> readable_view;
```

While we have ended up with something more complex and verbose we are making progress in the right direction. The structures generated are more modular, and homogenous, which later becomes important for automating their generation and composition.


### The composable vtable
The composable vtable follows the same principals as the composable view. All the common parts go in the base, all the orthogonal parts go in the layers.
A first approach would look something like this:

```cpp
struct vtable_base { }; // common base is empty as no common parts

struct writable
{
  template<typename BaseVTable = vtable_base>
  struct vtable : public BaseVTable
  {
    int (*write)(void * obj, const char *, int);
  };
  ...
};

struct readable
{
  template<typename BaseVTable = vtable_base>
  struct vtable : public BaseVTable
  {
    int (*read)(void * obj, char *, int);
  };
  ...
};

struct readwritable
{
  template<typename BaseVTable = vtable_base>
  struct vtable : public writable::vtable<readable::vtable<BaseVTable>> { };
  ...
};
```
We can still create our normal readable and writable vtables as:

```cpp
writable::vtable<> wvtl; 
readable::vtable<> rvtl;
readwritable::vtable<> rvtl; 
```

The problem we have now, is that while we can create function pointers for each vtable, we still need a way to assign them. Becase we are automating this, we need an orthogonal way to do this. Because each derived class is aware of its base type, it can call functions from the base. My approach here is that every layer has the same function, which hides the ones from the layer below. However, each layer still knows how to call the function in the layer directly below itself. Which means we can create a function call chain starting at the derived, and chaining down to the base. Lets do this by adding a `bind()` function to every layer. 

```cpp
struct vtable_base 
{ 
  template<typename T>
  void bind() { } // dummy function will be called by layer above
};

struct writable
{
  template<typename BaseVTable = vtable_base>
  struct vtable : public BaseVTable
  {
    using this_base = BaseVTable;
    template<typename T>
    void bind() { 
      this->this_base::template bind<T>(T & t); // calls layer below
      write = [](void * obj, const char * arg0, int arg1) -> int {
        return static_cast<T*>(obj)->write(arg0, arg1); 
      };
    }
  };
  
  int (*write)(void * obj, const char *, int);
  ...
};

struct readable
{
  template<typename BaseVTable = vtable_base>
  struct vtable : public BaseVTable
  {
    using this_base = BaseVTable;
    template<typename T>
    void bind()
    {
      this->BaseVTable::template bind<T>(); // calls layer below

      read = [](void * obj, char * arg0, int arg1) -> int {
        return static_cast<T*>(obj)->read(arg0, arg1); 
      };
    }
    int (*read)(void * obj, char *, int);
  };
  ...
};

struct readwritable
{
  template<typename BaseVTable = vtable_base>
  struct vtable : public writable::vtable<readable::vtable<BaseVTable>> 
  { 
    using this_base = writable::vtable<readable::vtable<BaseVTable>>;
    template<typename T>
    void bind()
    {
      this->this_base::template bind<T>();
    }
  };
  ...
};
```



### The restricted API

So far this implementation has been unrestricted, meaning its easy for users to accidentally reach into the implementations and assign/modify variables. To keep the library intuitive and safe I wanted to make a public API, and restrict access to non API structs or functions. 

- Archetypes shouldn't be instantiable. This means deleting constructors.
- The vtable shouldn't be called or used directly outside the library. This means making it a protected type within the archetype, but allowing friends within the library to have access. 

One of the interesting patterns was allowing the library to perform the inheritance chaining of view layers and vtables. I did this by allowing the archetypes to friend a templated helper. The helper can then publically expose the view_layer, and vtable types for use in inheritance chaining. 

```cpp
template <class C> struct 
helper 
{
  template <typename T = vtable_base>
  using vtable = typename C::template vtable<T>;
  
  template<typename T = view_base<vtable<>>>
  using view_layer = typename C::template view_layer<T>;
};
```

An inheritance chain example:

```cpp
using this_base = helper<writable>::vtable<helper<readable>::vtable<BaseVTable>>;
```

### The full expansion
For reference, I'm including the full expansion of the `writable`, `readable`, `readwritable` example as by the library with:

```
ARCHETYPE_DEFINE(writable, (ARCHETYPE_METHOD(int, write, const char *, int)))
ARCHETYPE_DEFINE(readable, (ARCHETYPE_METHOD(int, read, char *, int)))
ARCHETYPE_COMPOSE(readwritable, writable, readable)
```

```cpp
namespace archetype {

struct vtable_base {
  template <typename T> static vtable_base *make_vtable() {
    static vtable_base vtablet;
    return &vtablet;
  }

  template <typename T> void bind() {}
};

template <typename VTableType> class view_base {
protected:
  void *_obj;
  VTableType *_vtbl;
};

template <typename...> using void_t = void;

template <typename Base> struct identity : public Base {
  using Base::Base;
};

template <class Archetype> struct helper {
  template <typename T = vtable_base>
  using vtable = typename Archetype::template vtable<T>;

  template <typename T = view_base<vtable<>>>
  using view_layer = typename Archetype::template view_layer<T>;
};
} // namespace archetype


struct writable {
  writable() = delete;
  ~writable() = delete;
  writable &operator=(const writable &) = delete;
  friend struct archetype::helper<writable>;
  template <typename, typename = void> struct check : std::false_type {};
  template <typename T>
  struct check<T, archetype::void_t<decltype(
    static_cast<int (T::*)(const char *arg0, int arg1)>(&T::write))>> : std::true_type {};

protected:
  template <typename BaseVTable = archetype::vtable_base>
  struct vtable : public BaseVTable {
    int (*_write_483_0_stub)(void *obj, const char *, int);
    template <typename T> void bind() {
      static_assert(writable::check<T>::value,
                    "T must satisfy writable::check");
      this->BaseVTable::template bind<T>();
      _write_483_0_stub = [](void *obj, const char *arg0, int arg1) -> int {
        return static_cast<T *>(obj)->write(arg0, arg1);
      };
    }
    template <typename T> static vtable *make_vtable() {
      static vtable<BaseVTable> vtablet;
      vtablet.bind<T>();
      return &vtablet;
    }
  };
  template <typename BaseViewLayer = archetype::view_base<vtable<>>>
  struct view_layer : public BaseViewLayer {
  public:
    int write(const char *arg0, int arg1) {
      return _vtbl->_write_483_0_stub(_obj, arg0, arg1);
    }

  protected:
    using BaseViewLayer::_obj;
    using BaseViewLayer::_vtbl;
  };

public:
  struct view : public view_layer<> {
    template <typename T> view(T &t) {
      this->_obj = static_cast<void *>(&t);
      this->_vtbl = vtable<>::make_vtable<T>();
    }
  };
  template <template <typename> class API = archetype::identity> struct ptr {
    template <typename T> ptr(T &t) : _view(t) {}
    API<view> &operator*() { return &_view; }
    const API<view> &operator*() const { return &_view; }
    API<view> *operator->() { return &_view; }
    const API<view> *operator->() const { return &_view; }

  protected:
    API<view> _view;
  };
};
struct readable {
  readable() = delete;
  ~readable() = delete;
  readable &operator=(const readable &) = delete;
  friend struct archetype::helper<readable>;
  template <typename, typename = void> struct check : std::false_type {};
  template <typename T>
  struct check<
      T, archetype::void_t<
             decltype(static_cast<int (T::*)(char *arg0, int arg1)>(&T::read))>>
      : std::true_type {};

protected:
  template <typename BaseVTable = archetype::vtable_base>
  struct vtable : public BaseVTable {
    int (*_read_484_1_stub)(void *obj, char *, int);
    template <typename T> void bind() {
      static_assert(readable::check<T>::value,
                    "T must satisfy readable::check");
      this->BaseVTable::template bind<T>();
      _read_484_1_stub = [](void *obj, char *arg0, int arg1) -> int {
        return static_cast<T *>(obj)->read(arg0, arg1);
      };
    }
    template <typename T> static vtable *make_vtable() {
      static vtable<BaseVTable> vtablet;
      vtablet.bind<T>();
      return &vtablet;
    }
  };
  template <typename BaseViewLayer = archetype::view_base<vtable<>>>
  struct view_layer : public BaseViewLayer {
  public:
    int read(char *arg0, int arg1) {
      return _vtbl->_read_484_1_stub(_obj, arg0, arg1);
    }

  protected:
    using BaseViewLayer::_obj;
    using BaseViewLayer::_vtbl;
  };

public:
  struct view : public view_layer<> {
    template <typename T> view(T &t) {
      this->_obj = static_cast<void *>(&t);
      this->_vtbl = vtable<>::make_vtable<T>();
    }
  };
  template <template <typename> class API = archetype::identity> struct ptr {
    template <typename T> ptr(T &t) : _view(t) {}
    API<view> &operator*() { return &_view; }
    const API<view> &operator*() const { return &_view; }
    API<view> *operator->() { return &_view; }
    const API<view> *operator->() const { return &_view; }

  protected:
    API<view> _view;
  };
};
struct readwritable {
  readwritable() = delete;
  ~readwritable() = delete;
  readwritable &operator=(const readwritable &) = delete;
  friend struct archetype::helper<readwritable>;
  template <typename T>
  struct check : std::integral_constant<bool, writable::check<T>::value &&
                                                  readable::check<T>::value> {};

protected:
  template <typename BaseVTable = archetype::vtable_base>
  struct vtable : public archetype::helper<writable>::vtable<
                      archetype::helper<readable>::vtable<BaseVTable>> {
    using this_base = archetype::helper<writable>::vtable<
        archetype::helper<readable>::vtable<BaseVTable>>;
    template <typename T> void bind() { this->this_base::template bind<T>(); }
    template <typename T> static vtable *make_vtable() {
      static vtable<BaseVTable> vtablet;
      vtablet.bind<T>();
      return &vtablet;
    }
  };
  template <typename BaseViewLayer = archetype::view_base<vtable<>>>
  struct view_layer
      : public archetype::helper<writable>::view_layer<
            archetype::helper<readable>::view_layer<BaseViewLayer>> {
  protected:
    using BaseViewLayer::_obj;
    using BaseViewLayer::_vtbl;
  };

public:
  struct view : public view_layer<> {
    template <typename T> view(T &t) {
      this->_obj = static_cast<void *>(&t);
      this->_vtbl = vtable<>::make_vtable<T>();
    }
  };
  template <template <typename> class API = archetype::identity> struct ptr {
    template <typename T> ptr(T &t) : _view(t) {}
    API<view> &operator*() { return &_view; }
    const API<view> &operator*() const { return &_view; }
    API<view> *operator->() { return &_view; }
    const API<view> *operator->() const { return &_view; }

  protected:
    API<view> _view;
  };
};
```

The patterns.cpp file also contains a simplified, handwritten version of this example. 