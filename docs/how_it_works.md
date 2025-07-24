
# How Archetype Works:

This document is meant to clearly show the principles on which archetype works without you having to decipher the macro heavy archetype.h file. The intention is to show how the principles have been combined, and provide a rationale for why I have implemented archetype this way. 

Underneath the hood `Archetype` uses manual vtables to acheive type erasure and run time polymorphism. This is not dissimilar from how virtual functions in traditional inheritance work. Vtables and surrounding infrastructure require a lot of boiler plate. The point of `Archetype` is to automate manual `vtable` generation, in a modular/composable way.

### The basic vtable 
A `vtable` is a structure containing free function pointers (non member function pointers). For example, this is a `vtable` structure containing function pointers for `func_a` and `func_b`.

```cpp
struct vtable
{
  int (*func_a)(int);
  float (*func_b)(float);
};
```
We can now assign any function that matches this signature to the vtable. For example:

```cpp
int func_a(int a) { return a + 5; }
float func_b(float b) { return b + 5.3; }

vtable mytable;
mytable.func_a = &func_a;
mytable.func_b = &func_b;
```
we can now call these functions through the vtable:
```cpp
mytable.func_a(5);    //returns 10;
mytable.func_b(5.f);  //returns 10.3;
```
### The object facing vtable
In Archetype the goal is binding objects/classes, not free functions. Member function pointers (non static) are not free functions. They have to point to both the correct function, and the object instance, which mean they don't have the size of a free function pointer.

Lets say we want a vtable that can call objects of the following type:
```cpp
struct A
{
  int func_a(int a) { return a + internal_int++; }
  float func_b(float b) { return b + (internal_float += 1.3f); }
  int internal_int = 5;
  int internal_float = 5.3;
};
```

We can get around this by storing free functions that can be called with an object pointer. For example our vtable becomes:

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
vtbl.func_a = [](A * ptr, int a) { return ptr->func_a(a); };
vtbl.func_b = [](A * ptr, float b) { return ptr->func_b(b); };

vtbl.func_a(obj, 5);    // call func_a on obj
vtbl.func_b(obj, 6.4);  // call func_b on obj
```

This implementation is not very generic. As our vtable depends on type A. We can make the vtable type agnostic by passing in the object as a void pointer instead. We can push the type specific handling into the lambda. 

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
So far the vtable itself has given us a type erased way to call functions on arbitrary types, provided we can assign lambdas to out vtable function pointers. However, its still very awkward to setup and call. The `view` is an object that carries around the object `void *` and can pass this into the vtable functions.

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

This is a little cleaner. We have one vtable instance per bound type. And we have one view instance per object that we bind to. By keeping the vtable and view separate as separate objects we keep memory usage lower, and have improved cache locality for the function pointers. 

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
To handle function overloading we need to make sure that the `vtable` uses unique names for its function pointers. We can use normal function overloading in the view to resolve the correct function call.

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
    void * obj;                                  // from ?
    vtable * vtbl;                               // from ?
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

In Archetype did this by orthogonalising the structures, and then composing them through inheritance of orthogonal parts. The orthogonal parts come from each of the existing archetypes, while the common parts can come from a common base. `readable` annd `writable` views define the `read()` and `write()` functions respectively. But they will need to share a common vtable, and void object pointer. We will see how to define this vtable later. Both the object pointer and the vtable pointer get placed in the common base. 

```cpp
template<typename VTableType>
struct view_base
{
  void * _obj;
  VTableType * _vtbl;
};
```

The readable and writable views are orthogonalised into layers which can bve composed in an inheritance chain. For now you can ignore the default assignment of `BaseViewLayer = view_base<vtable<>>`, `vtable<>` will be discussed in a later section. 
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


#### The composable vtable
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

The problem we have now, is that while we can create function pointers for each vtable, we still need a way to assign them.Becase we are automating this, we need an orthogonal way to do this. Because each derived class is aware of its base type, it can call functions from the base. My approach here is that every layer has the same function, which hides the ones from the layer below. However, each layer still knows how to call the function in the layer directly below itself. Lets do this by adding a `bind()` function to every layer. 

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










```cpp
struct writable
{
  template<typename BaseVTable = VTableBase>
  struct vtable : public BaseVTable
  {
    int (*write)(void * obj, const char *, int);
    
