#include <cstddef>
#include <iostream>

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

template<typename T>
struct identity : public T {};

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

  template <template <typename> class API = identity>
  struct view_ptr
  {
    template<typename T>
    view_ptr(T & t) : _view(t) {}

    API<view> &operator*() { return &_view; }
    const API<view> &operator*() const { return &_view; }
    API<view> *operator->() { return &_view; }
    const API<view> *operator->() const { return &_view; }

    protected:
    API<view> _view;
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


  template<typename Baseview_layer = view_base<vtable<>>>
  struct view_layer : public helper<writable>::view_layer<helper<readable>::view_layer<Baseview_layer>>
  {
    protected:
    using Baseview_layer::_obj;
    using Baseview_layer::_vtbl;
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



struct AbstractWriter
{
  virtual int write(const char *buf, size_t size) = 0;
};

struct AbstractReader
{
  virtual int read(char *buf, size_t size) = 0;
};

struct DerivedWriter : public AbstractWriter
{
  virtual int write(const char *buf, size_t size) final {
    int n = 0;
    while (size--) {
      std::cout << buf[n++];
    }
    std::cout << std::flush;
    return n;
  }
};

struct DerivedReadWriter : public AbstractWriter, public AbstractReader
{
  virtual int write(const char *buf, size_t size) final {
    int n = 0;
    while (size--) {
      std::cout << buf[n++];
    }
    std::cout << std::flush;
    return n;
  }
  virtual int read(char *buf, size_t size) final {
    return std::fread(buf, 1, size, stdin);
  }
};


int main()
{
  DerivedReadWriter wr;
  writable::view wv(wr);
  readable::view rv(wr);
  readwritable::view wrv(wr);

  //helper<writable>::vtable<helper<readable>::vtable<>> built;


  wrv.write("hello\n", 7);

  char buf[5];
  wrv.read(buf, sizeof(buf));
  printf("done\n");



  // std::cout << "wrv._obj: " << &wrv._obj << std::endl; 

  
  return 0;
}