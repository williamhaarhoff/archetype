#include "archetype/archetype.h"
#include <iostream>
#include <cstring>

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
  using T::_obj;
  public:
  int write(const char * buf, size_t size)
  {
    return _write_callstub(_obj, buf, size);
  }

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
};




// template<typename T=Base>
// class ReadableBase : public T 
// {
//   public:
//   virtual int read(char *buf, size_t size) = 0;
// };


// Adaptors
// template <Writable T>
// class WritableAdaptor : public WritableBase<> {
//   public:
//   WritableAdaptor(T *ptr) : impl(ptr) {}
//   virtual size_t write(const char *buf, size_t size) override { return impl->write(buf, size); }
//   private:
//   T *impl;
// };


// template <Readable T>
// class ReadableAdaptor : public ReadableBase<> {
//   public:
//   ReadableAdaptor(T *ptr) : impl(ptr) {}
//   virtual int read(char *buf, size_t size) override { return impl->read(buf, size); }
//   private:
//   T *impl;
// };


// template <ReadWrite T>
// class ReadWriteAdaptor : public ReadableBase<WritableBase<>> {
//   public:
//   ReadWriteAdaptor(T *ptr) : impl(ptr) {}
//   virtual size_t write(const char *buf, size_t size) override { return impl->write(buf, size); }
//   virtual int read(char *buf, size_t size) override { return impl->read(buf, size); }
//   private:
//   T *impl;
// };


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



// struct WritableArchetype
// {
//   using base = WritableBase<>;

//   template <Writable W>
//   using adaptor = WritableAdaptor<W>;
// };


// struct ReadableArchetype
// {
//   using base = ReadableBase<>;

//   template <Readable R>
//   using adaptor = ReadableAdaptor<R>;
// };


// struct ReadWriteArchetype
// {
//   using base = ReadableBase<WritableBase<>>;

//   template <ReadWrite RW>
//   using adaptor = ReadWriteAdaptor<RW>;
// };


// class ReadWriteDependentClass
// {
//   public:

//   template <ReadWrite RW>
//   void set_port(RW * target_port)
//   {
//     static ReadWriteArchetype::adaptor<RW> port_adaptor(target_port);
//     port = static_cast<ReadWriteInterface<ReadWriteArchetype::base>*>(static_cast<ReadWriteArchetype::base*>(&port_adaptor));
//   }

//   ReadWriteInterface<ReadWriteArchetype::base>* port = nullptr;
// };


// class WriteDependentClass2
// {
//   public:

//   template <Writable W>
//   void set_port(W * target_port)
//   {
//     static WritableArchetype::adaptor<W> port_adaptor(target_port);
//     port = static_cast<WriteInterface<WritableArchetype::base>*>(static_cast<WritableArchetype::base*>(&port_adaptor));
//   }

//   WriteInterface<WritableArchetype::base> * port = nullptr;
// };

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

  WriteInterface<ReadableBase<WritableBase<>>> port;
};








int main() 
{
  Writer nrw;
  ComposedReadWriter crw;

  WriteDependentClass wdc;
  // ReadWriteDependentClass rwdc;


  // ErasedWritable ew;
  // ew.bind(crw);
  // ew.write("hello from erased interface!", 28);


  wdc.set_port(nrw);
  wdc.port.write_api("hello from writer", 17);

  // wdc.set_port(crw);
  // wdc.port->write_api("hello from composed read writer!", 32);

  // rwdc.set_port(crw);





  // char buf[4096];
  // int read = 0;
  // while(1)
  // {
    
  //   read = rwdc.port->read_api(buf, sizeof(buf));
  //   rwdc.port->write_api(buf, read);
  // }

}
