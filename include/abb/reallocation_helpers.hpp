#pragma once

#include <algorithm>

#include "abb/block.hpp"


namespace abb {

    //----------------------------------------------------------------------------------------------
    template<typename _Allocator>
    inline bool handle_common_reallocation_cases(_Allocator &allocator, block &b, size_t newSize)
    {
        // Nothing to do if we are reallocating to the same size
        if (b.size == round_to_alignment(newSize, _Allocator::alignment))
        {
            return true;
        }

        // If the new size is 0 it is equivalent to deallocating the block
        if (newSize == 0)
        {
            allocator.deallocate(b);
            return true;
        }

        // The old block is empty, same as allocating a new block
        if (b.ptr == nullptr)
        {
            b = allocator.allocate(newSize);
            return true;
        }

        return false;
    }

    //----------------------------------------------------------------------------------------------
    inline void copy_block(block &dstBlock, const block &srcBlock)
    {
        std::memcpy(dstBlock.ptr, srcBlock.ptr, std::min(dstBlock.size, srcBlock.size));
    }

    //----------------------------------------------------------------------------------------------
    template<typename _FromAllocator, typename _ToAllocator>
    inline bool reallocate_and_copy(_FromAllocator &fromAllocator, _ToAllocator &toAllocator, block &b, size_t newSize)
    {
        auto newBlock = toAllocator.allocate(newSize);
        if (!newBlock.ptr)
        {
            return false;
        }

        copy_block(newBlock, b);
        fromAllocator.deallocate(b);
        b = newBlock;
        return true;
    }

} /*abb*/
