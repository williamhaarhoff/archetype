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

DEFINE_ARCHETYPE(testing, (
  DEFINE_METHOD(int, test, char *, size_t)
))



COMPOSE_ARCHETYPE(readwritable, readable, writable)

COMPOSE_ARCHETYPE(testreadwritable, testing, readwritable)




#define ARCHETYPE_CHECK_OLD(A, T)\
  typename T, \
  typename = typename std::enable_if<A::check<T>::value>::type

#define ARCHETYPE_CHECK(ARCHETYPE, TYPE)\
static_assert(ARCHETYPE::check<TYPE>::value, STRINGIFY(TYPE must satisfy ARCHETYPE::check));


// Stateless interfaces
template <typename  W> class WriteInterface : public W {
  ARCHETYPE_CHECK(writable, W)
public:
  using W::W;
  using W::write;
  size_t write_api(const char *buf, size_t size) {
    return this->write(buf, size);
  }
};

template <typename W> class WriteInterfacePlain : public W {
  ARCHETYPE_CHECK(writable, W)
  public:
  using W::W;
  using W::write;
  size_t write_api(const char *buf, size_t size) {
    return this->write(buf, size);
  }
};




// template <typename W, std::true_type> 
// class WriteInterfaceTagged : public W {
// public:
//   using W::W;
//   using W::write;
//   size_t write_api(const char *buf, size_t size) {
//     return this->write(buf, size);
//   }
// };







template <typename R> class ReadInterface : public R {
  ARCHETYPE_CHECK(readable, R)
public:
  using R::R;
  using R::read;
  int read_api(char *buf, size_t size) { return this->read(buf, size); }
};

// Composed interfaces
// template <ARCHETYPE_CHECK(readwritable, RW)>
// class ReadWriteInterface : public ReadInterface<WriteInterface<RW>> {};


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


class Tester
{
  public:
  int test(char * buf, size_t size);
};

class TesterReaderWriter 
: public Tester
, public Writer
, public Reader
{

};

template<class C>
class helper
{
  public:
  template<typename T=archetype::Base>
  using get_inaccesible = typename C::template inaccesible_component<T>;
};

class A{
  public:
  class wrapping;
  friend class helper<A>; 

  protected:
  template<typename T = archetype::Base>
  class inaccesible_component : public T
  {
    protected:
    template<typename X>
    void bind(X & obj) { T::bind(obj); std::cout << "A::inaccesible_component bound" << std::endl; }

    public:
    friend class wrapping;
  };
  
  public:
  class component : public inaccesible_component<>
  {
    public:
    template<typename X>
    void bind(X & obj) { inaccesible_component<>::bind(obj); std::cout << "A::component bound" << std::endl; }
  };

  class wrapping
  {
    public:
    template<typename X>
    void bind(X & obj){
      std::cout << "A::wrapping bound" << std::endl;
      impl.bind(obj);  // OK now
    }
    private:
    inaccesible_component<> impl;
  };
};

class B{
  public:
  class wrapping;
  friend class helper<B>; 

  protected:
  template<typename T = archetype::Base>
  class inaccesible_component : public T
  {
    protected:
    template<typename X>
    void bind(X & obj) { T::bind(obj); std::cout << "B::inaccesible_component bound" << std::endl; }

    public:
    friend class wrapping;
  };
  
  public:
  class component : public inaccesible_component<>
  {
    public:
    template<typename X>
    void bind(X & obj) { inaccesible_component<>::bind(obj); std::cout << "B::component bound" << std::endl; }
  };

  class wrapping
  {
    public:
    template<typename X>
    void bind(X & obj){
      std::cout << "B::wrapping bound" << std::endl;
      impl.bind(obj);  // OK now
    }
    private:
    inaccesible_component<> impl;
  };
};


class C{
  public:
  class wrapping;
  friend class helper<C>; 

  protected:
  template<typename T = archetype::Base>
  class inaccesible_component : public helper<B>::get_inaccesible<helper<A>::get_inaccesible<T>> 
  {
    public:
    friend class wrapping;
  };
  
  public:
  class component : public inaccesible_component<>
  {
    public:
    template<typename X>
    void bind(X & obj) { inaccesible_component::bind(obj); std::cout << "C::component bound" << std::endl; }
  };

  class wrapping
  {
    public:
    template<typename X>
    void bind(X & obj){
      std::cout << "C::wrapping bound" << std::endl;
      impl.bind(obj);  // OK now
    }
    private:
    inaccesible_component<> impl;
  };
};





int main()
{
  // finder<A>::accessible<archetype::Base> instance;
  helper<A>::get_inaccesible<helper<B>::get_inaccesible<>> ab_chain;
  archetype::helper<writable>::get<archetype::helper<readable>::get<>> wr_chain;
  

  Writer w;
  writable::view w_view;
  WriteInterface<writable::view> wapi_view;
  writable::ptr<WriteInterfacePlain> w_ptr;


  ComposedReadWriter crw;
  readwritable::view rwv; 
  rwv.bind(crw);

  static_assert(readwritable::check<ComposedReadWriter>(), "Failed composed read writer check" );

  writable::ptr<> wptr;
  wptr.bind(w);
  wptr->write("asdfasdfasdf", 5);

  readwritable::ptr<> rwptr;
  rwptr.bind(crw);

  TesterReaderWriter trw;
  testreadwritable::view trwv;
  testreadwritable::ptr<> trwvp;
  trwv.bind(trw);
  trwvp.bind(trw);
  




  wapi_view.bind(w);
  // ptr<WriteInterface> xx;
  // w_ptr.bind(w);
  

  
  C::component c;
  c.bind(w);

  C::wrapping w_c;
  w_c.bind(w);

  
  
  
  
  
  // static_assert(!readwritable::check<Writer>::value, "Candidate being checked fails to match");
  // static_assert(has_exact_foo<FooCandidate2>::value, "FooCandidate2 does not match");
  // DoTheThing<FooCandidate> ext1;
  // WriteInterfacePlain<ComposedReadWriter> we;

  static_assert(writable::check<Writer>::value, "writer check");

  // WriteInterfacePlain<Reader> wir;

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
  
//   WriteInterface<writable::view> augmented_write_base;
//   augmented_write_base.bind(crw);
//   augmented_write_base.write_api("hello from macro generated write api!\n", 39);

//   // using a generic read write base
//   ReadWriteInterface<readwritable::view> augmented_readwrite_base;
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
