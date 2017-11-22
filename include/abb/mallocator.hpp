#pragma once

#include "abb/units.hpp"
#include "abb/block.hpp"
#include "abb/reallocation_helpers.hpp"


namespace abb {

    //----------------------------------------------------------------------------------------------
    class mallocator
    {
    public:
        //------------------------------------------------------------------------------------------
        static constexpr auto alignment = 8_B;
        //------------------------------------------------------------------------------------------
        static constexpr auto supports_truncated_deallocation = false;

    public:
        //------------------------------------------------------------------------------------------
        block allocate(size_t size)
        {
            return block{ malloc(size), size };
        }

        //------------------------------------------------------------------------------------------
        void deallocate(block &b)
        {
            free(b.ptr);
        }

        //------------------------------------------------------------------------------------------
        bool reallocate(block &b, size_t newSize)
        {
            if (handle_common_reallocation_cases(*this, b, newSize))
            {
                return true;
            }

            auto newBlock = block{ realloc(b.ptr, newSize), newSize };
            if (newBlock.ptr)
            {
                b = newBlock;
                return true;
            }
            return false;
        }
    };


    //----------------------------------------------------------------------------------------------
    template<size_t _Alignment>
    class aligned_mallocator
    {
    public:
        //------------------------------------------------------------------------------------------
        static constexpr auto alignment = _Alignment;
        //------------------------------------------------------------------------------------------
        static constexpr auto supports_truncated_deallocation = false;

    public:
        //------------------------------------------------------------------------------------------
        block allocate(size_t size)
        {
            return block{ _aligned_malloc(size, _Alignment), size };
        }

        //------------------------------------------------------------------------------------------
        void deallocate(block &b)
        {
            _aligned_free(b.ptr);
        }

        //------------------------------------------------------------------------------------------
        bool reallocate(block &b, size_t newSize)
        {
            if (handle_common_reallocation_cases(*this, b, newSize))
            {
                return true;
            }

            auto newBlock = block{ _aligned_realloc(b.ptr, newSize), newSize };
            if (newBlock.ptr)
            {
                b = newBlock;
                return true;
            }
            return false;
        }
    };

} /*abb*/
