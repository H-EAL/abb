#pragma once

//------------------------------------------------------------------------------------------
// This is a define for the sole purpose of having a different color 
#define nullblock (block{nullptr, 0})

//------------------------------------------------------------------------------------------
// Workaround for CLANG lack of method SFINAE
#define enable_if_workaround_t(expression) bool _CLANGWorkaround = true, typename = typename std::enable_if<((expression) && _CLANGWorkaround)>::type


namespace abb {

    //----------------------------------------------------------------------------------------------
    struct block
    {
        void  *ptr;
        size_t size;

        block(void *p, size_t s)
            : ptr(p)
            , size(s)
        {}

        block()
            : block(nullptr, 0)
        {}
    };

    //----------------------------------------------------------------------------------------------
    inline constexpr size_t round_to_alignment(size_t size, size_t alignment)
    {
        return size + ((size % alignment) == 0 ? 0 : alignment - (size % alignment));
    }

    //----------------------------------------------------------------------------------------------
    inline constexpr bool is_aligned(size_t size, size_t alignment)
    {
        return (size % alignment) == 0;
    }

} /*abb*/
