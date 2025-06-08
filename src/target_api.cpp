#include "archetype/archetype.h"
#include <iostream>
#include <cstring>




// API
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

template<ReadWrite RW>
class ReadWriteInterface : public ReadInterface<WriteInterface<RW>>
{
};



class ReadWriteDependentClass
{
  public:
  using port_type = ReadWriteInterface<

  template <ReadWrite RW>
  void set_port(RW * target_port)
  {
    static ReadWriteArchetype::adaptor<RW> port_adaptor(target_port);
    port = static_cast<ReadWriteInterface<ReadWriteArchetype::base>*>(static_cast<ReadWriteArchetype::base*>(&port_adaptor));
  }

  ReadWriteInterface<ReadWriteArchetype::base>* port = nullptr;
};


class NativeReadWriter 
{
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






int main() 
{
  NativeReadWriter nwrw;
  ReadWriteDependentClass rwdc;

  rwdc.set_port(&crw);
  rwdc.port->write_api("hello from native read writer!", 30);


  char buf[4096];
  int read = 0;
  while(1)
  {
    read = rwdc.port->read_api(buf, sizeof(buf));
    rwdc.port->write_api(buf, read);
  }

}
