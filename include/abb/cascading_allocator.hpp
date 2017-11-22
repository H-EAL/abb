#pragma once

#include "abb/block.hpp"


namespace abb {

    //----------------------------------------------------------------------------------------------
    template<typename _Allocator>
    class cascading_allocator
    {
    public:
        //------------------------------------------------------------------------------------------
        static constexpr auto alignment                       = _Allocator::alignment;
        //------------------------------------------------------------------------------------------
        static constexpr auto supports_truncated_deallocation = _Allocator::supports_truncated_deallocation;

    public:
        //------------------------------------------------------------------------------------------
        static_assert(std::is_move_constructible<_Allocator>::value, "_Allocator must be movable");

    private:
        //------------------------------------------------------------------------------------------
        struct node
        {
            _Allocator  allocator_;
            node        *pNext_;

            node()
                : pNext_(nullptr)
            {}

            node(node &&rhs)
                : allocator_(std::move(rhs.allocator_))
                , pNext_(rhs.pNext_)
            {
                rhs.pNext_ = nullptr;
            }
        };

    public:
        //------------------------------------------------------------------------------------------
        cascading_allocator()
            : pHead_(nullptr)
            , nodeAllocatedSize_(0)
        {}

        //------------------------------------------------------------------------------------------
        ~cascading_allocator()
        {
            eraseAllNodes();
        }

    public:
        //------------------------------------------------------------------------------------------
        block allocate(size_t size)
        {
            // Try to allocate from each node of the list
            auto b = allocateNoGrow(size);
            if (b.ptr)
            {
                return b;
            }

            // No node is able to allocate the requested size, just add a node and allocate from it
            if (auto pNode = prependNode())
            {
                return pNode->allocator_.allocate(size);
            }

            // No luck
            return block{ nullptr, 0 };
        }

        //------------------------------------------------------------------------------------------
        void deallocate(block &b)
        {
            if (auto pNode = findOwningNode(b))
            {
                pNode->allocator_.deallocate(b);
            }
        }

        //------------------------------------------------------------------------------------------
        bool reallocate(block &b, size_t newSize)
        {
            if (handle_common_reallocation_cases(*this, b, newSize))
            {
                return true;
            }

            auto pNode = findOwningNode(b);
            if (pNode == nullptr)
            {
                return false;
            }

            if (pNode->allocator_.reallocate(b, newSize))
            {
                return true;
            }

            return reallocate_and_copy(pNode->allocator_, *this, b, newSize);
        }

        //------------------------------------------------------------------------------------------
        bool owns(const block &b) const
        {
            return findOwningNode(b) != nullptr;
        }

    public:
        //------------------------------------------------------------------------------------------
        void deallocateAll()
        {
            // Empty allocator, nothing to do
            if (pHead_ == nullptr)
            {
                return;
            }

            // Erase everything after the head
            eraseNode(pHead_->pNext_);

            // Move the head node on the stack
            node stackNode(std::move(*pHead_));

            // Deallocate everything, this will also deallocate the space for the allocator that was in the head node!
            stackNode.allocator_.deallocateAll();

            // Allocate back some space
            auto nodeBlock = stackNode.allocator_.allocate(sizeof(node));
            pHead_ = static_cast<node*>(nodeBlock.ptr);

            // Move back the head inside its allocator
            new (pHead_) node(std::move(stackNode));
        }

    private:
        //------------------------------------------------------------------------------------------
        block allocateNoGrow(size_t size)
        {
            auto b      = block{ nullptr, 0 };
            auto pNode  = pHead_;
            while (pNode)
            {
                b = pNode->allocator_.allocate(size);
                if (b.ptr)
                {
                    return b;
                }
                pNode = pNode->pNext_;
            }
            return b;
        }

        //------------------------------------------------------------------------------------------
        node* prependNode()
        {
            // Start with creating a new node
            auto pNewNode = createNode();

            // If we failed at creating a new node it means we're most likely out of memory
            if (pNewNode == nullptr)
            {
                return nullptr;
            }

            // Make the new node point to the current head
            pNewNode->pNext_ = pHead_;
            // The new node becomes the current head
            pHead_ = pNewNode;

            return pNewNode;
        }

    private:
        //------------------------------------------------------------------------------------------
        node* createNode()
        {
            // First create a node embedding an allocator on the stack
            node stackNode;
            // Then get a block from that allocator to move the stack node in it
            auto nodeBlock = stackNode.allocator_.allocate(sizeof(node));
            auto pNewNode  = static_cast<node*>(nodeBlock.ptr);
            // Most likely out of memory
            if (!pNewNode)
            {
                return nullptr;
            }
            // All nodes should have the same size for a given allocator
            assert(nodeAllocatedSize_ == 0 || nodeAllocatedSize_ == nodeBlock.size);
            nodeAllocatedSize_ = nodeBlock.size;
            // Move the stack node to the allocated block
            new (pNewNode) node(std::move(stackNode));
            return pNewNode;
        }

        //------------------------------------------------------------------------------------------
        void eraseNode(node *&n)
        {
            // No node, nothing to erase
            if (n == nullptr)
            {
                return;
            }
            // Recursively erase all child nodes first
            if (n->pNext_)
            {
                eraseNode(n->pNext_);
            }
            // Move the node on the stack so it can be properly destructed
            node stackNode(std::move(*n));
            // Recreate a block from the node pointer
            auto b = block{ n, nodeAllocatedSize_ };
            // Deallocate ourselves from ourselves o_O
            stackNode.allocator_.deallocate(b);
            // Null out the node
            n = nullptr;
        }

        //------------------------------------------------------------------------------------------
        void eraseAllNodes()
        {
            eraseNode(pHead_);
        }

        //------------------------------------------------------------------------------------------
        node* findOwningNode(const block &b)
        {
            auto pNode = pHead_;
            while (pNode)
            {
                if (pNode->allocator_.owns(b))
                {
                    return pNode;
                }
                pNode = pNode->pNext_;
            }
            return nullptr;
        }

    private:
        //------------------------------------------------------------------------------------------
        node    *pHead_;
        size_t  nodeAllocatedSize_;
    };

} /*abb*/
