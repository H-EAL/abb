#pragma once

#include <intrin.h>


namespace abb {

    namespace details {

        //------------------------------------------------------------------------------------------
        inline constexpr size_t last_bit_set(size_t v)
        {
            return v ? 1ull + last_bit_set(v >> 1ull) : 0ull;
        }
    }

    //----------------------------------------------------------------------------------------------
    inline constexpr size_t is_pow2(size_t v)
    {
        return (v > 0) && ((v & v - 1) == 0);
    }

    //----------------------------------------------------------------------------------------------
    inline constexpr size_t last_bit_set(size_t v)
    {
        return v ? (details::last_bit_set(v) - 1ull) : 0ull;
    }

    //----------------------------------------------------------------------------------------------
    inline constexpr size_t next_pow2(size_t v)
    {
        return is_pow2(v) ? v : (1ull << (last_bit_set(v) + 1ull));
    }

    //----------------------------------------------------------------------------------------------
    inline size_t count_trailing_zeros(size_t v)
    {
        unsigned long bitIndex = 0;
        _BitScanForward64(&bitIndex, v);
        return bitIndex;
    }

} /*abb*/
