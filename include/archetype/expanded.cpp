# 0 "archetype.h"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/nix/store/r25srliigrrv5q3n7y8ms6z10spvjcd9-glibc-2.40-66-dev/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "archetype.h"
# 158 "archetype.h"
template<typename B> class Writable2 : public B { public: size_t write(const char * arg0, size_t arg1) { return _write_159_0_stub(_obj, arg0, arg1); } public: template<typename T> void bind(T & t) { this->T::bind(t); _write_159_0_stub = [](void * obj, const char * arg0, size_t arg1) -> size_t { return static_cast<T*>(obj)->write(arg0, arg1); }; } protected: using B::_obj; size_t (*_write_159_0_stub)(void * obj, const char *, size_t); };
