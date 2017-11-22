#pragma once

#include "abb/block.hpp"
#include "abb/units.hpp"


namespace abb {

    class null_allocator
    {
    public:
        static constexpr auto alignment = 8_B;

    public:
        block allocate(size_t size)
        {
            return block{ nullptr, 0 };
        }

        void deallocate(block &b)
        {
            assert(b.ptr == nullptr);
        }

        bool reallocate(block &b, size_t newSize)
        {
            return b.ptr == nullptr;
        }

        bool owns(const block &b) const
        {
            return b.ptr == nullptr;
        }
    };

} /*abb*/
