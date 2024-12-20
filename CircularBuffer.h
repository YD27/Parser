#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <vector>
#include <atomic>
#include <optional>

template <typename T>
class CircularBuffer {
private:
    std::vector<T> buffer;
    std::atomic<size_t> head;
    std::atomic<size_t> tail;
    const size_t capacity;

public:
    
    CircularBuffer(size_t capacity)
        : buffer(capacity), head(0), tail(0), capacity(capacity) {}

    
    bool push(const T& item) {
        size_t currentTail = tail.load(std::memory_order_relaxed);
        size_t nextTail = (currentTail + 1) % capacity;

        if (nextTail == head.load(std::memory_order_acquire)) {
            // Buffer is full
            return false;
        }

        buffer[currentTail] = item;
        tail.store(nextTail, std::memory_order_release);
        return true;
    }

    
    std::optional<T> pop() {
        size_t currentHead = head.load(std::memory_order_relaxed);

        if (currentHead == tail.load(std::memory_order_acquire)) {
            // Buffer is empty
            return std::nullopt;
        }

        T item = buffer[currentHead];
        head.store((currentHead + 1) % capacity, std::memory_order_release);
        return item;
    }

    bool isEmpty() const {
        return head.load(std::memory_order_acquire) == tail.load(std::memory_order_acquire);
    }
};

#endif // CIRCULAR_BUFFER_H
