#pragma once

#include "abb/block.hpp"


namespace abb {

    //----------------------------------------------------------------------------------------------
    template<typename _PrimaryAllocator, typename _FallbackAllocator>
    class fallback_allocator
        : public _PrimaryAllocator
        , public _FallbackAllocator
    {
    public:
        //------------------------------------------------------------------------------------------
        static constexpr auto alignment = std::max(_PrimaryAllocator::alignment, _FallbackAllocator::alignment);

    public:
        //------------------------------------------------------------------------------------------
        block allocate(size_t size)
        {
            block b = _PrimaryAllocator::allocate(size);
            if (!b.ptr)
            {
                b = _FallbackAllocator::allocate(size);
            }
            return b;
        }

        //------------------------------------------------------------------------------------------
        void deallocate(block &b)
        {
            if (_PrimaryAllocator::owns(b))
            {
                _PrimaryAllocator::deallocate(b);
            }
            else
            {
                _FallbackAllocator::deallocate(b);
            }
        }

        //------------------------------------------------------------------------------------------
        bool reallocate(block &b, size_t newSize)
        {
            if (handle_common_reallocation_cases(*this, b, newSize))
            {
                return true;
            }

            if (_PrimaryAllocator::owns(b))
            {
                if (_PrimaryAllocator::reallocate(b, newSize))
                {
                    return true;
                }
                return reallocate_and_copy<_PrimaryAllocator, _FallbackAllocator>(*this, *this, b, newSize);
            }
            
            return _FallbackAllocator::reallocate(b, newSize);
        }

        //------------------------------------------------------------------------------------------
        bool owns(const block &b) const
        {
            return _PrimaryAllocator::owns(b) || _FallbackAllocator::owns(b);
        }

        //------------------------------------------------------------------------------------------
        void deallocateAll()
        {
            _PrimaryAllocator::deallocateAll();
            _FallbackAllocator::deallocateAll();
        }
    };

} /*abb*/
