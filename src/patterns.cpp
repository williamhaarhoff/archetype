

#include <cstddef>
#include <iostream>
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


class Writer {
public:
  int write(const char *buf, size_t size) {
    int n = 0;
    while (size--) { std::cout << buf[n++]; }
    std::cout << std::flush;
    return n;
  }
};



int main()
{
  Writer w;
  writable::view v = writable::make_view(w);

  v.write("hello\n", 7);
  return 0;
}