    template<typename T>
    void bind()
    {
      this->BaseVTable::template bind<T>();

      write = [](void * obj, const char * arg0, int arg1) -> int {
        return static_cast<T*>(obj)->write(arg0, arg1); 
      };
    }

    public:
    template<typename T>
    static vtable * make_vtable()
    {
      static vtable<BaseVTable> vtablet;
      vtablet.bind<T>();

      return &vtablet;
    }
  };


  template<typename BaseViewLayer = ViewBase<vtable<>>>
  struct ViewLayer : public BaseViewLayer
  {
    using BaseViewLayer::_obj;
    using BaseViewLayer::_vtbl;

    int write(const char * arg0, int arg1) { return _vtbl->write(_obj, arg0, arg1); }
  };

  struct view : public ViewLayer<> {};

  template<typename T>
  static view make_view(T & t)
  {
    view v;
    v._obj = static_cast<void *>(&t);
    v._vtbl = vtable<>::make_vtable<T>();
    return v;
  }
};

struct readable
{
  template<typename BaseVTable = VTableBase>
  struct vtable : public BaseVTable
  {
    int (*read)(void * obj, char *, int);
    
    template<typename T>
    void bind()
    {
      this->BaseVTable::template bind<T>();

      read = [](void * obj, char * arg0, int arg1) -> int {
        return static_cast<T*>(obj)->read(arg0, arg1); 
      };
    }

    public:
    template<typename T>
    static vtable * make_vtable()
    {
      static vtable<BaseVTable> vtablet;
      vtablet.bind<T>();

      return &vtablet;
    }
  };


  template<typename BaseViewLayer = ViewBase<vtable<>>>
  struct ViewLayer : public BaseViewLayer
  {
    using BaseViewLayer::_obj;
    using BaseViewLayer::_vtbl;

    int read(char * arg0, int arg1) { return _vtbl->read(_obj, arg0, arg1); }
  };

  struct view : public ViewLayer<> {};

  template<typename T>
  static view make_view(T & t)
  {
    view v;
    v._obj = static_cast<void *>(&t);
    v._vtbl = vtable<>::make_vtable<T>();
    return v;
  }
};


struct readwritable
{
  template<typename BaseVTable = VTableBase>
  struct vtable : public writable::vtable<readable::vtable<BaseVTable>>
  {
    using this_base = writable::vtable<readable::vtable<BaseVTable>>;
    
    template<typename T>
    void bind()
    {
      this->this_base::template bind<T>();
    }

    public:
    template<typename T>
    static vtable * make_vtable()
    {
      static vtable<BaseVTable> vtablet;
      vtablet.bind<T>();

      return &vtablet;
    }
  };


  template<typename BaseViewLayer = ViewBase<vtable<>>>
  struct ViewLayer : public writable::ViewLayer<readable::ViewLayer<BaseViewLayer>>
  {
    using BaseViewLayer::_obj;
    using BaseViewLayer::_vtbl;
  };

  struct view : public ViewLayer<> {};

  template<typename T>
  static view make_view(T & t)
  {
    view v;
    v._obj = static_cast<void *>(&t);
    v._vtbl = vtable<>::make_vtable<T>();
    return v;
  }
};
```

Adding access control, to only expose part of the API

```cpp
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

template <class C> struct 
helper 
{
  template <typename T = vtable_base>
  using vtable = typename C::template vtable<T>;
  
  template<typename T = view_base<vtable<>>>
  using view_layer = typename C::template view_layer<T>;
};

struct writable
{
  public:
  writable() = delete;
  ~writable() = delete;
  writable & operator=(const writable &) = delete;
  
  friend class helper<writable>;

  protected:
  template<typename BaseVTable = vtable_base>
  struct vtable : public BaseVTable {
    int (*write)(void * obj, const char *, int);
    
