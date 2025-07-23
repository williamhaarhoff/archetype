
# How Archetype Works:

This document is meant to clearly show the principles on which archetype works without you having to decipher the macro heavy archetype.h file. The intention is to show how the principles have been combined, and provide a rationale for why I have implemented archetype this way. 

Underneath the hood Archetype uses manual vtables to acheive type erasure and run time polymorphism. This is not dissimilar from how virtual functions in traditional inheritance work. Vtables and surrounding infrastructure require a lot of boiler plate. The point of Archetype is to automate manual vtable generation, in a modular/composable way.


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

Making it composable:

```cpp
struct VTableBase {

  template<typename T>
  static VTableBase * make_vtable() {
    static VTableBase vtablet;
    return &vtablet;
  }

  template <typename T>
  void bind() {}
};

template<typename VTableType>
struct ViewBase
{
  void * _obj;
  VTableType * _vtbl;
};


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