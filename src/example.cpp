// #include "example.h"
#include "archetype/archetype.h"
#include <cstring>
#include <iostream>

DEFINE_ARCHETYPE(writable, (
  DEFINE_METHOD(int, write, const char *, size_t)
))

DEFINE_ARCHETYPE(readable, (
  DEFINE_METHOD(int, read, char *, size_t)
))

COMPOSE_ARCHETYPE(readwritable, readable, writable)


#define ARCHETYPE_CHECK(A, T)\
  typename T, \
  typename = typename std::enable_if<A::check<T>::value>::type


// Stateless interfaces
template <ARCHETYPE_CHECK(writable, W)> class WriteInterface : public W {
public:
  using W::W;
  using W::write;
  size_t write_api(const char *buf, size_t size) {
    return this->write(buf, size);
  }
};

template <ARCHETYPE_CHECK(readable, R)> class ReadInterface : public R {
public:
  using R::R;
  using R::read;
  int read_api(char *buf, size_t size) { return this->read(buf, size); }
};

// Composed interfaces
template <ARCHETYPE_CHECK(readwritable, RW)>
class ReadWriteInterface : public ReadInterface<WriteInterface<RW>> {};


// These are implementations provided by an external library
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





int main()
{
  static_assert(!readwritable::check<Writer>::value, "Candidate being checked fails to match");
  // static_assert(has_exact_foo<FooCandidate2>::value, "FooCandidate2 does not match");
  // DoTheThing<FooCandidate> ext1;
  // WriteExtension<ComposedReadWriter> we;
}



// int main() {
//   if (__cplusplus == 202302L)
//     std::cout << "C++23";
//   else if (__cplusplus == 202002L)
//     std::cout << "C++20";
//   else if (__cplusplus == 201703L)
//     std::cout << "C++17";
//   else if (__cplusplus == 201402L)
//     std::cout << "C++14";
//   else if (__cplusplus == 201103L)
//     std::cout << "C++11";
//   else if (__cplusplus == 199711L)
//     std::cout << "C++98";
//   else
//     std::cout << "pre-standard C++." << __cplusplus;
//   std::cout << "\n";

//   Writer nw;
//   Reader nr;
//   ComposedReadWriter crw;

//   // static casting
//   WriteInterface<Writer> *write_iface =
//       static_cast<WriteInterface<Writer> *>(&nw);
//   write_iface->write_api("hello from writer with extended write api\n", 42);

//   // augmentation
//   WriteInterface<Writer> augmented_writer;
//   augmented_writer.write_api("hello from augmented writer with write api\n",
//                              43);

//   // using the generic base
  
//   WriteInterface<writable::base<>> augmented_write_base;
//   augmented_write_base.bind(crw);
//   augmented_write_base.write_api("hello from macro generated write api!\n", 39);

//   // using a generic read write base
//   ReadWriteInterface<readwritable::base<>> augmented_readwrite_base;
//   augmented_readwrite_base.bind(crw);
//   augmented_readwrite_base.write_api("hello from generic concept base!\n", 32);



//   // ReadWriteArchetype::ptr<ReadWriteInterface> ptr;
//   // ptr.bind(crw);
//   // ptr->write_api("hello from generic concept ptr!\n", 32);

//   // Writable2::assert<Writer>();

//   // Writable2::ptr<WriteInterface> writable2_ptr;
//   // writable2_ptr.bind(nw);
//   // writable2_ptr->write_api("hello from macro generated ptr write api\n", 41);

//   char buf[4096];
//   int read = 0;
//   while (1) {
//     read = augmented_readwrite_base.read_api(buf, sizeof(buf));
//     augmented_readwrite_base.write_api(buf, read);
//   }
// }
