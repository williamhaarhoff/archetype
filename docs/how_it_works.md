```cpp
ARCHETYPE_DEFINE(basic_interface, (
  ARCHETYPE_METHOD(int, func0, const char *)
))
```

This expands to a structure that contains four substructures. These are:

-  **A protected component structure.** This implements the vtable as a stackable layer, but does not have a public interface. 
-  **A public view class.** Stacks the component on top of a base, and provides the public bind() function while keeping the internal component layer private. 
-  **A public ptr class.** This gives an alternative public interface that allows using pointer syntax.
-  **A public templated check<T> structure.** This is used for testing and verifying that type T meets the interface specification using SFINAE

## Component structure:
Almost all of the core functionality exists within the protected component that exists within the `basic_interface` struct. I'll start with a stripped down version, and explain how the functionality is build up. 

The component itself is templated on a base class. It will inherit from a particular base, and stack its functionality on top without using any virtuals. All the component cares about is being able to access the `B::_obj` that is a `void *` protected member variable defined by the base class. This `B::_obj` is going to store a `void *` of the object that will get bound. 

```cpp
template <typename B = archetype::Base> 
class component : public B {
  protected:
  using B::_obj;
};
```

For the component to actully be useful, it needs to implement the interface that we've defined. It does this by implementing a `vtable` through function pointers and functions. In this case we have a protected function pointer: `int (*_func0_stub)(void *obj, const char *);` 

```cpp
template <typename B = archetype::Base> 
class component : public B {
  public:
  int func0(const char *arg0) { return _func0_stub(_obj, arg0); }

  protected:
  int (*_func0_stub)(void *obj, const char *);
  using B::_obj;
};
```