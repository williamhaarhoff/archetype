#include "archetype/archetype.h"
#include <iostream>
#include <cstring>
#include <string>

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
ARCHETYPE_DEFINE(writable, (
  ARCHETYPE_METHOD(int, write, const char *, size_t)
))

ARCHETYPE_DEFINE(readable, (
  ARCHETYPE_METHOD(int, read, char *, size_t)
))

ARCHETYPE_COMPOSE(readwritable, readable, writable)



// Stateless mixin write API
template <typename  W> struct WriteAPI : public W {
  ARCHETYPE_CHECK(writable, W);
  using W::W;
  using W::write;
  size_t write_api(const char *buf) {
    return this->write(buf, strlen(buf));
  }
};

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


  // Create a writeable::view that can view DerivedReadWriter
  DerivedReadWriter derived_read_writer_instance;
  writable::view write_view_0;
  write_view_0.bind(derived_read_writer_instance);
  write_view_0.write("Hello from write view\r\n", 96);
  

  // Augment the writable::view with the write api
  Writer writer;
  WriteAPI<writable::view> augmented_write_view_0;
  augmented_write_view_0.bind(writer);
  augmented_write_view_0.write_api(
    "Hello from augmented view, using WriteAPI::write_api()\r\n"
  );

  // Augmentation without the view
  // This is only safe to do because WriteAPI has no virtuals or member variables
  ComposedReadWriter composed_read_writer_instance;
  WriteAPI<ComposedReadWriter> * pure_augmentation_0 = static_cast<WriteAPI<ComposedReadWriter> *>(&composed_read_writer_instance);  
  pure_augmentation_0->write_api(
    "Hello from augmented view of raw pointer\r\n"
  );

  // Stateful Augmentation 
  // This is safe because we are using an instance not a pointer
  AbstractWriter * abstract_writer_ptr = static_cast<AbstractWriter*>(&derived_read_writer_instance);
  StatefulWriteAPI<writable::view> stateful_augmented_view;
  stateful_augmented_view.bind(*abstract_writer_ptr);  
  stateful_augmented_view.write_api("Hello from stateful augmentation\r\n");
  stateful_augmented_view.write_api("Hello from stateful augmentation\r\n");
  stateful_augmented_view.write_api("Hello from stateful augmentation\r\n");

  return 0;

}