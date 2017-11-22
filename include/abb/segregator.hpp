#pragma once

#include "abb/block.hpp"
#include "abb/reallocation_helpers.hpp"


namespace abb {

    //----------------------------------------------------------------------------------------------
    template
    <
        // The segregating parameter in bytes
        size_t _Threshold
        // If the requested size is less or equals _Threshold, the _SmallAllocator will be picked up.
        , typename _SmallAllocator
        // If the requested size is strictly greater than _Threshold, the _LargeAllocator will be picked up.
        , typename _LargeAllocator
    >
    class segregator
        : public _SmallAllocator
        , public _LargeAllocator
    {
    public:
        //------------------------------------------------------------------------------------------
        static constexpr auto alignment = const_max(_SmallAllocator::alignment, _LargeAllocator::alignment);

    public:
        //------------------------------------------------------------------------------------------
        // Allocator interface
        block allocate(size_t size)
        {
            return size <= _Threshold
                ? _SmallAllocator::allocate(size)
                : _LargeAllocator::allocate(size);
        }

        //------------------------------------------------------------------------------------------
        void deallocate(block &b)
        {
            b.size <= _Threshold
                ? _SmallAllocator::deallocate(b)
                : _LargeAllocator::deallocate(b);
        }

        //------------------------------------------------------------------------------------------
        bool reallocate(block &b, size_t newSize)
        {
            if (handle_common_reallocation_cases(*this, b, newSize))
            {
                return true;
            }

            // Check if we live in the small allocator
            if (b.size <= _Threshold)
            {
                // Check if we can stay in the small allocator
                if (newSize <= _Threshold)
                {
                    return _SmallAllocator::reallocate(b, newSize);
                }

                // We're growing above the small allocator's upper bound, so we have to move to the large one
                return reallocate_and_copy<_SmallAllocator, _LargeAllocator>(*this, *this, b, newSize);
            }

            // If we're down here it means that we live in the large allocator

            // Check if we're shrinking below the large allocator's lower bound, if so we have to move to the small one
            if (newSize <= _Threshold)
            {
                return reallocate_and_copy<_LargeAllocator, _SmallAllocator>(*this, *this, b, newSize);
            }

            // We can stay in the large allocator
            return _LargeAllocator::reallocate(b, newSize);
        }

        //------------------------------------------------------------------------------------------
        bool owns(const block &b) const
        {
            return b.size <= _Threshold
                ? _SmallAllocator::owns(b)
                : _LargeAllocator::owns(b);
        }
    };

} /*abb*/
