#include <iostream>

#include "abb.hpp"

//--------------------------------------------------------------------------------------------------
using namespace abb::units;


//--------------------------------------------------------------------------------------------------
void test_linear_allocator()
{
    using alloc_t = abb::stack_linear_allocator<128_B>;
    alloc_t allocator;

    auto b0 = allocator.allocate(16);
    assert(b0.size >= 16);

    auto b1 = allocator.allocate(100);
    assert(b1.size >= 100);

    auto b2 = allocator.allocate(20);
    assert(b2.size == 0);

    allocator.deallocate(b1);

    b2 = allocator.allocate(20);
    assert(b2.size >= 20);
}


//--------------------------------------------------------------------------------------------------
int main()
{
    test_linear_allocator();

    return EXIT_SUCCESS;
}