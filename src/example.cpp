// #include "example.h"
#include <iostream>
#include <cstring>
#include "archetype/archetype.h"

// Concepts
#if __cplusplus >= 202002L
#include <concepts>
template <typename T>
concept Writable = requires(T t, const char *buf, size_t size) {
  { t.write(buf, size) } -> std::convertible_to<int>;
};

template <typename T>
concept Readable = requires(T t, char *buf, size_t size) {
  { t.read(buf, size) } -> std::convertible_to<int>;
};

template <typename T>
concept ReadWrite = Readable<T> && Writable<T>;
#else
#define Writable typename
#define Readable typename
#define ReadWrite typename
#endif


// Stateless interfaces
template <typename W> class WriteInterface : public W 
{
  public:
  using W::W;
  using W::write;
  size_t write_api(const char * buf, size_t size) { return this->write(buf, size); }
};

template<Readable R>
class ReadInterface : public R
{
  public:
  using R::R;
  using R::read;
  int read_api(char * buf, size_t size) { return this->read(buf, size); }
};

// Composed interfaces
template<ReadWrite RW>
class ReadWriteInterface : public ReadInterface<WriteInterface<RW>>
{
};

// Bases
class Base
{
  public:
  virtual ~Base() {};
  
  template<typename T>
  void bind(T & t)
  {
    _obj = static_cast<void *>(&t);
  }

  protected:
  void * _obj;
};

template<typename T=Base>
class WritableBase : public T
{ 
  public:
  int write(const char * buf, size_t size)
  {
    return _write_callstub(_obj, buf, size);
  }

  public:
  template<Writable W>
  void bind(W & w)
  {
    this->T::bind(w); // call immediate parent - which chains all the way down
    _write_callstub = [](void * obj, const char * buf, size_t size)->int
    {
      return static_cast<W*>(obj)->write(buf, size);
    };
  }

  protected:
  int(*_write_callstub)(void*, const char *, size_t);
  using T::_obj;
};

template<typename T=Base>
class ReadableBase : public T
{
  public:
  int read(char * buf, size_t size)
  {
    return _read_callstub(_obj, buf, size);
  }

  public:
  template<Readable R>
  void bind(R & r)
  {
    this->T::bind(r); // call immediate parent - which chains all the way down
    _read_callstub = [](void * obj, char * buf, size_t size) -> int
    {
      return static_cast<R*>(obj)->read(buf, size);
    };
  }

  protected:
  using T::_obj;
  int(*_read_callstub)(void*, char *, size_t);
};


// These are implementations provided by an external library
class Writer {
public:
  int write(const char *buf, size_t size) {
    int n = 0;
    while (size--) {
      std::cout << buf[n++];
    }
    std::cout << std::flush;
    return n;
  }
};

class Reader {
public:
  int read(char *buf, size_t size) { return std::fread(buf, 1, size, stdin); }
};

class InherritedReadWriter : public Writer {
public:
  int read(char *buf, size_t size) { return std::fread(buf, 1, size, stdin); }
};

class NativeReadWriter {
public:
  int read(char *buf, size_t size) { return std::fread(buf, 1, size, stdin); }
  int write(const char *buf, size_t size) {
    int n = 0;
    while (size--) {
      std::cout << buf[n++];
    }
    std::cout << std::flush;
    return n;
  }
};

class ComposedReadWriter {
public:
  int read(char *buf, size_t size) { return reader.read(buf, size); }
  int write(const char *buf, size_t size) { return writer.write(buf, size); }

private:
  Writer writer;
  Reader reader;
};


class WriteDependentClass
{
  public:

  template <Writable W>
  void set_port(W & target_port)
  {
    port.bind(target_port);
  }

  WriteInterface<WritableBase<>> port;
};


class ReadWriteDependentClass
{
  public:

  template <ReadWrite RW>
  void set_port(RW & target_port)
  {
    port.bind(target_port);
  }

  ReadWriteInterface<ReadableBase<WritableBase<>>> port;
};


template<template<typename> class Interface>
struct rw_ptr
{
  private:
  using base = Interface<ReadableBase<WritableBase<>>>;
  base impl;

  public:

  template<ReadWrite RW>
  void bind(RW & ref) { impl.bind(ref); }