    template<typename T>
    void bind()
    {
      this->BaseVTable::template bind<T>();

      write = [](void * obj, const char * arg0, int arg1) -> int {
        return static_cast<T*>(obj)->write(arg0, arg1); 
      };
    }

    public:
    template<typename T>
    static vtable * make_vtable()
    {
      static vtable<BaseVTable> vtablet;
      vtablet.bind<T>();

      return &vtablet;
    }
  };


  template<typename Baseview_layer = view_base<vtable<>>>
  struct view_layer : public Baseview_layer
  {
    protected:
    using Baseview_layer::_obj;
    using Baseview_layer::_vtbl;

    public:
    int write(const char * arg0, int arg1) { return _vtbl->write(_obj, arg0, arg1); }
  };

  public:
  struct view : public view_layer<> 
  {
    template<typename T>
    view(T & t)
    {
      this->_obj = static_cast<void *>(&t);
      this->_vtbl = vtable<>::make_vtable<T>();
    }
  };
};


struct readable
{
  public:
  readable() = delete;
  ~readable() = delete;
  readable & operator=(const readable &) = delete;
  
  friend class helper<readable>;

  protected:
  template<typename BaseVTable = vtable_base>
  struct vtable : public BaseVTable {
    int (*read)(void * obj, char *, int);
    
    template<typename T>
    void bind()
    {
      this->BaseVTable::template bind<T>();

      read = [](void * obj, char * arg0, int arg1) -> int {
        return static_cast<T*>(obj)->read(arg0, arg1); 
      };
    }

    public:
    template<typename T>
    static vtable * make_vtable()
    {
      static vtable<BaseVTable> vtablet;
      vtablet.bind<T>();

      return &vtablet;
    }
  };


  template<typename Baseview_layer = view_base<vtable<>>>
  struct view_layer : public Baseview_layer
  {
    protected:
    using Baseview_layer::_obj;
    using Baseview_layer::_vtbl;

    public:
    int read(char * arg0, int arg1) { return _vtbl->read(_obj, arg0, arg1); }
  };

  public:
  struct view : public view_layer<> 
  {
    template<typename T>
    view(T & t)
    {
      this->_obj = static_cast<void *>(&t);
      this->_vtbl = vtable<>::make_vtable<T>();
    }
  };
};


struct readwritable
{
  public:
  readwritable() = delete;
  ~readwritable() = delete;
  readwritable & operator=(const readwritable &) = delete;
  
  friend class helper<readwritable>;

  protected:
  template<typename BaseVTable = vtable_base>
  struct vtable : public helper<writable>::vtable<helper<readable>::vtable<BaseVTable>>
  {
    using this_base = helper<writable>::vtable<helper<readable>::vtable<BaseVTable>>;
    template<typename T>
    void bind()
    {
      this->this_base::template bind<T>();
    }

    public:
    template<typename T>
    static vtable * make_vtable()
    {
      static vtable<BaseVTable> vtablet;
      vtablet.bind<T>();

      return &vtablet;
    }
  };

```






## Component structure:
Almost all of the core functionality exists within the protected component that exists within the `basic_interface` struct. I'll start with a stripped down version, and explain how the functionality is build up. 

The component itself is templated on a base class. It will inherit from a particular base, and stack its functionality on top without using any virtuals. All the component cares about is being able to access the `B::_obj` that is a `void *` protected member variable defined by the base class. This `B::_obj` is going to store a `void *` of the object that will get bound. 

```cpp
template <typename B = archetype::Base> 
class component : public B {
  protected:
  using B::_obj;
};
```

For the component to actully be useful, it needs to implement the interface that we've defined. It does this by implementing a `vtable` through function pointers and functions. In this case we have a protected function pointer: `int (*_func0_stub)(void *obj, const char *);` 

```cpp
template <typename B = archetype::Base> 
class component : public B {
  public:
  int func0(const char *arg0) { return _func0_stub(_obj, arg0); }

  protected:
  int (*_func0_stub)(void *obj, const char *);
  using B::_obj;
};
```