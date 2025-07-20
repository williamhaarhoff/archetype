

#include <cstddef>
#include <iostream>


struct DummyVTable {

  template<typename T>
  static DummyVTable * make_vtable() {
    static DummyVTable vtablet;
    return &vtablet;
  }

  template <typename T>
  void bind() {}
  
};



struct writable
{
  template<typename BaseVTable = DummyVTable>
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

  struct view
  {
    void * obj;
    vtable<> * vtbl;
    int write(const char * arg0, int arg1) { return vtbl->write(obj, arg0, arg1); }
  };

  template<typename T>
  static view make_view(T & t)
  {
    view v;
    v.obj = static_cast<void *>(&t);
    v.vtbl = vtable<>::make_vtable<T>();
    return v;
  }
};


struct readable
{
  template<typename BaseVTable = DummyVTable>
  struct vtable : public BaseVTable
  {
    int (*read)(void * obj, const char *, int);
    
    template<typename T>
    void bind()
    {
      this->BaseVTable::template bind<T>();

      read = [](void * obj, const char * arg0, int arg1) -> int {
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

  struct view
  {
    void * obj;
    vtable<> * vtbl;
    int read(const char * arg0, int arg1) { return vtbl->read(obj, arg0, arg1); }
  };

  template<typename T>
  static view make_view(T & t)
  {
    view v;
    v.obj = static_cast<void *>(&t);
    v.vtbl = vtable<>::make_vtable<T>();
    return v;
  }
};

struct readwritable
{
  template<typename BaseVTable = DummyVTable>
  struct vtable : public BaseVTable
  {
    int (*read)(void * obj, const char *, int);
    
    template<typename T>
    void bind()
    {
      this->BaseVTable::template bind<T>();
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

  struct view
  {
    void * obj;
    vtable<> * vtbl;
    int read(const char * arg0, int arg1) { return vtbl->read(obj, arg0, arg1); }
  };

  template<typename T>
  static view make_view(T & t)
  {
    view v;
    v.obj = static_cast<void *>(&t);
    v.vtbl = vtable<>::make_vtable<T>();
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
  writable::view v = writable::make_view(wr);

  v.write("hello\n", 7);
  return 0;
}