  base& operator*() { return &impl;}
  const base& operator*() const { return &impl;}

  base* operator->() { return &impl; }
  const base* operator->() const { return &impl; }
};


DEFINE_ARCHETYPE(Writable2, (
  DEFINE_METHOD(size_t, write, const char *, size_t)
))

struct ReadWriteArchetype
{
  // using compat = ReadWrite;
  using base = ReadableBase<WritableBase<>>;
  


  template<template<typename> class Interface>
  struct ptr
  {
    private:
    using T = Interface<ReadableBase<WritableBase<>>>;
    T impl;

    public:
    template<ReadWrite RW>
    void bind(RW & ref) { impl.bind(ref); }

    T & operator*() { return &impl;}
    const T & operator*() const { return &impl;}

    T * operator->() { return &impl; }
    const T * operator->() const { return &impl; }
  };
};




#include <type_traits>

// Trait to detect if T has a member function void foo(int)
template <typename, typename = std::void_t<>>
struct has_foo : std::false_type {};

template <typename T>
struct has_foo<T, std::void_t<decltype(std::declval<T>().foo(std::declval<const int *>()))>>
    : std::is_same<void, decltype(std::declval<T>().foo(std::declval<const int *>()))> {};

// Concept-like check
template <typename T>
void check_concept() {
    static_assert(has_foo<T>::value, "T must have a method void foo(int)");
}


template<typename, typename = void>
struct has_exact_foo : std::false_type {};

template<typename T>
struct has_exact_foo<T, std::void_t<decltype(static_cast<void (T::*)(const int *)>(&T::foo))>> : std::true_type {};

template <typename T>
void check_exact_concept() {
    static_assert(has_exact_foo<T>::value, "T must have a method void foo(const int *)");
}



template<typename, typename = void>
struct has_void_foo : std::false_type {};

template<typename T>
struct has_void_foo<T, std::void_t<decltype(static_cast<void (T::*)()>(&T::foo))>> : std::true_type {};

template <typename T>
void check_exact_void_foo_concept() {
    static_assert(has_void_foo<T>::value, "T must have a method void foo()");
}


struct VoidFoo
{
  void foo();
};

struct Good {
    void foo(const int *) {}
};

struct Bad {
    void foo(int *) {}
};








int main() 
{
  if (__cplusplus == 202302L) std::cout << "C++23";
  else if (__cplusplus == 202002L) std::cout << "C++20";
  else if (__cplusplus == 201703L) std::cout << "C++17";
  else if (__cplusplus == 201402L) std::cout << "C++14";
  else if (__cplusplus == 201103L) std::cout << "C++11";
  else if (__cplusplus == 199711L) std::cout << "C++98";
  else std::cout << "pre-standard C++." << __cplusplus;
  std::cout << "\n";


  check_exact_concept<Good>();
  // check_exact_concept<Bad>();
  check_exact_void_foo_concept<VoidFoo>();
  
  
  Writer nw;
  ComposedReadWriter crw;

  // static casting
  WriteInterface<Writer> * write_iface = static_cast<WriteInterface<Writer>*>(&nw);
  write_iface->write_api("hello from writer with extended write api\n", 42);

  // augmentation
  WriteInterface<Writer> augmented_writer;
  augmented_writer.write_api("hello from augmented writer with write api\n", 43);

  // generic concept ptr
  ReadWriteInterface<ReadWriteArchetype::base> base;
  base.bind(crw);
  base.write_api("hello from generic concept base!\n", 32);
  
  ReadWriteArchetype::ptr<ReadWriteInterface> ptr;
  ptr.bind(crw);
  ptr->write_api("hello from generic concept ptr!\n", 32);


  WriteInterface<Writable2::base<>> writable2_base;
  writable2_base.bind(nw);
  writable2_base.write_api("hello from macro generated write api!\n", 39); 

  Writable2::ptr<WriteInterface> writable2_ptr;
  writable2_ptr.bind(nw);
  writable2_ptr->write_api("hello from macro generated ptr write api\n", 41);


  char buf[4096];
  int read = 0;
  while(1)
  {
    read = ptr->read_api(buf, sizeof(buf));
    ptr->write_api(buf, read);
  }

}
