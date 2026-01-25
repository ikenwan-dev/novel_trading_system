#include <atomic>
#include <new>

// Use 64 bytes for cache line alignment (standard for x86 and Apple Silicon)
#if __cpp_lib_hardware_interference_size
    using std::hardware_destructive_interference_size;
#else
    constexpr size_t hardware_destructive_interference_size = 64;
#endif

template<typename T, size_t Capacity>
class SharedRingBuffer {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");

public:
    SharedRingBuffer() : head(0), tail(0) {}

    // --- Producer Side ---
    bool push(const T& item) {
        const size_t current_head = head.load(std::memory_order_relaxed);
        const size_t next_head = (current_head + 1) & (Capacity - 1);

        // Check if buffer is full
        if (next_head == tail.load(std::memory_order_acquire)) {
            return false; 
        }

        buffer[current_head] = item;
        // release ensures 'item' is written before 'head' is updated
        head.store(next_head, std::memory_order_release);
        return true;
    }

    // --- Consumer Side ---
    bool pop(T& item) {
        const size_t current_tail = tail.load(std::memory_order_relaxed);

        // Check if buffer is empty
        if (current_tail == head.load(std::memory_order_acquire)) {
            return false;
        }

        item = buffer[current_tail];
        // release ensures we are done reading before allowing Producer to overwrite
        tail.store((current_tail + 1) & (Capacity - 1), std::memory_order_release);
        return true;
    }

private:
    // keep head and tail on separate cache lines to prevent False Sharing
    alignas(hardware_destructive_interference_size) std::atomic<size_t> head;
    alignas(hardware_destructive_interference_size) std::atomic<size_t> tail;

    // The data array
    T buffer[Capacity];
};