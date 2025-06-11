# 0 "archetype.h"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/nix/store/r25srliigrrv5q3n7y8ms6z10spvjcd9-glibc-2.40-66-dev/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "archetype.h"
# 175 "archetype.h"
template<typename B = archetype::base> class Writable : public B { public: size_t write(const char * arg0, size_t arg1) { return _write_stub(_obj, arg0, arg1); } public: size_t write(const char * arg0) { return _write_stub(_obj, arg0); } public: template<typename T> void bind(T & t) { this->T::bind(w); _write_stub = [](void * obj, const char * arg0, size_t arg1) -> size_t { return static_cast<T*>(obj)->write(arg0, arg1); } _write_stub = [](void * obj, const char * arg0) -> size_t { return static_cast<T*>(obj)->write(arg0); } } protected: using B::_obj size_t (*_write_stub)(void * obj, const char *, size_t); size_t (*_write_stub)(void * obj, const char *); };



  ;
