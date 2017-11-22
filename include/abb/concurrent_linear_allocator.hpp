#pragma once

#include <atomic>

#include "abb/block.hpp"
#include "abb/units.hpp"
#include "abb/mallocator.hpp"
#include "abb/buffer_provider.hpp"


namespace abb {
    
    //----------------------------------------------------------------------------------------------
    // Thread safe version of the linear allocator
    //
    template
    <
        // The size of the block of memory in bytes
          size_t         _BufferSize
        // Alignment of the sub-allocations, the buffer will be itself aligned on this value
        , size_t         _Alignment
        // The allocator responsible for providing the memory to the buffer provider
        , typename       _Allocator
        // The provider of the underlying block of memory
        , template<size_t, size_t, BufferInitMode, typename> class _BufferProvider
    >
    class concurrent_linear_allocator
        : public _BufferProvider<_BufferSize, _Alignment, BufferInitMode::InitOnConstruct, _Allocator>
    {
        //------------------------------------------------------------------------------------------
        using buffer_provider_t = _BufferProvider<_BufferSize, _Alignment, BufferInitMode::InitOnConstruct, _Allocator>;

    public:
        //------------------------------------------------------------------------------------------
        static constexpr auto alignment = _Alignment;

    public:
        //------------------------------------------------------------------------------------------
        concurrent_linear_allocator()
            : p_(buffer_provider_t::buffer_)
        {}

        //------------------------------------------------------------------------------------------
        // Can be moved only if the buffer provider can be moved
        concurrent_linear_allocator(concurrent_linear_allocator &&rhs)
            : buffer_provider_t(std::move(rhs))
            , p_(rhs.p_)
        {
            rhs.p_ = nullptr;
        }

        //------------------------------------------------------------------------------------------
        // Can't be copied
        concurrent_linear_allocator(const concurrent_linear_allocator &) = delete;

    public:
        //------------------------------------------------------------------------------------------
        // Allocator interface
        block allocate(size_t size)
        {
            const auto alignedSize = align(size);
            auto p = p_.load();

            while (hasEnoughSpace(p, alignedSize))
            {
                if (p_.compare_exchange_strong(p, p + alignedSize))
                {
                    return block { p, alignedSize };
                }
            }

            // Out of memory
            return nullblock;
        }

        //------------------------------------------------------------------------------------------
        void deallocate(block &b)
        {
            // We can only deallocate the last allocated block
            auto p = p_.load();
            while (isLastAllocatedBlock(b, p))
            {
                // Rewind the stack
                if (p_.compare_exchange_strong(p, static_cast<uint8_t*>(b.ptr)))
                {
                    return;
                }
            }
        }

        //------------------------------------------------------------------------------------------
        bool reallocate(block &b, size_t newSize)
        {
            if (handle_common_reallocation_cases(*this, b, newSize))
            {
                return true;
            }

            // From here we'll need the aligned size
            const auto alignedNewSize = align(newSize);

            // If we are reallocating the last block on the stack there's room for optimization
            auto p = p_.load();
            while (isLastAllocatedBlock(b, p))
            {
                // Check if there's enough memory left
                if (static_cast<uint8_t*>(b.ptr) + alignedNewSize <= end())
                {
                    // If so, just update the pointer to the new end of the block
                    // note that it may have shrunk or grown
                    if (p_.compare_exchange_strong(p, static_cast<uint8_t*>(b.ptr) + alignedNewSize))
                    {
                        return true;
                    }
                }
                else
                {
                    // Out of memory
                    return false;
                }
            }

            // If we're shrinking the block we're done.
            // Note that this is done _after_ checking if we're the last allocated
            // block so that we can release some memory back to the stack.
            if (b.size >= alignedNewSize)
            {
                // Do not update the size of the block because if we deallocate enough blocks and this block
                // finds itself the last, if its size changes, it won't be recognized as the last allocated block.
                return true;
            }

            // Nothing worked so far, just allocate a new block and copy the data to it
            return reallocate_and_copy(*this, *this, b, newSize);
        }

        //------------------------------------------------------------------------------------------
        bool owns(const block &b) const
        {
            // If the block falls inside the stack we own it
            return (begin() <= b.ptr) && (b.ptr < end()) ;
        }

    public:
        //------------------------------------------------------------------------------------------
        // Allocator augmented interface
        void deallocateAll()
        {
            p_ = buffer_provider_t::buffer_;
        }

    private:
        // Helpers
        //------------------------------------------------------------------------------------------
        size_t align(size_t size) const
        {
            return round_to_alignment(size, alignment);
        }

        //------------------------------------------------------------------------------------------
        const uint8_t* begin() const
        {
            return buffer_provider_t::buffer_;
        }

        //------------------------------------------------------------------------------------------
        const uint8_t* end() const
        {
            return buffer_provider_t::buffer_ + buffer_provider_t::size();
        }

        //------------------------------------------------------------------------------------------
        bool hasEnoughSpace(uint8_t *p, size_t alignedSize) const
        {
            return (p + alignedSize) <= end();
        }

        //------------------------------------------------------------------------------------------
        bool isLastAllocatedBlock(const block &b, uint8_t *p) const
        {
            return (static_cast<uint8_t*>(b.ptr) + b.size) == p;
        }

    private:
        //------------------------------------------------------------------------------------------
        // A pointer to the top of the stack
        std::atomic<uint8_t*> p_;
    };

    //------------------------------------------------------------------------------------------
    // Shortcut to a linear allocator using a buffer on the stack
    template<size_t _BufferSize, size_t _Alignment = 8_B>
    using concurrent_stack_linear_allocator = concurrent_linear_allocator<_BufferSize, _Alignment, void, stack_buffer_provider>;

    //------------------------------------------------------------------------------------------
    // Shortcut to a linear allocator using a buffer on the heap
    template<size_t _BufferSize, size_t _Alignment = 8_B, typename _Allocator = mallocator>
    using concurrent_heap_linear_allocator = concurrent_linear_allocator<_BufferSize, _Alignment, _Allocator, heap_buffer_provider>;
    
} /*abb*/
