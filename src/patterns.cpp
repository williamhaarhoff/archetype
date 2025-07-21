

#include <cstddef>
#include <iostream>


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
class ViewBase
{
  protected:
  void * _obj;
  VTableType * _vtbl;
};


template<typename T>
struct exposer : public T {};

struct writable
{
  // forward delcarations
  struct view;
  template<typename T> view make_view(T & );


  template<typename BaseVTable = VTableBase>
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


  template<typename BaseViewLayer = ViewBase<vtable<>>>
  struct ViewLayer : public BaseViewLayer
  {
    protected:
    using BaseViewLayer::_obj;
    using BaseViewLayer::_vtbl;

    public:
    int write(const char * arg0, int arg1) { return _vtbl->write(_obj, arg0, arg1); }
  };

  struct view : public ViewLayer<> {
    template<typename T> friend view make_view<T &t>;
  };


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
    protected:
    using BaseViewLayer::_obj;
    using BaseViewLayer::_vtbl;
    public:
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
    protected:
    using BaseViewLayer::_obj;
    using BaseViewLayer::_vtbl;
  };


  struct view : public ViewLayer<> { };

  template<typename T>
  static view make_view(T & t)
  {
    view v;
    v._obj = static_cast<void *>(&t);
    v._vtbl = vtable<>::make_vtable<T>();
    return v;
  }
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
  writable::view wv = writable::make_view(wr);
  readable::view rv = readable::make_view(wr);
  readwritable::view wrv = readwritable::make_view(wr);

  wrv.write("hello\n", 7);

  char buf[5];
  wrv.read(buf, sizeof(buf));
  printf("done\n");

  std::cout << "wrv._obj: " << &wrv._obj << std::endl; 

  
  return 0;
}