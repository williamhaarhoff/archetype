

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

template <class C> class helper {
public:
  template <typename T = VTableBase>
  using vtable = typename C::template vtable<T>;
};

struct writable
{
  public:
  writable() = delete;
  ~writable() = delete;
  writable & operator=(const writable &) = delete;
  
  friend class helper<writable>;

  protected:
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

  public:
  struct view : public ViewLayer<> 
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
  template<typename BaseVTable = VTableBase>
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


  template<typename BaseViewLayer = ViewBase<vtable<>>>
  struct ViewLayer : public BaseViewLayer
  {
    protected:
    using BaseViewLayer::_obj;
    using BaseViewLayer::_vtbl;

    public:
    int read(char * arg0, int arg1) { return _vtbl->read(_obj, arg0, arg1); }
  };

  public:
  struct view : public ViewLayer<> 
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
  template<typename BaseVTable = VTableBase>
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


  template<typename BaseViewLayer = ViewBase<vtable<>>>
  struct ViewLayer : public BaseViewLayer
  {
    protected:
    using BaseViewLayer::_obj;
    using BaseViewLayer::_vtbl;

    public:
    int read(char * arg0, int arg1) { return _vtbl->read(_obj, arg0, arg1); }
  };

  public:
  struct view : public ViewLayer<> 
  {
    template<typename T>
    view(T & t)
    {
      this->_obj = static_cast<void *>(&t);
      this->_vtbl = vtable<>::make_vtable<T>();
    }
  };
};

// struct readwritable
// {
//   template<typename BaseVTable = VTableBase>
//   struct vtable : public writable::vtable<readable::vtable<BaseVTable>>
//   {
//     using this_base = writable::vtable<readable::vtable<BaseVTable>>;
    
//     template<typename T>
//     void bind()
//     {
//       this->this_base::template bind<T>();
//     }

//     public:
//     template<typename T>
//     static vtable * make_vtable()
//     {
//       static vtable<BaseVTable> vtablet;
//       vtablet.bind<T>();

//       return &vtablet;
//     }
//   };

//   template<typename BaseViewLayer = ViewBase<vtable<>>>
//   struct ViewLayer : public writable::ViewLayer<readable::ViewLayer<BaseViewLayer>>
//   {
//     protected:
//     using BaseViewLayer::_obj;
//     using BaseViewLayer::_vtbl;
//   };

//   struct view : public ViewLayer<> 
//   {
//     template<typename T>
//     view(T & t)
//     {
//       this->_obj = static_cast<void *>(&t);
//       this->_vtbl = vtable<>::make_vtable<T>();
//     }
//   };
// };







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
  // readwritable::view wrv(wr);

  helper<writable>::vtable<helper<readable>::vtable<>> built;
  

  // wrv.write("hello\n", 7);

  char buf[5];
  // wrv.read(buf, sizeof(buf));
  printf("done\n");



  // std::cout << "wrv._obj: " << &wrv._obj << std::endl; 
  

  
  
  

  // std::cout << "wrv._obj: " << &wrv._obj << std::endl; 

  
  return 0;
}