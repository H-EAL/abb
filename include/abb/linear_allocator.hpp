#pragma once

#include "abb/block.hpp"
#include "abb/buffer_provider.hpp"


namespace abb {

    //----------------------------------------------------------------------------------------------
    // A linear allocator that allocates blocks of memory in a stack way.
    // It uses a buffer provider as a back end, cf. buffer_provider.h
    // It keeps track of the remaining space by moving a pointer to that buffer.
    // Once it reaches the end of the buffer it falls out of memory.
    //          ______________________________________________________
    // Buffer: |                                                      |
    //         |______________________________________________________|
    //         ^
    //       cursor
    //
    // A1: allocates X bytes:
    //          ______________________________________________________
    // Buffer: |XXXXXXXXXXXXXXX|                                      |
    //         |_______________|______________________________________|
    //                         ^
    //                       cursor
    //
    // A2: allocates Y bytes:
    //          ______________________________________________________
    // Buffer: |XXXXXXXXXXXXXXX|YYYYYY|                               |
    //         |_______________|______|_______________________________|
    //                                ^
    //                              cursor
    //
    // Deallocate A2, as it's the last allocation, we can rewind the cursor by Y bytes:
    //          ______________________________________________________
    // Buffer: |XXXXXXXXXXXXXXX|                                      |
    //         |_______________|______________________________________|
    //                         ^
    //                       cursor
    //
    // A3: allocates Z bytes:
    //          ______________________________________________________
    // Buffer: |XXXXXXXXXXXXXXX|ZZZ|                                  |
    //         |_______________|___|__________________________________|
    //                             ^
    //                           cursor
    //
    // Trying to deallocate A1 (X first bytes) at this point won't do anything as it's not
    // the last allocation.
    //
    template
    <
        // The size of the block of memory in bytes
          size_t         _BufferSize
        // Alignment of the sub-allocations, the buffer will be itself aligned on this value
        , size_t         _Alignment
        // Whether we allocate the buffer on the first allocation or on construction
        , BufferInitMode _InitMode
        // The allocator responsible for providing the memory to the buffer provider
        , typename       _Allocator
        // The provider of the underlying block of memory
        , template<size_t, size_t, BufferInitMode, typename> class _BufferProvider
    >
    class linear_allocator
        : public _BufferProvider<_BufferSize, _Alignment, _InitMode, _Allocator>
    {
        //------------------------------------------------------------------------------------------
        using buffer_provider_t = _BufferProvider<_BufferSize, _Alignment, _InitMode, _Allocator>;

    public:
        //------------------------------------------------------------------------------------------
        static constexpr auto alignment                         = _Alignment;
        //------------------------------------------------------------------------------------------
        static constexpr auto supports_truncated_deallocation   = true;

    public:
        //------------------------------------------------------------------------------------------
        linear_allocator()
            : p_(buffer_provider_t::buffer_)
        {}

        //------------------------------------------------------------------------------------------
        // This constructor is enabled only if _BufferSize is a dynamic value (set at runtime)
        template<enable_if_workaround_t(is_dynamic_value(_BufferSize))>
        explicit linear_allocator(size_t bufferSize)
            : buffer_provider_t(bufferSize)
            , p_(buffer_provider_t::buffer_)
        {}

        //------------------------------------------------------------------------------------------
        // Can be moved only if the buffer provider can be moved
        linear_allocator(linear_allocator &&rhs)
            : buffer_provider_t(std::move(rhs))
            , p_(rhs.p_)
        {
            rhs.p_ = nullptr;
        }

        //------------------------------------------------------------------------------------------
        // Can't be copied
        linear_allocator(const linear_allocator &) = delete;

    public:
        //------------------------------------------------------------------------------------------
        // Allocator interface
        block allocate(size_t size)
        {
            const auto alignedSize = align(size);
            if (!hasEnoughSpace(alignedSize))
            {
                // Out of memory
                return nullblock;
            }

            // Lazy init
            buffer_provider_t::init(p_);

            block b{ p_, alignedSize };
            p_ += alignedSize;
            return b;
        }

        //------------------------------------------------------------------------------------------
        void deallocate(block &b)
        {
            // We can only deallocate the last allocated block
            if (isLastAllocatedBlock(b))
            {
                // Rewind the stack
                p_ = static_cast<uint8_t*>(b.ptr);
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
            if (isLastAllocatedBlock(b))
            {
                // Check if there's enough memory left
                if (static_cast<uint8_t*>(b.ptr) + alignedNewSize <= end())
                {
                    // If so, just update the pointer to the new end of the block
                    // note that it may have shrunk or grown
                    p_ = static_cast<uint8_t*>(b.ptr) + alignedNewSize;
                    return true;
                }

                // Out of memory
                return false;
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

        //------------------------------------------------------------------------------------------
        // This is enabled only if we are dynamically sizing our buffer
        template<enable_if_workaround_t(is_dynamic_value(_BufferSize))>
        void setBufferSize(size_t bufferSize)
        {
            buffer_provider_t::setValue(bufferSize);
            if (!is_lazy_init(_InitMode))
            {
                init(p_);
            }
        }

    private:
        // Helpers
        //------------------------------------------------------------------------------------------
        inline size_t align(size_t size) const
        {
            return round_to_alignment(size, alignment);
        }

        //------------------------------------------------------------------------------------------
        inline const uint8_t* begin() const
        {
            return buffer_provider_t::buffer_;
        }

        //------------------------------------------------------------------------------------------
        inline const uint8_t* end() const
        {
            return buffer_provider_t::buffer_ + buffer_provider_t::size();
        }

        //------------------------------------------------------------------------------------------
        inline bool hasEnoughSpace(size_t alignedSize) const
        {
            return (p_ + alignedSize) <= end();
        }

        //------------------------------------------------------------------------------------------
        inline bool isLastAllocatedBlock(const block &b) const
        {
            return (static_cast<uint8_t*>(b.ptr) + b.size) == p_;
        }

    private:
        //------------------------------------------------------------------------------------------
        // A pointer to the top of the stack
        uint8_t *p_;
    };

    //------------------------------------------------------------------------------------------
    // Shortcut to a linear allocator using a buffer on the stack
    template<size_t _BufferSize, size_t _Alignment = 8_B>
    using stack_linear_allocator = linear_allocator<_BufferSize, _Alignment, BufferInitMode::InitOnConstruct, void, stack_buffer_provider>;

    //------------------------------------------------------------------------------------------
    // Shortcut to a linear allocator using a buffer on the heap
    template<size_t _BufferSize, size_t _Alignment = 8_B, BufferInitMode _InitMode = BufferInitMode::InitOnConstruct, typename _Allocator = mallocator>
    using heap_linear_allocator = linear_allocator<_BufferSize, _Alignment, _InitMode, _Allocator, heap_buffer_provider>;

} /*abb*/
