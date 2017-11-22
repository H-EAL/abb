#pragma once

#include "abb/block.hpp"
#include "abb/reallocation_helpers.hpp"


namespace abb {

    //----------------------------------------------------------------------------------------------
    // Placeholder type to specify that we don't need this affix.
    // Note that it can't be used to specify both prefix and suffix at the same time.
    struct no_affix {};

    //----------------------------------------------------------------------------------------------
    template<typename _Allocator, typename _Prefix, typename _Suffix = no_affix>
    class affix_allocator
        : public _Allocator
    {
    public:
        //------------------------------------------------------------------------------------------
        static constexpr auto alignment = _Allocator::alignment;
        //------------------------------------------------------------------------------------------
        static constexpr auto prefix_size = std::is_same_v<_Prefix, no_affix> ? 0ull : round_to_alignment(sizeof(_Prefix), alignment);
        //------------------------------------------------------------------------------------------
        static constexpr auto suffix_size = std::is_same_v<_Suffix, no_affix> ? 0ull : round_to_alignment(sizeof(_Suffix), alignment);

    public:
        //------------------------------------------------------------------------------------------
        static_assert(!(prefix_size == 0 && suffix_size == 0), "Pointless affix_allocator detected.");

    public:
        //------------------------------------------------------------------------------------------
        block allocate(size_t size)
        {
            const auto affixedSize  = prefix_size + size + suffix_size;
            const auto affixedBlock = _Allocator::allocate(affixedSize);
            return toStrippedBlock(affixedBlock);
        }

        //------------------------------------------------------------------------------------------
        void deallocate(block &strippedBlock)
        {
            auto affixedBlock = toAffixedBlock(strippedBlock);
            _Allocator::deallocate(affixedBlock);
        }

        //------------------------------------------------------------------------------------------
        bool reallocate(block &strippedBlock, size_t newSize)
        {
            if (handle_common_reallocation_cases(*this, strippedBlock, newSize))
            {
                return true;
            }

            return reallocate_and_copy(*this, *this, strippedBlock, newSize);
        }

        //------------------------------------------------------------------------------------------
        bool owns(const block &strippedBlock) const
        {
            const auto affixedBlock = toAffixedBlock(strippedBlock);
            return _Allocator::owns(affixedBlock);
        }

    private:
        //------------------------------------------------------------------------------------------
        block toAffixedBlock(const block &strippedBlock) const
        {
            const auto affixedSize = prefix_size + strippedBlock.size + suffix_size;
            return block{ static_cast<uint8_t*>(strippedBlock.ptr) - prefix_size, affixedSize };
        }

        //------------------------------------------------------------------------------------------
        block toStrippedBlock(const block &affixedBlock) const
        {
            const auto strippedSize = affixedBlock.size - prefix_size - suffix_size;
            return block{ static_cast<uint8_t*>(affixedBlock.ptr) + prefix_size, strippedSize };
        }

    public:
        //------------------------------------------------------------------------------------------
        template<enable_if_workaround_t(prefix_size != 0)>
        _Prefix* prefix(const block &strippedBlock)
        {
            return reinterpret_cast<_Prefix*>(static_cast<uint8_t*>(strippedBlock.ptr) - prefix_size);
        }

        //------------------------------------------------------------------------------------------
        template<enable_if_workaround_t(prefix_size != 0)>
        _Prefix* prefix(void *ptr)
        {
            return reinterpret_cast<_Prefix*>(static_cast<uint8_t*>(ptr) - prefix_size);
        }

        //------------------------------------------------------------------------------------------
        template<enable_if_workaround_t(suffix_size != 0)>
        _Suffix* suffix(const block &strippedBlock)
        {
            return reinterpret_cast<_Suffix*>(static_cast<uint8_t*>(strippedBlock.ptr) + strippedBlock.size);
        }
    };

} /*abb*/
