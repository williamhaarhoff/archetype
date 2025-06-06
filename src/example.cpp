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

  size_t write_api(const char *str) 
  {
    if (str == NULL) { return 0; }
    return this->write(str, strlen(str));
  }
};

template<Readable R>
class ReadInterface : public R
{
  public:
  using R::R;
  size_t read_api(char *str) 
  {
    return this->read(str);
  }
};

// Composed interfaces
template<ReadWrite RW>
class ReadWriteInterface : public ReadInterface<WriteInterface<RW>>
{
  public:
  using RW::RW;
};

// Bases
class Base
{
  public: 
  virtual ~Base() {};
};

template<typename T=Base>
class WritableBase {
  public:
  virtual size_t write(const char *buf, size_t size) = 0;
};


template<typename T=Base>
class ReadableBase {
  public:
  virtual size_t write(const char *buf, size_t size) = 0;
};


// Adaptors
template <Writable T>
class WritableAdaptor : public WritableBase<> {
  public:
  WritableAdaptor(T *ptr) : impl(ptr) {}
  virtual size_t write(const char *buf, size_t size) override { return impl->write(buf, size); }
  private:
  T *impl;
};


template <Readable T>
class ReadAdaptor : public ReadableBase<> {
  public:
  ReadAdaptor(T *ptr) : impl(ptr) {}
  virtual size_t read(char *buf, size_t size) override { return impl->read(buf); }
  private:
  T *impl;
};


template <ReadWrite T>
class ReadWriteAdaptor : public ReadableBase<WritableBase<>> {
  public:
  ReadWriteAdaptor(T *ptr) : impl(ptr) {}
  virtual size_t write(const char *buf, size_t size) override { return impl->write(buf, size); }
  virtual size_t read(char *buf, size_t size) override { return impl->read(buf); }
  private:
  T *impl;
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
  void set_port(W * target_port)
  {
    static WritableAdaptor<W> port_adaptor(target_port);
    port = static_cast<WriteInterface<WritableBase<>>*>(static_cast<WritableBase<>*>(&port_adaptor));
  }

  WriteInterface<WritableBase<>> * port = nullptr;
};


int main() 
{
  Writer nrw;
  ComposedReadWriter crw;

  WriteDependentClass wdc;
  wdc.set_port(&nrw);
  wdc.port->write_api("hello from writer");

  wdc.set_port(&crw);
  wdc.port->write_api("hello from composed read writer!");
}
