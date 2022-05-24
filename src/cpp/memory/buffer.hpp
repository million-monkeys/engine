#pragma once

// WIP: try to come up with a modular solution for paged allocation system for use by event streams, command stream and messages. Perhaps also for storing resources.
// Needs more thought.
// Basic requirements:
//      1. Event streams need a double buffered single writer multiple reader stack allocator
//      2. Command stream needs a multiple writer single reader stack allocator. Doesn't matter if single or double buffered
//      3. Messages need a multiple writer single reader stack allocator that supports both single and doulble buffered modes, with remaining messages persisting between buffer swaps
//      4. Resources probably need a pool allocator
//
// Should support multiple "pages" of buffers, so if a buffer runs out of space, another one is taken from a pool, and used to expand the size of the previous.
// This would require allocation to switch to a new buffer (possibly atomically) and iteration to handle switching from one buffer to the next

#include <cstdint>
#include <cstddef>
#include <utility>
#include <stdexcept>

namespace memory {

    class Buffer {
    public:
        Buffer () : m_size(0), m_memory(nullptr) {}
        Buffer (std::size_t capacity) : m_size(capacity), m_memory(new std::byte[capacity]) {}
        Buffer (Buffer&& other) : m_size(other.m_size), m_memory(other.m_memory) {other.m_memory = nullptr; }
        ~Buffer() { if (m_memory) {delete [] m_memory;} }

        bool valid () const { return m_memory != nullptr; }
        std::size_t size () const { return m_size; }
        std::byte* data () { return m_memory; }

        std::byte& operator[](std::size_t index)
        {
            if (index < m_size) {
                return m_memory[index];
            } else {
                throw std::out_of_range("Buffer index out of range");
            }
        }

        Buffer view (std::size_t start_index, std::size_t size)
        {
            if (start_index + size <= m_size) {
                return Buffer(m_memory + start_index, start_index + size);
            } else {
                return Buffer();
            }
        }

    private:
        Buffer(std::byte* memory, std::size_t size) : m_size(size), m_memory(memory) {}

        const std::size_t m_size;
        std::byte* m_memory;
    };

    class StackAllocator {
    public:
        std::size_t allocate (std::size_t amount)
        {
            auto ptr = m_next;
            m_next += amount;
            return ptr;
        }

        void reset ()
        {
            m_next = 0;
        }

        std::size_t used () const { return m_next; }
        std::size_t free () const { return m_max - m_next; }
    private:
        std::size_t m_max;
        std::size_t m_next;
    };

    class SingleBuffer {
    public:
        SingleBuffer (std::size_t capacity) : m_buffer(capacity) {}
        ~SingleBuffMemoryManager() {}

        Buffer view (std::size_t start_index, std::size_t size)
        {
            return m_buffer.view(start_index, size);
        }

        void swap (std::size_t) {}

        Buffer data (std::size_t size)
        {
            return m_buffer.view(0, size);
        }

    private:
        Buffer m_buffer;
    };

    class DoubleBuffer {
    public:
        DoubleBuffer (std::size_t capacity) : m_buffers{{capacity}, {capacity}}, m_current(0), m_used(0) {}
        ~DoubleBuffer() {}

        Buffer view (std::size_t start_index, std::size_t size)
        {
            return m_buffers[m_current].view(start_index, size);
        }

        void swap (std::size_t used)
        {
            m_current = 1 - m_current;
            m_used = used;
        }

        Buffer data (std::size_t)
        {
            return m_buffers[1 - m_current].view(0, m_used);
        }

    private:
        Buffer m_buffers[2];
        std::size_t m_current;
        std::size_t m_used;
    };

    template <typename BufferT, typename AllocatorT>
    class MemoryManager {
    public:
        MemoryManager (std::size_t capacity) : m_memory(capacity) {}
        ~MemoryManager() {}

        Buffer allocate (std::size_t amount)
        {
            return m_memory.view(m_allocator.allocate(amount), amount);
        }

        void reset ()
        {
            m_memory.swap(m_allocator.used());
            m_allocator.reset();
        }

        Buffer data ()
        {
            return m_memory.data(m_allocator.used());
        }

    private:
        BufferT m_memory;
        AllocatorT m_allocator;
    };

    using StackDblBuff = MemoryManager<DoubleBuffer, StackAllocator>;
}