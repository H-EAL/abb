#pragma once

#include <cassert>
#include "abb/block.hpp"
#include "abb/range_helpers.hpp"


namespace abb {

    //----------------------------------------------------------------------------------------------
    enum class BufferInitMode
    {
        // The buffer is allocated the first time we call allocate
        InitOnFirstAllocation,
        // The buffer is allocated when we construct the buffer provider
        InitOnConstruct
    };
    //----------------------------------------------------------------------------------------------
    inline constexpr bool is_lazy_init(BufferInitMode initMode)
    {
        return initMode == BufferInitMode::InitOnFirstAllocation;
    }
    //----------------------------------------------------------------------------------------------

    
    //----------------------------------------------------------------------------------------------
    // Stack buffer provider
    template
    <
        // The size of the chunk of memory that's going to be allocated
          size_t _BufferSize
        // Alignment of the buffer, the buffer size should be a multiple of this value
        , size_t _Alignment
        // We use the stack so no need to specify any init mode
        , BufferInitMode
        // The buffer is on the stack so no need for an allocator
        , typename
    >
    struct stack_buffer_provider
    {
        //------------------------------------------------------------------------------------------
        static_assert(!is_dynamic_value(_BufferSize), "Can't create a dynamically sized buffer on the stack. Set a fixed size or use a heap_buffer_provider");

        //------------------------------------------------------------------------------------------
        stack_buffer_provider() = default;

        //------------------------------------------------------------------------------------------
        // No move nor copy
        stack_buffer_provider(stack_buffer_provider &&)         = delete;
        stack_buffer_provider(const stack_buffer_provider &)    = delete;

        //------------------------------------------------------------------------------------------
        static constexpr void   init(uint8_t *&ptr) {}
        static constexpr size_t size()              { return _BufferSize; }

        //------------------------------------------------------------------------------------------
        alignas(_Alignment) uint8_t buffer_[_BufferSize];
    };
    //----------------------------------------------------------------------------------------------


    //----------------------------------------------------------------------------------------------
    // Heap buffer provider
    template
    <
        // The size of the chunk of memory that's going to be allocated
          size_t         _BufferSize 
        // Alignment of the buffer, the buffer size should be a multiple of this value
        , size_t         _Alignment
        // Whether we allocate the memory on creation or on the first allocate
        , BufferInitMode _InitMode
        // The allocator from where to allocate the buffer
        , typename       _Allocator
    >
    struct heap_buffer_provider
        : public std::conditional
            <
                is_dynamic_value(_BufferSize)
                , dynamic_value_t<size_t>
                , static_value_t<size_t, _BufferSize>
            >::type
        , private _Allocator
    {
        //------------------------------------------------------------------------------------------
        using value_type_t = typename std::conditional<is_dynamic_value(_BufferSize), dynamic_value_t<size_t>, static_value_t<size_t, _BufferSize>>::type;

    public:
        //------------------------------------------------------------------------------------------
        // Default constructor available for both dynamic and static sizes
        heap_buffer_provider()
            : buffer_(is_lazy_init(_InitMode) || is_dynamic_value(_BufferSize) ? nullptr : static_cast<uint8_t*>(_Allocator::allocate(_BufferSize).ptr))
        {}

        //------------------------------------------------------------------------------------------
        // This constructor is enabled only if we are dynamically sizing our buffer
        template<enable_if_workaround_t(is_dynamic_value(_BufferSize))>
        explicit heap_buffer_provider(size_t dynamicBufferSize)
            : value_type_t(dynamicBufferSize)
            , buffer_(is_lazy_init(_InitMode) ? nullptr : static_cast<uint8_t*>(_Allocator::allocate(value_type_t::value())))
        {}

        //------------------------------------------------------------------------------------------
        // Can be moved
        heap_buffer_provider(heap_buffer_provider &&rhs)
            : buffer_(rhs.buffer_)
        {
            rhs.buffer_ = nullptr;
        }

        //------------------------------------------------------------------------------------------
        // Can't be copied
        heap_buffer_provider(const heap_buffer_provider &) = delete;

        //------------------------------------------------------------------------------------------
        ~heap_buffer_provider()
        {
            if (buffer_)
            {
                _Allocator::deallocate(block{ buffer_, size() });
                buffer_ = nullptr;
            }
        }

        //------------------------------------------------------------------------------------------
        void init(uint8_t *&ptr)
        {
            if (is_lazy_init(_InitMode) && !buffer_)
            {
                assert(value_type_t::is_set());
                buffer_ = static_cast<uint8_t*>(_Allocator::allocate(size()).ptr);
                ptr = buffer_;
            }
        }

        //------------------------------------------------------------------------------------------
        constexpr size_t size() const
        {
            return value_type_t::value();
        }

    protected:
        //------------------------------------------------------------------------------------------
        uint8_t *buffer_;
    };

} /*abb*/
