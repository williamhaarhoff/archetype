#include "archetype/archetype.h"
#include <iostream>


DEFINE_ARCHETYPE(something, (DEFINE_METHOD(int, myname, float)) )

int main()
{
    std::cout << "fast build worked" << std::endl;
    something::view sv;
    return 0;
}