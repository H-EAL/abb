#pragma once

#include "abb/block.hpp"


namespace abb {

    //----------------------------------------------------------------------------------------------
    template
    <
        typename _Allocator
        , size_t _AllocationPattern   = 0xAA
        , size_t _DeallocationPattern = 0xFF
    >
    class stamp
        : public _Allocator
    {
    public:
        //------------------------------------------------------------------------------------------
        static constexpr auto alignment = _Allocator::alignment;

    public:
        //------------------------------------------------------------------------------------------
        block allocate(size_t size)
        {
            block b = _Allocator::allocate(size);
            if (b.ptr)
            {
                memset(b.ptr, _AllocationPattern, b.size);
            }
            return b;
        }

        //------------------------------------------------------------------------------------------
        void deallocate(block &b)
        {
            if (b.ptr)
            {
                memset(b.ptr, _DeallocationPattern, b.size);
            }
            _Allocator::deallocate(b);
        }
    };

} /*abb*/
