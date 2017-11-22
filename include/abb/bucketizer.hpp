#pragma once

#include "abb/block.hpp"


namespace abb {

    //----------------------------------------------------------------------------------------------
    template
    <
          typename _Allocator
        , typename _RangeRaider
    >
    class bucketizer
        : public _RangeRaider
    {
    public:
        //------------------------------------------------------------------------------------------
        static constexpr auto alignment     = _Allocator::alignment;
        //------------------------------------------------------------------------------------------
        static constexpr auto num_buckets   = _RangeRaider::num_steps;

    public:
        //------------------------------------------------------------------------------------------
        bucketizer()
        {
            auto currentBucketMinSize = _RangeRaider::min();
            int i = 0;
            for (auto &bucket : buckets_)
            {
                const size_t stepSize = _RangeRaider::step_size(i++);
                bucket.setMinMax(currentBucketMinSize + (i - 1 ? 1 : 0), currentBucketMinSize + stepSize);
                currentBucketMinSize += stepSize;
            }
        }

    public:
        //------------------------------------------------------------------------------------------
        block allocate(size_t size)
        {
            return isGoodSize(size)
                ?  buckets_[bucketIndex(size)].allocate(size)
                :  block{ nullptr, 0 };
        }

        //------------------------------------------------------------------------------------------
        void deallocate(block &b)
        {
            if (isGoodSize(b.size))
            {
                buckets_[bucketIndex(b.size)].deallocate(b);
            }
        }

        //------------------------------------------------------------------------------------------
        bool reallocate(block &b, size_t newSize)
        {
            if (!isGoodSize(newSize))
            {
                return false;
            }

            if (handle_common_reallocation_cases(*this, b, newSize))
            {
                return true;
            }

            const auto oldBucketIndex = bucketIndex(b.size);
            const auto newBucketIndex = bucketIndex(newSize);

            if (oldBucketIndex == newBucketIndex)
            {
                return buckets_[newBucketIndex].reallocate(b, newSize);
            }
            
            return reallocate_and_copy(buckets_[oldBucketIndex], buckets_[newBucketIndex], b, newSize);
        }

        //------------------------------------------------------------------------------------------
        bool owns(const block &b) const
        {
            return isGoodSize(b.size)
                ?  buckets_[bucketIndex(b.size)].owns(b)
                :  false;
        }

    private:
        //------------------------------------------------------------------------------------------
        constexpr bool isGoodSize(size_t size) const
        {
            return (_RangeRaider::min() <= size) && (size <= _RangeRaider::max());
        }

        //------------------------------------------------------------------------------------------
        constexpr size_t bucketIndex(size_t size) const
        {
            return _RangeRaider::step_index(size);
        }

    private:
        //------------------------------------------------------------------------------------------
        _Allocator buckets_[num_buckets];
    };

} /*abb*/
