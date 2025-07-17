#include "archetype/archetype.h"
#include <iostream>
#include <cstring>
#include <string>
#include <vector>

// Define our archetypes
ARCHETYPE_DEFINE(writable, (
  ARCHETYPE_METHOD(int, write, const char *, size_t)
))

ARCHETYPE_DEFINE(readable, (
  ARCHETYPE_METHOD(int, read, char *, size_t)
))

ARCHETYPE_COMPOSE(readwritable, readable, writable)



// Here is a collection of readers and writers created in different ways
class Writer {
public:
  int write(const char *buf, size_t size) {
    int n = 0;
    while (size--) { std::cout << buf[n++]; }
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

// Define interfaces 


// Stateless mixin read API
template <typename R> struct ReadAPI : public R {
  ARCHETYPE_CHECK(readable, R)
  using R::R;
  using R::read;
  int read_api(char *buf, size_t size) { return this->read(buf, size); }
};


// Stateful mixin APIs
template <typename  W> struct StatefulWriteAPI : public W {
  ARCHETYPE_CHECK(writable, W);
  using W::W;
  using W::write;
  size_t write_api(const char *buf) {
    std::string header = "count: " + std::to_string(count++)+ " ";
    int size = this->write(header.c_str(), header.size());
    size += this->write(buf, strlen(buf));
    return size;
  }
  int count = 0;
};

int main()
{

  if (__cplusplus == 202302L)
    std::cout << "C++23";
  else if (__cplusplus == 202002L)
    std::cout << "C++20";
  else if (__cplusplus == 201703L)
    std::cout << "C++17";
  else if (__cplusplus == 201402L)
    std::cout << "C++14";
  else if (__cplusplus == 201103L)
    std::cout << "C++11";
  else if (__cplusplus == 199711L)
    std::cout << "C++98";
  else
    std::cout << "pre-standard C++." << __cplusplus;
  std::cout << "\n";


  ComposedReadWriter composed_read_writer;
  NativeReadWriter native_read_writer;
  InherritedReadWriter inherrited_read_writer;
  AbstractWriter * base_ptr = new DerivedReadWriter();

  // Creating standard views
  std::vector<writable::view> views(4);
  views[0].bind(composed_read_writer);
  views[1].bind(native_read_writer);
  views[2].bind(inherrited_read_writer);
  views[3].bind(*base_ptr);

  for (auto & view : views) {
    view.write("hello\r\n", 7);
  }

  // Creating views with pointer syntax
  writable::ptr<> view_ptr; 
  view_ptr.bind(native_read_writer);
  view_ptr->write("hello from view_ptr\r\n", 21);


  delete base_ptr;
  return 0;

}