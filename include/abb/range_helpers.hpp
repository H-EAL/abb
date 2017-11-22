#pragma once

#include <limits>
#include "abb/bit_helpers.hpp"


namespace abb {

    //----------------------------------------------------------------------------------------------
    enum DynamicValues : size_t
    {
        dynamic_value = std::numeric_limits<size_t>::max(),
        invalid_index = std::numeric_limits<size_t>::max(),
    };

    //----------------------------------------------------------------------------------------------
    inline constexpr bool is_dynamic_value(size_t v)
    {
        return v == dynamic_value;
    }

    //----------------------------------------------------------------------------------------------
    template<typename T, typename _Differentiator = void>
    struct dynamic_value_t
    {
        dynamic_value_t(T v = dynamic_value)
            : value_(v)
        {}

        size_t value() const    { return value_;                    }
        void   setValue(T v)    { value_ = v;                       }
        bool   is_set() const   { return !is_dynamic_value(value_); }
        operator T()            { return value_;                    }

        T value_;
    };

    //----------------------------------------------------------------------------------------------
    template<typename T, T _Value, typename _Differentiator = void>
    struct static_value_t
    {
        constexpr size_t value()  const { return _Value; }
        constexpr bool   is_set() const { return true;   }
    };

    //----------------------------------------------------------------------------------------------
    struct min_type {};
    //----------------------------------------------------------------------------------------------
    struct max_type {};

    //----------------------------------------------------------------------------------------------
    template<size_t _Min, size_t _Max>
    class range_t
        : public std::conditional<is_dynamic_value(_Min), dynamic_value_t<size_t, min_type>, static_value_t<size_t, _Min, min_type>>::type
        , public std::conditional<is_dynamic_value(_Max), dynamic_value_t<size_t, max_type>, static_value_t<size_t, _Max, max_type>>::type
    {
    public:
        //------------------------------------------------------------------------------------------
        using min_type_t = typename std::conditional<is_dynamic_value(_Min), dynamic_value_t<size_t, min_type>, static_value_t<size_t, _Min, min_type>>::type;
        //------------------------------------------------------------------------------------------
        using max_type_t = typename std::conditional<is_dynamic_value(_Max), dynamic_value_t<size_t, max_type>, static_value_t<size_t, _Max, max_type>>::type;

    public:
        //------------------------------------------------------------------------------------------
        constexpr size_t min() const { return min_type_t::value(); }
        //------------------------------------------------------------------------------------------
        constexpr size_t max() const { return max_type_t::value(); }

        //------------------------------------------------------------------------------------------
        static constexpr bool is_dynamic()
        {
            return (_Min == dynamic_value) && (_Max == dynamic_value);
        }

        //------------------------------------------------------------------------------------------
        static constexpr bool is_valid()
        {
            return _Min <= _Max;
        }

        //------------------------------------------------------------------------------------------
        static constexpr bool is_strictly_in_range(size_t val)
        {
            return (_Min < val) && (val < _Max);
        }

        //------------------------------------------------------------------------------------------
        static constexpr bool is_in_range(size_t val)
        {
            return (_Min <= val) && (val <= _Max);
        }

        //------------------------------------------------------------------------------------------
        void inline setMinMax(size_t minVal, size_t maxVal)
        {
            static_assert(is_dynamic(), "Can't set min max values on a static range");
            min_type_t::setValue(minVal);
            max_type_t::setValue(maxVal);
        }

        //------------------------------------------------------------------------------------------
        static_assert(is_dynamic() || is_valid(), "Invalid range.");
    };

    //----------------------------------------------------------------------------------------------
    using dynamic_range_t = range_t<dynamic_value, dynamic_value>;


    //----------------------------------------------------------------------------------------------
    template<size_t _Min, size_t _Max, size_t _Step>
    struct linear_range_raider
        : public range_t<_Min, _Max>
    {
        //------------------------------------------------------------------------------------------
        static_assert((_Max - _Min) > 0, "Range must be non null");
        //------------------------------------------------------------------------------------------
        static_assert((_Max - _Min) % _Step == 0, "_StepSize must be a multiple of range");

        //------------------------------------------------------------------------------------------
        static constexpr auto num_steps = (_Max - _Min) / _Step;

        //------------------------------------------------------------------------------------------
        static constexpr size_t step_index(size_t val)
        {
            return range_t<_Min, _Max>::is_in_range(val) ? (val - _Min) / _Step : invalid_index;
        }

        //------------------------------------------------------------------------------------------
        static constexpr size_t step_size(size_t)
        {
            return _Step;
        }
    };

    //----------------------------------------------------------------------------------------------
    template<size_t _Min, size_t _Max>
    struct pow2_range_raider
        : public range_t<_Min, _Max>
    {
        //------------------------------------------------------------------------------------------
        static_assert(is_pow2(_Min) && is_pow2(_Max), "_MinSize and _MaxSize must be powers of 2.");

        //------------------------------------------------------------------------------------------
        static constexpr auto pow2bit_min_index = last_bit_set(_Min);
        //------------------------------------------------------------------------------------------
        static constexpr auto pow2bit_max_index = last_bit_set(_Max);
        //------------------------------------------------------------------------------------------
        static constexpr auto num_steps         = pow2bit_max_index - pow2bit_min_index;

        //------------------------------------------------------------------------------------------
        static constexpr size_t step_index(size_t val)
        {
            return last_bit_set(next_pow2(val)) - pow2bit_min_index - 1;
        }

        //------------------------------------------------------------------------------------------
        static constexpr size_t step_size(size_t stepIndex)
        {
            return 1ull << (pow2bit_min_index + stepIndex);
        }
    };

} /*abb*/
