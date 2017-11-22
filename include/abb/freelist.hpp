#pragma once

#include "abb/block.hpp"


namespace abb {

    //----------------------------------------------------------------------------------------------
    template
    <
        // The underlying allocator which will provide the actual memory blocks
          typename _Allocator
        // If the requested allocation size fits inside the inclusive range [_MinSize, _MaxSize]
        // the freelist will allocate a block of _MaxSize size which could be recycled on deallocation 
        , typename _Range
        // How many blocks the freelist can hold, if it's full the next block being deallocated
        // will not be kept in the list
        , size_t _MaxNodeCount
        // When allocating a block that fits in the freelist, this parameters enables the allocation of
        // a bunch on blocks in one step and add them in the freelist
        , size_t _BatchedAllocations
    >
    class freelist
        : public _Allocator
        , public _Range
    {
    public:
        //------------------------------------------------------------------------------------------
        static constexpr auto alignment                       = _Allocator::alignment;
        //------------------------------------------------------------------------------------------
        static constexpr auto supports_truncated_deallocation = _Allocator::supports_truncated_deallocation;

    public:
        //------------------------------------------------------------------------------------------
        static constexpr size_t min_size() { return _Range::min(); }
        //------------------------------------------------------------------------------------------
        static constexpr size_t max_size() { return _Range::max(); }

    private:
        //------------------------------------------------------------------------------------------
        // This structure represents the singly linked list that will be used to keep track of the
        // deallocated blocks. It will be written right inside freed memory.
        struct node
        {
            node *pNext_;
        };

    private:
        //------------------------------------------------------------------------------------------
        // Invariants
        static_assert(_MaxNodeCount > 0                     , "Pointless freelist.");
        static_assert(_BatchedAllocations <= _MaxNodeCount  , "Can't allocate more blocks than the _MaxNodeCount.");
        
    public:
        //------------------------------------------------------------------------------------------
        // The list starts empty
        freelist()
            : pHead_(nullptr)
            , currentNodeCount_(0)
        {}

        //------------------------------------------------------------------------------------------
        // Can be moved
        freelist(freelist &&rhs)
            : _Allocator(std::move(rhs))
            , pHead_(rhs.pHead_)
            , currentNodeCount_(rhs.currentNodeCount_)
        {
            rhs.pHead_ = nullptr;
            rhs.currentNodeCount_ = 0;
        }

        //------------------------------------------------------------------------------------------
        // Can't be copied
        freelist(const freelist &rhs) = delete;

        //------------------------------------------------------------------------------------------
        ~freelist()
        {
            // Properly deallocate every block still in the freelist
            while (pHead_)
            {
                auto b = block{ pHead_, max_size() };
                _Allocator::deallocate(b);
                pHead_ = pHead_->pNext_;
            }
        }

    public:
        //------------------------------------------------------------------------------------------
        // Allocator interface
        block allocate(size_t size)
        {
            const auto alignedSize = round_to_alignment(size, alignment);

            if (isGoodSize(alignedSize))
            {
                // The block fits in the freelist range
                // If the list is empty preallocate a bunch of blocks (parametrized by _BatchedAllocations)
                if (!pHead_)
                {
                    tryPopulateFreeList();
                }

                // If we managed to populate the freelist just pop it
                if (auto ptr = popNode())
                {
                    return block{ ptr, max_size() };
                }
            }

            // The size is outside the range, just fall back to the allocator
            return _Allocator::allocate(alignedSize);
        }

        //------------------------------------------------------------------------------------------
        void deallocate(block &b)
        {
            if (!isFull() && (b.size == max_size()))
            {
                // The pool still got some space
                pushNode(b.ptr);
            }
            else
            {
                // The pool is full, deallocate the block for real
                _Allocator::deallocate(b);
            }
        }

        //------------------------------------------------------------------------------------------
        bool reallocate(const block &b, size_t newSize)
        {
            if (handle_common_reallocation_cases(*this, b, newSize))
            {
                return true;
            }

            const auto alignedNewSize = round_to_alignment(newSize, alignment);
            if (isGoodSize(alignedNewSize))
            {
                return true;
            }

            return reallocate_and_copy(*this, *this, b, newSize);
        }

    public:
        //------------------------------------------------------------------------------------------
        void setMinMax(size_t minSize, size_t maxSize)
        {
            assert(maxSize >= sizeof(node) && "Maximum allocation size must be at least the size of a pointer.");
            _Range::setMinMax(minSize, maxSize);
        }

    private:
        //------------------------------------------------------------------------------------------
        // Helpers
        constexpr bool isGoodSize(size_t size) const
        {
            return min_size() <= size && size <= max_size();
        }

        //------------------------------------------------------------------------------------------
        bool isFull() const
        {
            return currentNodeCount_ == _MaxNodeCount;
        }

        //------------------------------------------------------------------------------------------
        void pushNode(void *ptr)
        {
            assert(ptr != nullptr);

            // The freed block becomes the new head
            auto pNewHead = static_cast<node*>(ptr);
            // Make the new head point to the old head
            pNewHead->pNext_ = pHead_;
            // Update the head
            pHead_ = pNewHead;
            // And the list count
            ++currentNodeCount_;
        }

        //------------------------------------------------------------------------------------------
        void* popNode()
        {
            void *ptr = nullptr;

            if (pHead_)
            {
                // The head of the freelist points directly to the first free block
                ptr = pHead_;
                // Update the new head
                pHead_ = pHead_->pNext_;
                // And the list count
                --currentNodeCount_;
            }

            return ptr;
        }

        //------------------------------------------------------------------------------------------
        void tryPopulateFreeList()
        {
            // We allocate blocks of _MaxSize to ensure that any size in the range fits in the freelist blocks
            const auto blockSize = max_size();
            // Don't go over _MaxNodeCount
            const auto numBlocks = std::min(_BatchedAllocations, _MaxNodeCount - currentNodeCount_);

            // There is an optimization opportunity here if the allocator supports truncated deallocation,
            // meaning that we can allocate a big chunk of memory and then deallocate only parts of it.
            // Typically a linear allocator, or really any allocator that does nothing on deallocation
            // is eligible to benefit from this optimization.
            bool batchedAllocationSucceeded = false;

            if (supports_truncated_deallocation)
            {
                // Allocate one big block then split it
                const auto batchSize    = numBlocks * blockSize;
                const auto batchBlock   = _Allocator::allocate(batchSize);

                // If the allocation succeeded proceed to splitting the block
                // Otherwise fall back to discreet allocation
                batchedAllocationSucceeded = (batchBlock.ptr != nullptr);
                if (batchedAllocationSucceeded)
                {
                    for (size_t i = 0; i < numBlocks; ++i)
                    {
                        pushNode(static_cast<uint8_t*>(batchBlock.ptr) + i * blockSize);
                    }
                }
            }

            // We either don't support truncated deallocations or the batch allocation failed
            if (!batchedAllocationSucceeded)
            {
                // Allocate the blocks one by one
                for (size_t i = 0; i < numBlocks; ++i)
                {
                    if (auto ptr = _Allocator::allocate(blockSize).ptr)
                    {
                        pushNode(ptr);
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }

    private:
        //------------------------------------------------------------------------------------------
        // Beginning of the freelist, nullptr means the list is empty
        node  *pHead_;
        // How many blocks are currently in the freelist
        size_t currentNodeCount_;
    };

} /*abb*/
