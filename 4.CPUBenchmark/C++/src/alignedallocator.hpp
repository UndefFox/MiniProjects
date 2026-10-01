#include <cstddef>
#include <new>



template<class T, std::size_t Alignment = 64>
struct aligned_allocator {
    using value_type = T;

    aligned_allocator() noexcept = default;

    template<class U>
    constexpr aligned_allocator(const aligned_allocator<U, Alignment>&) noexcept {}

    [[nodiscard]]
    T* allocate(std::size_t n) {
        if (n > static_cast<std::size_t>(-1) / sizeof(T))
            throw std::bad_array_new_length{};

        return static_cast<T*>(
            ::operator new(n * sizeof(T), std::align_val_t{Alignment})
        );
    }

    void deallocate(T* p, std::size_t) noexcept {
        ::operator delete(p, std::align_val_t{Alignment});
    }

    template<class U>
    struct rebind {
        using other = aligned_allocator<U, Alignment>;
    };
};

template<class T, class U, std::size_t A>
bool operator==(const aligned_allocator<T, A>&,
                const aligned_allocator<U, A>&) noexcept {
    return true;
}

template<class T, class U, std::size_t A>
bool operator!=(const aligned_allocator<T, A>&,
                const aligned_allocator<U, A>&) noexcept {
    return false;
}
